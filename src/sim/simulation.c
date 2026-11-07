#include "simulation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *text)
{
    size_t length;
    char *copy;
    if (text == NULL) {
        return NULL;
    }
    length = strlen(text) + 1;
    copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, text, length);
    }
    return copy;
}

static void destroy_decision(void *value) { free(value); }

void simulation_result_init(SimulationResult *result)
{
    if (result == NULL) {
        return;
    }
    dynamic_array_init(&result->events, sizeof(GanttEvent));
    dynamic_array_init(&result->metrics, sizeof(JobMetrics));
    linked_list_init(&result->decisions);
    result->makespan = 0;
    result->context_switches = 0;
    result->average_waiting = 0.0;
    result->average_turnaround = 0.0;
    result->average_response = 0.0;
    result->throughput = 0.0;
    result->utilization = 0.0;
    result->fairness = 0.0;
    result->max_waiting = 0;
    result->starved_count = 0;
}

void simulation_result_destroy(SimulationResult *result)
{
    if (result == NULL) {
        return;
    }
    dynamic_array_destroy(&result->events);
    dynamic_array_destroy(&result->metrics);
    linked_list_destroy(&result->decisions, destroy_decision);
    simulation_result_init(result);
}

static int job_order(const void *left, const void *right)
{
    const Job *a = left;
    const Job *b = right;
    if (a->arrival_tick < b->arrival_tick) {
        return -1;
    }
    if (a->arrival_tick > b->arrival_tick) {
        return 1;
    }
    if (a->pid < b->pid) {
        return -1;
    }
    if (a->pid > b->pid) {
        return 1;
    }
    return 0;
}

static int add_event(SimulationResult *result, unsigned long pid,
                     unsigned long long start, unsigned long long end,
                     EventKind kind)
{
    GanttEvent event;
    if (end <= start) {
        return 1;
    }
    if (result->events.length > 0) {
        GanttEvent *last = dynamic_array_at(&result->events,
                                            result->events.length - 1);
        if (last->kind == kind && last->pid == pid && last->end_tick == start) {
            last->end_tick = end;
            return 1;
        }
    }
    event.pid = pid;
    event.start_tick = start;
    event.end_tick = end;
    event.kind = kind;
    return dynamic_array_push(&result->events, &event);
}

static int add_decision(SimulationResult *result, const Job *job,
                        unsigned long long time)
{
    char message[160];
    char *copy;
    int written = snprintf(message, sizeof(message),
                           "Time %llu: selected PID %lu using FCFS arrival order.",
                           time, job->pid);
    if (written < 0 || (size_t)written >= sizeof(message)) {
        return 0;
    }
    copy = copy_string(message);
    if (copy == NULL || !linked_list_push_back(&result->decisions, copy)) {
        free(copy);
        return 0;
    }
    return 1;
}

static int calculate_metrics(const Job *jobs, size_t count,
                             SimulationResult *result)
{
    size_t index;
    double waiting_sum = 0.0;
    double turnaround_sum = 0.0;
    double response_sum = 0.0;
    double fairness_sum = 0.0;
    double fairness_square_sum = 0.0;
    for (index = 0; index < count; index++) {
        const Job *job = &jobs[index];
        const GanttEvent *first = NULL;
        const GanttEvent *last = NULL;
        size_t event_index;
        JobMetrics metric;
        for (event_index = 0; event_index < result->events.length; event_index++) {
            const GanttEvent *event = dynamic_array_const_at(&result->events,
                                                             event_index);
            if (event->kind == EVENT_RUN && event->pid == job->pid) {
                if (first == NULL) {
                    first = event;
                }
                last = event;
            }
        }
        if (first == NULL || last == NULL) {
            return 0;
        }
        metric.pid = job->pid;
        metric.completion_tick = last->end_tick;
        metric.turnaround_tick = metric.completion_tick - job->arrival_tick;
        metric.waiting_tick = metric.turnaround_tick - job->burst_tick;
        metric.response_tick = first->start_tick - job->arrival_tick;
        if (!dynamic_array_push(&result->metrics, &metric)) {
            return 0;
        }
        waiting_sum += (double)metric.waiting_tick;
        turnaround_sum += (double)metric.turnaround_tick;
        response_sum += (double)metric.response_tick;
        if (metric.waiting_tick > result->max_waiting) {
            result->max_waiting = metric.waiting_tick;
        }
        {
            double share = (double)job->burst_tick /
                           (double)metric.turnaround_tick;
            fairness_sum += share;
            fairness_square_sum += share * share;
        }
    }
    if (count > 0) {
        result->average_waiting = waiting_sum / (double)count;
        result->average_turnaround = turnaround_sum / (double)count;
        result->average_response = response_sum / (double)count;
        result->fairness = fairness_sum * fairness_sum /
                           ((double)count * fairness_square_sum);
        result->throughput = (double)count / (double)result->makespan;
    }
    if (count > 0) {
        double average_burst = 0.0;
        for (index = 0; index < count; index++) {
            average_burst += (double)jobs[index].burst_tick;
        }
        average_burst /= (double)count;
        for (index = 0; index < result->metrics.length; index++) {
            const JobMetrics *metric = dynamic_array_const_at(&result->metrics,
                                                               index);
            if ((double)metric->waiting_tick > 5.0 * average_burst) {
                result->starved_count++;
            }
        }
    }
    return 1;
}

int simulate_fcfs(const Job *jobs, size_t count, unsigned long long context_cost,
                  SimulationResult *result)
{
    Job *ordered;
    size_t index;
    unsigned long previous_pid = 0;
    unsigned long long time = 0;
    unsigned long long busy = 0;
    if (jobs == NULL || result == NULL) {
        return 0;
    }
    simulation_result_destroy(result);
    ordered = malloc(count * sizeof(*ordered));
    if (count > 0 && ordered == NULL) {
        return 0;
    }
    if (count > 0) {
        memcpy(ordered, jobs, count * sizeof(*ordered));
        qsort(ordered, count, sizeof(*ordered), job_order);
    }
    for (index = 0; index < count; index++) {
        const Job *job = &ordered[index];
        if (time < job->arrival_tick) {
            if (!add_event(result, 0, time, job->arrival_tick, EVENT_IDLE)) {
                free(ordered);
                return 0;
            }
            time = job->arrival_tick;
        }
        if (previous_pid != 0 && previous_pid != job->pid) {
            if (!add_event(result, 0, time, time + context_cost,
                           EVENT_CONTEXT_SWITCH)) {
                free(ordered);
                return 0;
            }
            time += context_cost;
            result->context_switches++;
        }
        if (!add_decision(result, job, time) ||
            !add_event(result, job->pid, time, time + job->burst_tick,
                       EVENT_RUN)) {
            free(ordered);
            return 0;
        }
        time += job->burst_tick;
        busy += job->burst_tick;
        previous_pid = job->pid;
    }
    free(ordered);
    result->makespan = time;
    result->utilization = time == 0 ? 0.0 : (double)busy / (double)time;
    return calculate_metrics(jobs, count, result);
}

typedef enum {
    POLICY_SJF,
    POLICY_SRTF,
    POLICY_ROUND_ROBIN,
    POLICY_PRIORITY,
    POLICY_PRIORITY_AGING
} TickPolicy;

static int better_job(const Job *jobs, const unsigned long long *remaining,
                      size_t candidate, size_t current, TickPolicy policy,
                      unsigned long long time,
                      unsigned long long aging_interval,
                      unsigned int aging_amount)
{
    unsigned long long candidate_key;
    unsigned long long current_key;
    unsigned long long candidate_wait;
    unsigned long long current_wait;
    if (current == (size_t)-1) {
        return 1;
    }
    if (policy == POLICY_SJF || policy == POLICY_SRTF) {
        candidate_key = policy == POLICY_SJF ? jobs[candidate].burst_tick :
                                               remaining[candidate];
        current_key = policy == POLICY_SJF ? jobs[current].burst_tick :
                                             remaining[current];
    } else {
        candidate_wait = time > jobs[candidate].arrival_tick ?
                         time - jobs[candidate].arrival_tick : 0;
        current_wait = time > jobs[current].arrival_tick ?
                       time - jobs[current].arrival_tick : 0;
        candidate_key = jobs[candidate].priority;
        current_key = jobs[current].priority;
        if (aging_interval > 0 && aging_amount > 0) {
            candidate_key = candidate_key >
                            (candidate_wait / aging_interval) * aging_amount ?
                            candidate_key - (candidate_wait / aging_interval) * aging_amount : 0;
            current_key = current_key >
                          (current_wait / aging_interval) * aging_amount ?
                          current_key - (current_wait / aging_interval) * aging_amount : 0;
        }
    }
    if (candidate_key != current_key) {
        return candidate_key < current_key;
    }
    if (jobs[candidate].arrival_tick != jobs[current].arrival_tick) {
        return jobs[candidate].arrival_tick < jobs[current].arrival_tick;
    }
    return jobs[candidate].pid < jobs[current].pid;
}

static size_t choose_ready_job(const Job *jobs, const unsigned long long *remaining,
                               size_t count, unsigned long long time,
                               TickPolicy policy, unsigned long long aging_interval,
                               unsigned int aging_amount)
{
    size_t index;
    size_t selected = (size_t)-1;
    for (index = 0; index < count; index++) {
        if (remaining[index] == 0 || jobs[index].arrival_tick > time) {
            continue;
        }
        if (better_job(jobs, remaining, index, selected, policy, time,
                       aging_interval, aging_amount)) {
            selected = index;
        }
    }
    return selected;
}

static int add_algorithm_decision(SimulationResult *result, const Job *job,
                                  unsigned long long time, const char *name)
{
    char buffer[192];
    char *message;
    int written = snprintf(buffer, sizeof(buffer),
                           "Time %llu: selected PID %lu using %s.",
                           time, job->pid, name);
    if (written < 0 || (size_t)written >= sizeof(buffer)) {
        return 0;
    }
    message = copy_string(buffer);
    if (message == NULL || !linked_list_push_back(&result->decisions, message)) {
        free(message);
        return 0;
    }
    return 1;
}

static int simulate_tick_policy(const Job *jobs, size_t count, TickPolicy policy,
                                int preemptive,
                                unsigned long long quantum,
                                unsigned long long aging_interval,
                                unsigned int aging_amount,
                                unsigned long long context_cost,
                                SimulationResult *result)
{
    unsigned long long *remaining;
    unsigned long long *first_start;
    size_t *rr_indexes;
    unsigned char *enqueued;
    CircularQueue ring;
    size_t index;
    size_t current = (size_t)-1;
    size_t rr_quantum = 0;
    size_t completed = 0;
    unsigned long previous_pid = 0;
    unsigned long long time = 0;
    unsigned long long busy = 0;
    int ring_ready = policy == POLICY_ROUND_ROBIN;
    const char *name = policy == POLICY_SJF ? "SJF" :
                       policy == POLICY_SRTF ? "SRTF" :
                       policy == POLICY_ROUND_ROBIN ? "Round Robin" :
                       "Priority";
    if (jobs == NULL || result == NULL ||
        (policy == POLICY_ROUND_ROBIN && quantum == 0)) {
        return 0;
    }
    simulation_result_destroy(result);
    remaining = calloc(count, sizeof(*remaining));
    first_start = malloc(count * sizeof(*first_start));
    rr_indexes = malloc(count * sizeof(*rr_indexes));
    enqueued = calloc(count, sizeof(*enqueued));
    if ((count > 0 && (remaining == NULL || first_start == NULL ||
                       rr_indexes == NULL || enqueued == NULL)) ||
        (ring_ready && !circular_queue_init(&ring, count + 1))) {
        free(remaining);
        free(first_start);
        free(rr_indexes);
        free(enqueued);
        return 0;
    }
    for (index = 0; index < count; index++) {
        remaining[index] = jobs[index].burst_tick;
        first_start[index] = (unsigned long long)-1;
        rr_indexes[index] = index;
    }
    while (completed < count) {
        if (ring_ready) {
            for (index = 0; index < count; index++) {
                if (jobs[index].arrival_tick <= time && remaining[index] > 0 &&
                    !enqueued[index] && current != index) {
                    if (!circular_queue_push(&ring, &rr_indexes[index])) {
                        goto failure;
                }
                    enqueued[index] = 1;
                }
            }
        }
        if (current == (size_t)-1) {
            if (ring_ready) {
                size_t *next = circular_queue_pop(&ring);
                current = next == NULL ? (size_t)-1 : *next;
                if (current != (size_t)-1) {
                    enqueued[current] = 0;
                }
            } else {
                current = choose_ready_job(jobs, remaining, count, time, policy,
                                            aging_interval, aging_amount);
            }
            if (current == (size_t)-1) {
                unsigned long long next_arrival = (unsigned long long)-1;
                for (index = 0; index < count; index++) {
                    if (remaining[index] > 0 && jobs[index].arrival_tick < next_arrival) {
                        next_arrival = jobs[index].arrival_tick;
                    }
                }
                if (next_arrival == (unsigned long long)-1) {
                    break;
                }
                if (!add_event(result, 0, time, next_arrival, EVENT_IDLE)) {
                    goto failure;
                }
                time = next_arrival;
                continue;
            }
            if (previous_pid != 0 && previous_pid != jobs[current].pid) {
                if (!add_event(result, 0, time, time + context_cost,
                               EVENT_CONTEXT_SWITCH)) {
                    goto failure;
                }
                time += context_cost;
                result->context_switches++;
            }
            if (!add_algorithm_decision(result, &jobs[current], time, name)) {
                goto failure;
            }
            rr_quantum = 0;
        }
        if (first_start[current] == (unsigned long long)-1) {
            first_start[current] = time;
        }
        if (!add_event(result, jobs[current].pid, time, time + 1, EVENT_RUN)) {
            goto failure;
        }
        remaining[current]--;
        busy++;
        time++;
        rr_quantum++;
        previous_pid = jobs[current].pid;
        if (remaining[current] == 0) {
            completed++;
            current = (size_t)-1;
        } else if (policy == POLICY_ROUND_ROBIN && rr_quantum >= quantum) {
            if (!circular_queue_push(&ring, &rr_indexes[current])) {
                goto failure;
            }
            enqueued[current] = 1;
            current = (size_t)-1;
        } else if (preemptive && policy != POLICY_ROUND_ROBIN) {
            size_t next = choose_ready_job(jobs, remaining, count, time, policy,
                                            aging_interval, aging_amount);
            if (next != (size_t)-1 && next != current) {
                current = (size_t)-1;
            }
        }
    }
    free(remaining);
    free(first_start);
    free(rr_indexes);
    free(enqueued);
    if (ring_ready) {
        circular_queue_destroy(&ring);
    }
    result->makespan = time;
    result->utilization = time == 0 ? 0.0 : (double)busy / (double)time;
    return calculate_metrics(jobs, count, result);

failure:
    free(remaining);
    free(first_start);
    free(rr_indexes);
    free(enqueued);
    if (ring_ready) {
        circular_queue_destroy(&ring);
    }
    simulation_result_destroy(result);
    return 0;
}

int simulate_sjf(const Job *jobs, size_t count, int preemptive,
                 unsigned long long context_cost, SimulationResult *result)
{
    return simulate_tick_policy(jobs, count, preemptive ? POLICY_SRTF : POLICY_SJF,
                                preemptive,
                                0, 0, 0, context_cost, result);
}

int simulate_round_robin(const Job *jobs, size_t count, unsigned long long quantum,
                         unsigned long long context_cost, SimulationResult *result)
{
    return simulate_tick_policy(jobs, count, POLICY_ROUND_ROBIN, 1, quantum, 0, 0,
                                context_cost, result);
}

int simulate_priority(const Job *jobs, size_t count, int preemptive,
                      unsigned long long aging_interval, unsigned int aging_amount,
                      unsigned long long context_cost, SimulationResult *result)
{
    TickPolicy policy = aging_amount == 0 ? POLICY_PRIORITY : POLICY_PRIORITY_AGING;
    if (!preemptive) {
        policy = aging_amount == 0 ? POLICY_PRIORITY : POLICY_PRIORITY_AGING;
    }
    return simulate_tick_policy(jobs, count, policy, preemptive, 0, aging_interval,
                                aging_amount, context_cost, result);
}

int simulation_validate(const Job *jobs, size_t count,
                        const SimulationResult *result)
{
    size_t index;
    unsigned long long previous_end = 0;
    if (jobs == NULL || result == NULL || result->metrics.length != count) {
        return 0;
    }
    for (index = 0; index < result->events.length; index++) {
        const GanttEvent *event = dynamic_array_const_at(&result->events, index);
        if (event->end_tick <= event->start_tick || event->start_tick < previous_end) {
            return 0;
        }
        previous_end = event->end_tick;
    }
    for (index = 0; index < count; index++) {
        const Job *job = &jobs[index];
        const JobMetrics *metric = dynamic_array_const_at(&result->metrics, index);
        unsigned long long run_total = 0;
        size_t event_index;
        for (event_index = 0; event_index < result->events.length; event_index++) {
            const GanttEvent *event = dynamic_array_const_at(&result->events,
                                                             event_index);
            if (event->kind == EVENT_RUN && event->pid == job->pid) {
                if (event->start_tick < job->arrival_tick) {
                    return 0;
                }
                run_total += event->end_tick - event->start_tick;
            }
        }
        if (run_total != job->burst_tick ||
            metric->turnaround_tick != metric->completion_tick - job->arrival_tick ||
            metric->waiting_tick != metric->turnaround_tick - job->burst_tick) {
            return 0;
        }
    }
    return result->utilization >= 0.0 && result->utilization <= 1.0;
}

static unsigned long next_random(unsigned long *state)
{
    *state = *state * 1103515245UL + 12345UL;
    return (*state / 65536UL) % 32768UL;
}

int workload_generate(DynamicArray *jobs, size_t count, unsigned long seed)
{
    size_t index;
    unsigned long time = 0;
    if (jobs == NULL || !dynamic_array_init(jobs, sizeof(Job))) {
        return 0;
    }
    for (index = 0; index < count; index++) {
        Job job;
        job_init(&job);
        job.pid = (unsigned long)(index + 1);
        job.arrival_tick = time;
        job.burst_tick = 1 + next_random(&seed) % 20;
        job.remaining_tick = job.burst_tick;
        job.priority = next_random(&seed) % 10;
        if (!dynamic_array_push(jobs, &job)) {
            dynamic_array_destroy(jobs);
            return 0;
        }
        time += next_random(&seed) % 4;
    }
    return 1;
}

int workload_load_csv(const char *path, DynamicArray *jobs)
{
    FILE *file;
    char line[512];
    if (path == NULL || jobs == NULL || !dynamic_array_init(jobs, sizeof(Job))) {
        return 0;
    }
    file = fopen(path, "r");
    if (file == NULL) {
        return 0;
    }
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return 0;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        Job job;
        unsigned long long arrival;
        unsigned long long burst;
        unsigned int priority;
        unsigned long pid;
        if (sscanf(line, "%lu,%llu,%llu,%u", &pid, &arrival, &burst,
                   &priority) != 4) {
            continue;
        }
        job_init(&job);
        job.pid = pid;
        job.arrival_tick = arrival;
        job.burst_tick = burst == 0 ? 1 : burst;
        job.remaining_tick = job.burst_tick;
        job.priority = priority > 9 ? 9 : priority;
        if (!dynamic_array_push(jobs, &job)) {
            fclose(file);
            dynamic_array_destroy(jobs);
            return 0;
        }
    }
    fclose(file);
    return 1;
}

