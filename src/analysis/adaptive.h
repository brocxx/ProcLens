#ifndef PROCLENS_ADAPTIVE_H
#define PROCLENS_ADAPTIVE_H

#include <stddef.h>
#include <stdio.h>

#include "../sim/simulation.h"
#include "../analysis/report.h"

typedef struct {
    size_t job_count;
    double mean_burst;
    double burst_variance;
    double short_job_share;
    double long_job_share;
    unsigned long long arrival_spread;
    unsigned int priority_spread;
    size_t cpu_heavy_count;
} WorkloadFeatures;

typedef struct {
    AlgorithmKind policy;
    WorkloadFeatures features;
    char explanation[256];
} AdaptiveRecommendation;

typedef enum {
    ADAPTIVE_OBJECTIVE_RESPONSE,
    ADAPTIVE_OBJECTIVE_WAITING
} AdaptiveObjective;

typedef struct {
    size_t runs;
    size_t matches;
    double accuracy;
} AdaptiveValidation;

int adaptive_analyze(const Job *jobs, size_t count, WorkloadFeatures *features);
int adaptive_recommend(const Job *jobs, size_t count,
                       AdaptiveRecommendation *recommendation);
int adaptive_run(const Job *jobs, size_t count,
                 unsigned long long quantum,
                 unsigned long long context_cost,
                 unsigned long long aging_interval,
                 unsigned int aging_amount,
                 AdaptiveRecommendation *recommendation,
                 SimulationResult *result);
int adaptive_validate(size_t workload_count, unsigned long seed,
                      AdaptiveObjective objective,
                      AdaptiveValidation *validation);
int adaptive_write_report(FILE *file, const AdaptiveRecommendation *recommendation,
                          const AdaptiveValidation *validation);

#endif

