#include "advisor.h"

#include <stdlib.h>

void advisor_report_init(AdvisorReport *report)
{
    if (report != NULL) {
        report->groups = NULL;
        report->count = 0;
    }
}

void advisor_report_destroy(AdvisorReport *report)
{
    if (report != NULL) {
        free(report->groups);
        advisor_report_init(report);
    }
}

int advisor_analyze(const ProcessGroupSummary *groups, size_t count,
                    AdvisorReport *report)
{
    size_t index;
    unsigned long long total_cpu = 0;
    unsigned long long total_memory = 0;
    if (groups == NULL || report == NULL || count == 0) return 0;
    advisor_report_destroy(report);
    report->groups = calloc(count, sizeof(*report->groups));
    if (report->groups == NULL) return 0;
    report->count = count;
    for (index = 0; index < count; index++) {
        total_cpu += groups[index].cpu_time_ms;
        total_memory += groups[index].memory_kb;
        report->groups[index].root_pid = groups[index].root_pid;
        report->groups[index].cpu_time_ms = groups[index].cpu_time_ms;
        report->groups[index].memory_kb = groups[index].memory_kb;
        report->groups[index].process_count = groups[index].process_count;
    }
    for (index = 0; index < count; index++) {
        report->groups[index].cpu_hog = total_cpu > 0 &&
            report->groups[index].cpu_time_ms * 100 >= total_cpu * 50;
        report->groups[index].memory_hog = total_memory > 0 &&
            report->groups[index].memory_kb * 100 >= total_memory * 50;
    }
    return 1;
}

int advisor_write(FILE *file, const AdvisorReport *report)
{
    size_t index;
    if (file == NULL || report == NULL) return 0;
    if (fprintf(file, "Modeled process-group advisor (read-only)\n") < 0) return 0;
    for (index = 0; index < report->count; index++) {
        const AdvisorGroup *group = &report->groups[index];
        if (fprintf(file, "Root PID %lu: CPU=%llu ms, memory=%llu KiB, "
                          "processes=%zu, CPU hog=%s, memory hog=%s\n",
                    group->root_pid, group->cpu_time_ms, group->memory_kb,
                    group->process_count, group->cpu_hog ? "yes" : "no",
                    group->memory_hog ? "yes" : "no") < 0) return 0;
    }
    return 1;
}

int advisor_model_priority_change(const Job *jobs, size_t count,
                                  unsigned long target_pid,
                                  unsigned int modeled_priority,
                                  unsigned long long quantum,
                                  unsigned long long context_cost,
                                  SimulationResult *before,
                                  SimulationResult *after)
{
    Job *modeled;
    size_t index;
    int found = 0;
    if (jobs == NULL || before == NULL || after == NULL || quantum == 0) return 0;
    modeled = malloc(count * sizeof(*modeled));
    if (count > 0 && modeled == NULL) return 0;
    for (index = 0; index < count; index++) {
        modeled[index] = jobs[index];
        if (modeled[index].pid == target_pid) {
            modeled[index].priority = modeled_priority > 9 ? 9 : modeled_priority;
            found = 1;
        }
    }
    if (!found || !simulate_priority(jobs, count, 1, 0, 0, context_cost, before) ||
        !simulate_priority(modeled, count, 1, 0, 0, context_cost, after)) {
        free(modeled);
        return 0;
    }
    free(modeled);
    return 1;
}

