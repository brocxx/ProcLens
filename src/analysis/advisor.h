#ifndef PROCLENS_ADVISOR_H
#define PROCLENS_ADVISOR_H

#include <stddef.h>
#include <stdio.h>

#include "groups.h"
#include "../sim/simulation.h"

typedef struct {
    unsigned long root_pid;
    unsigned long long cpu_time_ms;
    unsigned long long memory_kb;
    size_t process_count;
    int cpu_hog;
    int memory_hog;
} AdvisorGroup;

typedef struct {
    AdvisorGroup *groups;
    size_t count;
} AdvisorReport;

void advisor_report_init(AdvisorReport *report);
void advisor_report_destroy(AdvisorReport *report);
int advisor_analyze(const ProcessGroupSummary *groups, size_t count,
                    AdvisorReport *report);
int advisor_write(FILE *file, const AdvisorReport *report);
int advisor_model_priority_change(const Job *jobs, size_t count,
                                  unsigned long target_pid,
                                  unsigned int modeled_priority,
                                  unsigned long long quantum,
                                  unsigned long long context_cost,
                                  SimulationResult *before,
                                  SimulationResult *after);

#endif

