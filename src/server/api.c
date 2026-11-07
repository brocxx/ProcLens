#include "api.h"
#include "../analysis/advisor.h"
#include "../collector/snapshot.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* JSON strings are escaped byte by byte. Input is UTF-8; controls cannot escape. */
static void json_string(FILE *file, const char *text)
{
    const unsigned char *p = (const unsigned char *)(text == NULL ? "" : text);
    fputc('"', file);
    for (; *p != 0; p++) {
        if (*p == '"' || *p == '\\') fprintf(file, "\\%c", *p);
        else if (*p < 32) fprintf(file, "\\u%04x", (unsigned int)*p);
        else fputc(*p, file);
    }
    fputc('"', file);
}

/* Initialize empty owned containers; O(1). */
void api_init(ApiState *state)
{
    memset(state, 0, sizeof(*state));
    dynamic_array_init(&state->observed, sizeof(ProcessRecord));
    dynamic_array_init(&state->jobs, sizeof(Job));
    comparison_report_init(&state->report);
}

/* Release nested strings before their arrays; O(records + jobs + events). */
void api_destroy(ApiState *state)
{
    size_t i;
    for (i = 0; i < state->observed.length; i++)
        process_record_destroy(dynamic_array_at(&state->observed, i));
    for (i = 0; i < state->jobs.length; i++)
        job_destroy(dynamic_array_at(&state->jobs, i));
    dynamic_array_destroy(&state->observed);
    dynamic_array_destroy(&state->jobs);
    comparison_report_destroy(&state->report);
    api_init(state);
}

/* A small bounded query reader. Duplicate/oversized fields fail; O(query length). */
static int query_value(const char *query, const char *name, char *value, size_t size)
{
    size_t name_length = strlen(name);
    int found = 0;
    while (*query != 0) {
        const char *end = strchr(query, '&');
        size_t length = end == NULL ? strlen(query) : (size_t)(end - query);
        if (length > name_length && strncmp(query, name, name_length) == 0 &&
            query[name_length] == '=') {
            const char *cursor = query + name_length + 1;
            const char *end = query + length;
            size_t used = 0;
            if (found) return -1;
            while (cursor < end) {
                unsigned char decoded = (unsigned char)*cursor++;
                if (decoded == '+') {
                    decoded = ' ';
                } else if (decoded == '%') {
                    unsigned int high, low;
                    char a, b;
                    if (end - cursor < 2) return -1;
                    a = *cursor++; b = *cursor++;
                    if (a >= '0' && a <= '9') high = (unsigned int)(a - '0');
                    else if (a >= 'A' && a <= 'F') high = (unsigned int)(a - 'A' + 10);
                    else if (a >= 'a' && a <= 'f') high = (unsigned int)(a - 'a' + 10);
                    else return -1;
                    if (b >= '0' && b <= '9') low = (unsigned int)(b - '0');
                    else if (b >= 'A' && b <= 'F') low = (unsigned int)(b - 'A' + 10);
                    else if (b >= 'a' && b <= 'f') low = (unsigned int)(b - 'a' + 10);
                    else return -1;
                    decoded = (unsigned char)(high * 16U + low);
                    if (decoded == 0) return -1;
                }
                if (used + 1 >= size) return -1;
                value[used++] = (char)decoded;
            }
            value[used] = 0;
            found = 1;
        }
        if (end == NULL) break;
        query = end + 1;
    }
    return found;
}

/* Parse unsigned, bounded decimal values; never accept signs or trailing junk. */
static int number(const char *query, const char *name, unsigned long fallback,
                  unsigned long minimum, unsigned long maximum, unsigned long *out)
{
    char text[32];
    unsigned long value = 0;
    size_t i;
    int found = query_value(query, name, text, sizeof(text));
    if (found == 0) { *out = fallback; return 1; }
    if (found < 0 || text[0] == 0) return 0;
    for (i = 0; text[i] != 0; i++) {
        unsigned long digit;
        if (text[i] < '0' || text[i] > '9') return 0;
        digit = (unsigned long)(text[i] - '0');
        if (value > maximum / 10 || (value == maximum / 10 && digit > maximum % 10)) return 0;
        value = value * 10 + digit;
    }
    if (value < minimum || value > maximum) return 0;
    *out = value;
    return 1;
}

static int failure(FILE *output, int status, const char *message)
{
    fputs("{\"error\":", output);
    json_string(output, message);
    fputs("}", output);
    return status;
}

static void write_metrics(FILE *file, const SimulationResult *r)
{
    fprintf(file, "\"average_waiting\":%.8f,\"average_response\":%.8f,"
            "\"average_turnaround\":%.8f,\"utilization\":%.8f,"
            "\"fairness\":%.8f,\"throughput\":%.8f,\"context_switches\":%llu,"
            "\"starved_count\":%zu,\"max_waiting\":%llu,\"makespan\":%llu",
            r->average_waiting, r->average_response, r->average_turnaround,
            r->utilization, r->fairness, r->throughput, r->context_switches,
            r->starved_count, r->max_waiting, r->makespan);
}

static void write_run(FILE *file, const AlgorithmRun *run, double threshold)
{
    size_t i;
    const ListNode *node;
    fputs("{\"name\":", file); json_string(file, run->name); fputc(',', file);
    write_metrics(file, &run->result);
    fputs(",\"events\":[", file);
    for (i = 0; i < run->result.events.length; i++) {
        const GanttEvent *e = dynamic_array_const_at(&run->result.events, i);
        fprintf(file, "%s{\"pid\":%lu,\"start_tick\":%llu,\"end_tick\":%llu,\"kind\":%d}",
                i ? "," : "", e->pid, e->start_tick, e->end_tick, (int)e->kind);
    }
    fputs("],\"metrics\":[", file);
    for (i = 0; i < run->result.metrics.length; i++) {
        const JobMetrics *m = dynamic_array_const_at(&run->result.metrics, i);
        fprintf(file, "%s{\"pid\":%lu,\"waiting_tick\":%llu,\"response_tick\":%llu,"
                "\"turnaround_tick\":%llu,\"completion_tick\":%llu,\"starved\":%s}",
                i ? "," : "", m->pid, m->waiting_tick, m->response_tick,
                m->turnaround_tick, m->completion_tick,
                (double)m->waiting_tick > threshold ? "true" : "false");
    }
    fputs("],\"decisions\":[", file);
    for (node = run->result.decisions.head; node != NULL; node = node->next) {
        if (node != run->result.decisions.head) fputc(',', file);
        json_string(file, node->value);
    }
    fputs("]}", file);
}

static void write_observed(FILE *file, const DynamicArray *records)
{
    size_t i;
    fputc('[', file);
    for (i = 0; i < records->length; i++) {
        const ProcessRecord *p = dynamic_array_const_at(records, i);
        fprintf(file, "%s{\"pid\":%lu,\"ppid\":%lu,\"name\":", i ? "," : "", p->pid, p->ppid);
        json_string(file, p->name);
        fprintf(file, ",\"cpu_time_ms\":%llu,\"memory_kb\":%llu,\"memory_available\":%s,"
                "\"priority_raw\":%ld,\"timestamp_ms\":%llu,\"sample_index\":%lu,\"start_time\":",
                p->cpu_time_ms, p->memory_kb, p->memory_kb == 0 ? "false" : "true",
                p->priority_raw, p->timestamp_ms, p->sample_index);
        json_string(file, p->start_time); fputs(",\"os\":", file);
        json_string(file, p->os); fputc('}', file);
    }
    fputc(']', file);
}

static void write_jobs(FILE *file, const DynamicArray *jobs)
{
    size_t i;
    fputc('[', file);
    for (i = 0; i < jobs->length; i++) {
        const Job *j = dynamic_array_const_at(jobs, i);
        fprintf(file, "%s{\"pid\":%lu,\"ppid\":%lu,\"name\":", i ? "," : "", j->pid, j->ppid);
        json_string(file, j->name);
        fprintf(file, ",\"arrival_tick\":%llu,\"burst_tick\":%llu,\"remaining_tick\":%llu,\"priority\":%u}",
                j->arrival_tick, j->burst_tick, j->remaining_tick, j->priority);
    }
    fputc(']', file);
}

static int compact_latest_observations(DynamicArray *records)
{
    ProcessRecord *latest;
    unsigned char *retained;
    size_t count = 0, i, j;
    if (records->length == 0) return 1;
    latest = malloc(records->length * sizeof(*latest));
    retained = (unsigned char *)calloc(records->length, sizeof(*retained));
    if (latest == NULL || retained == NULL) {
        free(latest);
        free(retained);
        return 0;
    }
    for (i = 0; i < records->length; i++) {
        int superseded = 0;
        for (j = i + 1; j < records->length; j++) {
            const ProcessRecord *current = dynamic_array_const_at(records, i);
            const ProcessRecord *candidate = dynamic_array_const_at(records, j);
            if (candidate->pid == current->pid &&
                candidate->sample_index >= current->sample_index) {
                superseded = 1;
                break;
            }
        }
        if (!superseded) {
            latest[count++] = *(ProcessRecord *)dynamic_array_at(records, i);
            retained[i] = 1;
        }
    }
    for (i = 0; i < records->length; i++) {
        ProcessRecord *record = dynamic_array_at(records, i);
        if (retained[i]) process_record_init(record);
        else process_record_destroy(record);
    }
    free(records->data);
    free(retained);
    records->data = (unsigned char *)latest;
    records->length = count;
    records->capacity = count;
    return 1;
}

static int capture_job_precedes(const Job *left, const Job *right)
{
    if (left->burst_tick != right->burst_tick)
        return left->burst_tick > right->burst_tick;
    if (left->arrival_tick != right->arrival_tick)
        return left->arrival_tick < right->arrival_tick;
    return left->pid < right->pid;
}

static int bound_capture_jobs(ApiState *state)
{
    DynamicArray selected;
    unsigned char *keep;
    size_t count = state->jobs.length, selected_count = 0, i;
    unsigned long long total = 0;
    keep = (unsigned char *)calloc(count, sizeof(*keep));
    if (count != 0 && keep == NULL) return 0;
    while (selected_count < 128) {
        size_t best = count;
        for (i = 0; i < count; i++) {
            const Job *candidate = dynamic_array_const_at(&state->jobs, i);
            if (keep[i] || candidate->burst_tick == 0 ||
                candidate->burst_tick > 20000 ||
                total + candidate->burst_tick > 20000) continue;
            if (best == count ||
                capture_job_precedes(candidate, dynamic_array_const_at(&state->jobs, best)))
                best = i;
        }
        if (best == count) break;
        keep[best] = 1;
        total += ((const Job *)dynamic_array_const_at(&state->jobs, best))->burst_tick;
        selected_count++;
    }
    if (!dynamic_array_init(&selected, sizeof(Job))) {
        free(keep);
        return 0;
    }
    for (i = 0; i < count; i++) {
        if (keep[i] && !dynamic_array_push(&selected, dynamic_array_at(&state->jobs, i))) {
            dynamic_array_destroy(&selected);
            free(keep);
            return 0;
        }
    }
    for (i = 0; i < count; i++) {
        Job *job = dynamic_array_at(&state->jobs, i);
        if (keep[i]) job->name = NULL;
        job_destroy(job);
    }
    dynamic_array_destroy(&state->jobs);
    state->jobs = selected;
    state->omitted_jobs = count - selected_count;
    free(keep);
    return 1;
}

static void write_recommendation(FILE *file, const ApiState *state)
{
    const WorkloadFeatures *f = &state->recommendation.features;
    if (!state->recommended) { fputs("null", file); return; }
    fprintf(file, "{\"policy\":%d,\"name\":", (int)state->recommendation.policy);
    json_string(file, algorithm_name(state->recommendation.policy));
    fputs(",\"explanation\":", file); json_string(file, state->recommendation.explanation);
    fprintf(file, ",\"features\":{\"job_count\":%zu,\"mean_burst\":%.5f,"
            "\"burst_variance\":%.5f,\"short_job_share\":%.5f,\"long_job_share\":%.5f,"
            "\"arrival_spread\":%llu,\"priority_spread\":%u,\"cpu_heavy_count\":%zu}}",
            f->job_count, f->mean_burst, f->burst_variance, f->short_job_share,
            f->long_job_share, f->arrival_spread, f->priority_spread, f->cpu_heavy_count);
}

static int write_state(FILE *file, const ApiState *state)
{
    DynamicArray groups;
    size_t i;
    double burst = 0;
    fprintf(file, "{\"mode\":\"%s\",\"observed\":", state->captured ? "capture" : state->jobs.length ? "demo" : "empty");
    write_observed(file, &state->observed);
    fputs(",\"jobs\":", file); write_jobs(file, &state->jobs);
    fprintf(file, ",\"omitted_jobs\":%zu", state->omitted_jobs);
    for (i = 0; i < state->jobs.length; i++)
        burst += (double)((const Job *)dynamic_array_const_at(&state->jobs, i))->burst_tick;
    fputs(",\"runs\":[", file);
    for (i = 0; i < state->report.count; i++) {
        if (i) fputc(',', file);
        write_run(file, &state->report.runs[i], state->jobs.length ? 5 * burst / state->jobs.length : 0);
    }
    fputs("],\"recommendation\":", file); write_recommendation(file, state);
    fputs(",\"validation\":", file);
    if (state->validated) fprintf(file, "{\"runs\":%zu,\"matches\":%zu,\"accuracy\":%.6f,\"objective\":\"average_waiting\",\"seed\":42}",
                                 state->validation.runs, state->validation.matches, state->validation.accuracy);
    else fputs("null", file);
    fputs(",\"groups\":[", file);
    if (state->observed.length && process_group_summarize((ProcessRecord *)state->observed.data, state->observed.length, &groups)) {
        for (i = 0; i < groups.length; i++) {
            const ProcessGroupSummary *g = dynamic_array_const_at(&groups, i);
            fprintf(file, "%s{\"root_pid\":%lu,\"cpu_time_ms\":%llu,\"memory_kb\":%llu,"
                    "\"memory_available\":%s,\"process_count\":%zu}",
                    i ? "," : "", g->root_pid, g->cpu_time_ms, g->memory_kb,
                    g->memory_kb == 0 ? "false" : "true", g->process_count);
        }
        dynamic_array_destroy(&groups);
    }
    fputs("]}", file);
    return ferror(file) ? 500 : 200;
}

/* Bound tick-based work before running a synchronous local simulation. */
static int workload_allowed(const ApiState *state)
{
    size_t i;
    unsigned long long total = 0;
    if (!state->jobs.length || state->jobs.length > 128) return 0;
    for (i = 0; i < state->jobs.length; i++) {
        const Job *job = dynamic_array_const_at(&state->jobs, i);
        if (!job->burst_tick || job->burst_tick > 20000 || job->arrival_tick > 20000) return 0;
        total += job->burst_tick;
    }
    return total <= 20000;
}

static int simulate(ApiState *state, const char *query, FILE *file)
{
    AlgorithmKind selected[6];
    char list[32];
    size_t count = 0, i;
    unsigned long quantum, cost, interval, amount;
    int found;
    if (!workload_allowed(state)) return failure(file, 422, "Load 1-128 jobs with at most 20000 total burst ticks.");
    if (!number(query,"quantum",2,1,100,&quantum) || !number(query,"context",0,0,10,&cost) ||
        !number(query,"interval",4,1,1000,&interval) || !number(query,"amount",1,0,9,&amount))
        return failure(file,400,"Invalid simulation settings.");
    found = query_value(query,"algorithms",list,sizeof(list));
    if (found < 0) return failure(file,400,"Invalid algorithm selection.");
    if (!found) { for (i=0;i<6;i++) selected[count++]=(AlgorithmKind)i; }
    else {
        size_t length = strlen(list);
        unsigned int seen = 0;
        if (!length || !(length % 2)) return failure(file,400,"Use comma-separated policy IDs 0-5.");
        for (i=0;i<length;i+=2) {
            unsigned int id = (unsigned int)(list[i]-'0');
            if (id>5 || (seen & (1U<<id)) || (i+1<length && list[i+1]!=','))
                return failure(file,400,"Invalid or duplicate policy ID.");
            seen |= 1U<<id; selected[count++]=(AlgorithmKind)id;
        }
    }
    if (!comparison_report_run((Job *)state->jobs.data,state->jobs.length,selected,count,quantum,cost,interval,(unsigned int)amount,&state->report))
        return failure(file,500,"Simulation failed.");
    for (i=0;i<state->report.count;i++)
        if (!simulation_validate((Job *)state->jobs.data,state->jobs.length,&state->report.runs[i].result))
            return failure(file,500,"Simulation invariant check failed.");
    return write_state(file,state);
}

static int capture(ApiState *state, const char *query, FILE *file)
{
    ApiState next;
    Collector collector;
    unsigned long seconds;
    if (!number(query,"seconds",3,1,10,&seconds)) return failure(file,400,"Capture duration must be 1-10 seconds.");
    api_init(&next);
    if (!collector_init(&collector)) return failure(file,500,"Collector initialization failed.");
    if (!snapshot_collect(&collector,seconds,1000,&next.observed)) {
        collector_destroy(&collector); api_destroy(&next);
        return failure(file,500,"Read-only capture failed.");
    }
    collector_destroy(&collector);
    if (!snapshot_records_to_jobs((ProcessRecord *)next.observed.data,next.observed.length,1000,0.5,&next.jobs)) {
        api_destroy(&next); return failure(file,500,"Could not estimate sampled jobs.");
    }
    if (!bound_capture_jobs(&next) || !compact_latest_observations(&next.observed)) {
        api_destroy(&next); return failure(file,500,"Could not prepare the bounded capture workload.");
    }
    next.captured=1;
    api_destroy(state); *state=next;
    return write_state(file,state);
}

static int advisor(ApiState *state, const char *query, FILE *file)
{
    unsigned long pid, priority;
    SimulationResult before, after;
    int ok;
    if (!workload_allowed(state)) return failure(file,422,"Load a bounded workload first.");
    if (!number(query,"pid",0,1,ULONG_MAX,&pid) || !pid || !number(query,"priority",9,0,9,&priority))
        return failure(file,400,"Provide a PID and priority from 0 through 9.");
    simulation_result_init(&before); simulation_result_init(&after);
    ok=advisor_model_priority_change((Job *)state->jobs.data,state->jobs.length,pid,(unsigned int)priority,2,0,&before,&after);
    if (ok) {
        fputs("{\"label\":\"modeled single-job priority change; real OS unchanged\",\"before\":{",file);
        write_metrics(file,&before); fputs("},\"after\":{",file); write_metrics(file,&after); fputs("}}",file);
    }
    simulation_result_destroy(&before); simulation_result_destroy(&after);
    return ok ? 200 : failure(file,422,"PID not in workload or simulation failed.");
}

static int sweep(ApiState *state, const char *query, FILE *file)
{
    const unsigned long long quanta[] = {1,2,4,8,16,32};
    SimulationResult runs[6];
    unsigned long cost;
    size_t i,j;
    if (!workload_allowed(state)) return failure(file,422,"Load a bounded workload first.");
    if (!number(query,"context",0,0,10,&cost)) return failure(file,400,"Context cost must be 0-10 ms.");
    for(i=0;i<6;i++) simulation_result_init(&runs[i]);
    for(i=0;i<6;i++) {
        if (!simulate_round_robin((Job *)state->jobs.data,state->jobs.length,quanta[i],cost,&runs[i])) {
            for(j=0;j<6;j++) simulation_result_destroy(&runs[j]);
            return failure(file,500,"Quantum sweep failed.");
        }
    }
    fputs("{\"points\":[",file);
    for(i=0;i<6;i++) {
        fprintf(file,"%s{\"quantum\":%llu,\"context_switches\":%llu}",i ? "," : "",quanta[i],runs[i].context_switches);
        simulation_result_destroy(&runs[i]);
    }
    fputs("]}",file); return 200;
}

static int export_report(ApiState *state,const char *query,FILE *file,const char **type)
{
    char format[16]="json";
    if (!state->report.count) return failure(file,409,"Run a simulation before exporting.");
    if (query_value(query,"format",format,sizeof(format))<0) return failure(file,400,"Invalid export format.");
    if (strcmp(format,"json")==0) return write_state(file,state);
    if (strcmp(format,"csv")==0) {
        *type="text/csv; charset=utf-8";
        return report_write_csv(file,&state->report) ? 200 : 500;
    }
    if (strcmp(format,"text")==0) {
        size_t i;
        *type="text/plain; charset=utf-8";
        fputs("Single-CPU modeled simulation. Not a recording of the real OS scheduler.\nRead-only: no real process was changed. Bursts are estimates.\n\n",file);
        report_write_text(file,&state->report); report_write_tradeoffs(file,&state->report);
        for(i=0;i<state->report.count;i++) {
            fprintf(file,"\n%s\n",state->report.runs[i].name);
            report_write_gantt(file,&state->report.runs[i].result);
        }
        return ferror(file) ? 500 : 200;
    }
    return failure(file,400,"Use json, csv or text.");
}

/* Dispatch only documented routes; all mutations use POST. O(workload work). */
int api_handle(ApiState *state,const char *method,const char *path,
               const char *query,FILE *file,const char **type)
{
    *type="application/json; charset=utf-8";
    if (strcmp(method,"GET")==0) {
        if (strcmp(path,"/api/health")==0) { fputs("{\"status\":\"ok\",\"read_only\":true}",file); return 200; }
        if (strcmp(path,"/api/state")==0) return write_state(file,state);
        if (strcmp(path,"/api/export")==0) return export_report(state,query,file,type);
        return failure(file,404,"Unknown GET route.");
    }
    if (strcmp(method,"POST")!=0) return failure(file,405,"Use GET or POST.");
    if (strcmp(path,"/api/demo")==0) {
        ApiState next; api_init(&next);
        if (!workload_generate(&next.jobs,12,42)) { api_destroy(&next); return failure(file,500,"Demo allocation failed."); }
        api_destroy(state); *state=next; return write_state(file,state);
    }
    if (strcmp(path,"/api/capture")==0) return capture(state,query,file);
    if (strcmp(path,"/api/simulate")==0) return simulate(state,query,file);
    if (strcmp(path,"/api/advisor")==0) return advisor(state,query,file);
    if (strcmp(path,"/api/sweep")==0) return sweep(state,query,file);
    if (strcmp(path,"/api/recommend")==0) {
        if (!workload_allowed(state)) return failure(file,422,"Load a bounded workload first.");
        state->recommended=adaptive_recommend((Job *)state->jobs.data,state->jobs.length,&state->recommendation);
        return state->recommended ? write_state(file,state) : failure(file,500,"Recommendation failed.");
    }
    if (strcmp(path,"/api/validate")==0) {
        state->validated=adaptive_validate(500,42,ADAPTIVE_OBJECTIVE_WAITING,&state->validation);
        return state->validated ? write_state(file,state) : failure(file,500,"Validation failed.");
    }
    return failure(file,404,"Unknown POST route.");
}
