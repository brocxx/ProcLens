#ifndef PROCLENS_REPORT_H
#define PROCLENS_REPORT_H

#include <stddef.h>
#include <stdio.h>

#include "../sim/simulation.h"

typedef enum {
    ALGORITHM_FCFS,
    ALGORITHM_SJF,
    ALGORITHM_SRTF,
    ALGORITHM_ROUND_ROBIN,
    ALGORITHM_PRIORITY,
    ALGORITHM_PRIORITY_AGING
} AlgorithmKind;

typedef struct {
    AlgorithmKind kind;
    const char *name;
    SimulationResult result;
} AlgorithmRun;

typedef struct {
    AlgorithmRun *runs;
    size_t count;
} ComparisonReport;

const char *algorithm_name(AlgorithmKind kind);
void comparison_report_init(ComparisonReport *report);
void comparison_report_destroy(ComparisonReport *report);
int comparison_report_run(const Job *jobs, size_t job_count,
                          const AlgorithmKind *algorithms, size_t algorithm_count,
                          unsigned long long quantum,
                          unsigned long long context_cost,
                          unsigned long long aging_interval,
                          unsigned int aging_amount,
                          ComparisonReport *report);
const AlgorithmRun *comparison_best_waiting(const ComparisonReport *report);
const AlgorithmRun *comparison_best_response(const ComparisonReport *report);
int report_write_text(FILE *file, const ComparisonReport *report);
int report_write_csv(FILE *file, const ComparisonReport *report);
int report_write_json(FILE *file, const ComparisonReport *report);
int report_write_gantt(FILE *file, const SimulationResult *result);
int report_write_tradeoffs(FILE *file, const ComparisonReport *report);
int report_export_text(const char *path, const ComparisonReport *report);
int report_export_csv(const char *path, const ComparisonReport *report);
int report_export_json(const char *path, const ComparisonReport *report);

#endif

