#include "report.h"

#include <stdlib.h>

const char *algorithm_name(AlgorithmKind kind)
{
    switch (kind) {
    case ALGORITHM_FCFS: return "FCFS";
    case ALGORITHM_SJF: return "SJF";
    case ALGORITHM_SRTF: return "SRTF";
    case ALGORITHM_ROUND_ROBIN: return "Round Robin";
    case ALGORITHM_PRIORITY: return "Priority";
    case ALGORITHM_PRIORITY_AGING: return "Priority with Aging";
    default: return "Unknown";
    }
}

void comparison_report_init(ComparisonReport *report)
{
    if (report != NULL) {
        report->runs = NULL;
        report->count = 0;
    }
}

void comparison_report_destroy(ComparisonReport *report)
{
    size_t index;
    if (report == NULL) {
        return;
    }
    for (index = 0; index < report->count; index++) {
        simulation_result_destroy(&report->runs[index].result);
    }
    free(report->runs);
    comparison_report_init(report);
}

static int run_algorithm(const Job *jobs, size_t count, AlgorithmKind kind,
                         unsigned long long quantum,
                         unsigned long long context_cost,
                         unsigned long long aging_interval,
                         unsigned int aging_amount,
                         SimulationResult *result)
{
    simulation_result_init(result);
    switch (kind) {
    case ALGORITHM_FCFS:
        return simulate_fcfs(jobs, count, context_cost, result);
    case ALGORITHM_SJF:
        return simulate_sjf(jobs, count, 0, context_cost, result);
    case ALGORITHM_SRTF:
        return simulate_sjf(jobs, count, 1, context_cost, result);
    case ALGORITHM_ROUND_ROBIN:
        return simulate_round_robin(jobs, count, quantum, context_cost, result);
    case ALGORITHM_PRIORITY:
        return simulate_priority(jobs, count, 1, 0, 0, context_cost, result);
    case ALGORITHM_PRIORITY_AGING:
        return simulate_priority(jobs, count, 1, aging_interval, aging_amount,
                                 context_cost, result);
    default:
        return 0;
    }
}

int comparison_report_run(const Job *jobs, size_t job_count,
                          const AlgorithmKind *algorithms, size_t algorithm_count,
                          unsigned long long quantum,
                          unsigned long long context_cost,
                          unsigned long long aging_interval,
                          unsigned int aging_amount,
                          ComparisonReport *report)
{
    size_t index;
    int needs_quantum = 0;
    if (jobs == NULL || algorithms == NULL || report == NULL ||
        algorithm_count == 0) {
        return 0;
    }
    for (index = 0; index < algorithm_count; index++) {
        if (algorithms[index] == ALGORITHM_ROUND_ROBIN) {
            needs_quantum = 1;
        }
    }
    if (needs_quantum && quantum == 0) {
        return 0;
    }
    comparison_report_destroy(report);
    report->runs = calloc(algorithm_count, sizeof(*report->runs));
    if (report->runs == NULL) {
        return 0;
    }
    report->count = algorithm_count;
    for (index = 0; index < algorithm_count; index++) {
        report->runs[index].kind = algorithms[index];
        report->runs[index].name = algorithm_name(algorithms[index]);
        if (!run_algorithm(jobs, job_count, algorithms[index], quantum,
                           context_cost, aging_interval, aging_amount,
                           &report->runs[index].result)) {
            comparison_report_destroy(report);
            return 0;
        }
    }
    return 1;
}

const AlgorithmRun *comparison_best_waiting(const ComparisonReport *report)
{
    size_t index;
    const AlgorithmRun *best = NULL;
    if (report == NULL) {
        return NULL;
    }
    for (index = 0; index < report->count; index++) {
        if (best == NULL || report->runs[index].result.average_waiting <
            best->result.average_waiting) {
            best = &report->runs[index];
        }
    }
    return best;
}

const AlgorithmRun *comparison_best_response(const ComparisonReport *report)
{
    size_t index;
    const AlgorithmRun *best = NULL;
    if (report == NULL) {
        return NULL;
    }
    for (index = 0; index < report->count; index++) {
        if (best == NULL || report->runs[index].result.average_response <
            best->result.average_response) {
            best = &report->runs[index];
        }
    }
    return best;
}

int report_write_gantt(FILE *file, const SimulationResult *result)
{
    size_t index;
    if (file == NULL || result == NULL) {
        return 0;
    }
    if (fprintf(file, "Gantt: ") < 0) {
        return 0;
    }
    for (index = 0; index < result->events.length; index++) {
        const GanttEvent *event = dynamic_array_const_at(&result->events, index);
        const char *label = event->kind == EVENT_RUN ? "PID" :
                            event->kind == EVENT_IDLE ? "IDLE" : "CS";
        if (fprintf(file, "[%llu-%llu %s%lu] ", event->start_tick,
                    event->end_tick, label, event->pid) < 0) {
            return 0;
        }
    }
    return fputc('\n', file) != EOF;
}

int report_write_text(FILE *file, const ComparisonReport *report)
{
    size_t index;
    if (file == NULL || report == NULL) {
        return 0;
    }
    if (fprintf(file, "ProcLens Scheduling Comparison\n\n") < 0 ||
        fprintf(file, "Algorithm | Avg Waiting | Avg Turnaround | Avg Response | "
                      "Throughput | Utilization | Fairness | Context Switches | Starved\n") < 0) {
        return 0;
    }
    for (index = 0; index < report->count; index++) {
        const SimulationResult *result = &report->runs[index].result;
        if (fprintf(file, "%s | %.2f | %.2f | %.2f | %.4f | %.4f | %.4f | "
                          "%llu | %zu\n", report->runs[index].name,
                    result->average_waiting, result->average_turnaround,
                    result->average_response, result->throughput,
                    result->utilization, result->fairness,
                    result->context_switches, result->starved_count) < 0) {
            return 0;
        }
    }
    return 1;
}

int report_write_csv(FILE *file, const ComparisonReport *report)
{
    size_t index;
    if (file == NULL || report == NULL ||
        fprintf(file, "algorithm,average_waiting,average_turnaround,average_response,"
                      "throughput,utilization,fairness,context_switches,starved\n") < 0) {
        return 0;
    }
    for (index = 0; index < report->count; index++) {
        const SimulationResult *result = &report->runs[index].result;
        if (fprintf(file, "%s,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%llu,%zu\n",
                    report->runs[index].name, result->average_waiting,
                    result->average_turnaround, result->average_response,
                    result->throughput, result->utilization, result->fairness,
                    result->context_switches, result->starved_count) < 0) {
            return 0;
        }
    }
    return 1;
}

int report_write_json(FILE *file, const ComparisonReport *report)
{
    size_t index;
    if (file == NULL || report == NULL || fputs("{\"algorithms\":[", file) == EOF) {
        return 0;
    }
    for (index = 0; index < report->count; index++) {
        const SimulationResult *result = &report->runs[index].result;
        if (index > 0 && fputc(',', file) == EOF) {
            return 0;
        }
        if (fprintf(file, "{\"name\":\"%s\",\"average_waiting\":%.6f,"
                          "\"average_turnaround\":%.6f,\"average_response\":%.6f,"
                          "\"throughput\":%.6f,\"utilization\":%.6f,"
                          "\"fairness\":%.6f,\"context_switches\":%llu,"
                          "\"starved\":%zu}", report->runs[index].name,
                    result->average_waiting, result->average_turnaround,
                    result->average_response, result->throughput,
                    result->utilization, result->fairness,
                    result->context_switches, result->starved_count) < 0) {
            return 0;
        }
    }
    return fputs("]}\n", file) != EOF;
}

int report_write_tradeoffs(FILE *file, const ComparisonReport *report)
{
    const AlgorithmRun *waiting;
    const AlgorithmRun *response;
    if (file == NULL || report == NULL) {
        return 0;
    }
    waiting = comparison_best_waiting(report);
    response = comparison_best_response(report);
    if (waiting == NULL || response == NULL) {
        return 0;
    }
    return fprintf(file, "Trade-offs:\n"
                         "- %s has the lowest average waiting time in this workload.\n"
                         "- %s has the lowest average response time in this workload.\n"
                         "- SJF generally reduces waiting time, while Round Robin favors responsiveness.\n"
                         "- Priority scheduling respects importance but may starve lower-priority jobs.\n"
                         "- FCFS is simple but can suffer from the convoy effect.\n"
                         "No algorithm is universally best; results depend on the workload.\n",
                   waiting->name, response->name) >= 0;
}

static int export_file(const char *path, const ComparisonReport *report,
                       int (*writer)(FILE *, const ComparisonReport *))
{
    FILE *file;
    int result;
    if (path == NULL || report == NULL || writer == NULL) {
        return 0;
    }
    file = fopen(path, "w");
    if (file == NULL) {
        return 0;
    }
    result = writer(file, report);
    if (fclose(file) != 0) {
        result = 0;
    }
    return result;
}

int report_export_text(const char *path, const ComparisonReport *report)
{
    return export_file(path, report, report_write_text);
}

int report_export_csv(const char *path, const ComparisonReport *report)
{
    return export_file(path, report, report_write_csv);
}

int report_export_json(const char *path, const ComparisonReport *report)
{
    return export_file(path, report, report_write_json);
}

