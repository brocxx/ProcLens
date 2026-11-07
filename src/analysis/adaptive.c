#include "adaptive.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static double square(double value) { return value * value; }

int adaptive_analyze(const Job *jobs, size_t count, WorkloadFeatures *features)
{
    size_t index;
    double sum = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
    unsigned int minimum_priority = 0;
    unsigned int maximum_priority = 0;
    unsigned long long minimum_arrival = 0;
    unsigned long long maximum_arrival = 0;
    if (jobs == NULL || features == NULL || count == 0) {
        return 0;
    }
    memset(features, 0, sizeof(*features));
    features->job_count = count;
    minimum = (double)jobs[0].burst_tick;
    maximum = minimum;
    minimum_priority = jobs[0].priority;
    maximum_priority = jobs[0].priority;
    minimum_arrival = jobs[0].arrival_tick;
    maximum_arrival = minimum_arrival;
    for (index = 0; index < count; index++) {
        double burst = (double)jobs[index].burst_tick;
        sum += burst;
        if (burst < minimum) minimum = burst;
        if (burst > maximum) maximum = burst;
        if (jobs[index].priority < minimum_priority) minimum_priority = jobs[index].priority;
        if (jobs[index].priority > maximum_priority) maximum_priority = jobs[index].priority;
        if (jobs[index].arrival_tick < minimum_arrival) minimum_arrival = jobs[index].arrival_tick;
        if (jobs[index].arrival_tick > maximum_arrival) maximum_arrival = jobs[index].arrival_tick;
    }
    features->mean_burst = sum / (double)count;
    for (index = 0; index < count; index++) {
        double burst = (double)jobs[index].burst_tick;
        features->burst_variance += square(burst - features->mean_burst);
        if (burst <= features->mean_burst * 0.5) features->short_job_share += 1.0;
        if (burst >= features->mean_burst * 2.0) features->long_job_share += 1.0;
        if (burst >= features->mean_burst * 2.0) features->cpu_heavy_count++;
    }
    features->burst_variance /= (double)count;
    features->short_job_share /= (double)count;
    features->long_job_share /= (double)count;
    features->arrival_spread = maximum_arrival - minimum_arrival;
    features->priority_spread = maximum_priority - minimum_priority;
    return 1;
}

int adaptive_recommend(const Job *jobs, size_t count,
                       AdaptiveRecommendation *recommendation)
{
    WorkloadFeatures *features;
    if (recommendation == NULL) return 0;
    features = &recommendation->features;
    if (!adaptive_analyze(jobs, count, features)) return 0;
    recommendation->policy = ALGORITHM_ROUND_ROBIN;
    if (features->priority_spread >= 6 && features->long_job_share < 0.5) {
        recommendation->policy = ALGORITHM_PRIORITY_AGING;
        snprintf(recommendation->explanation, sizeof(recommendation->explanation),
                 "Priority with aging was selected because priority spread is %u and aging limits starvation.",
                 features->priority_spread);
    } else if (features->short_job_share >= 0.5) {
        recommendation->policy = ALGORITHM_ROUND_ROBIN;
        snprintf(recommendation->explanation, sizeof(recommendation->explanation),
                 "Round Robin was selected because %.0f%% of jobs are short and response time is emphasized.",
                 features->short_job_share * 100.0);
    } else if (features->burst_variance > square(features->mean_burst)) {
        recommendation->policy = ALGORITHM_SRTF;
        snprintf(recommendation->explanation, sizeof(recommendation->explanation),
                 "SRTF was selected because burst-time variation is high.");
    } else if (features->arrival_spread == 0) {
        recommendation->policy = ALGORITHM_SJF;
        snprintf(recommendation->explanation, sizeof(recommendation->explanation),
                 "SJF was selected because jobs arrive together and burst variation is manageable.");
    } else {
        recommendation->policy = ALGORITHM_FCFS;
        snprintf(recommendation->explanation, sizeof(recommendation->explanation),
                 "FCFS was selected as a predictable baseline for this mixed workload.");
    }
    return 1;
}

int adaptive_run(const Job *jobs, size_t count, unsigned long long quantum,
                 unsigned long long context_cost, unsigned long long aging_interval,
                 unsigned int aging_amount, AdaptiveRecommendation *recommendation,
                 SimulationResult *result)
{
    if (!adaptive_recommend(jobs, count, recommendation)) return 0;
    switch (recommendation->policy) {
    case ALGORITHM_ROUND_ROBIN:
        return simulate_round_robin(jobs, count, quantum, context_cost, result);
    case ALGORITHM_PRIORITY_AGING:
        return simulate_priority(jobs, count, 1, aging_interval, aging_amount,
                                 context_cost, result);
    case ALGORITHM_SRTF:
        return simulate_sjf(jobs, count, 1, context_cost, result);
    case ALGORITHM_SJF:
        return simulate_sjf(jobs, count, 0, context_cost, result);
    default:
        return simulate_fcfs(jobs, count, context_cost, result);
    }
}

static double objective_value(const SimulationResult *result,
                              AdaptiveObjective objective)
{
    return objective == ADAPTIVE_OBJECTIVE_RESPONSE ? result->average_response :
                                                       result->average_waiting;
}

int adaptive_validate(size_t workload_count, unsigned long seed,
                      AdaptiveObjective objective,
                      AdaptiveValidation *validation)
{
    size_t run;
    if (validation == NULL || workload_count == 0) return 0;
    validation->runs = 0;
    validation->matches = 0;
    validation->accuracy = 0.0;
    for (run = 0; run < workload_count; run++) {
        DynamicArray generated;
        Job *jobs;
        size_t job_count = 5 + (run % 11);
        AdaptiveRecommendation recommendation;
        SimulationResult adaptive_result;
        SimulationResult candidate_results[4];
        AlgorithmKind policies[4] = {ALGORITHM_FCFS, ALGORITHM_SJF,
                                     ALGORITHM_ROUND_ROBIN, ALGORITHM_PRIORITY_AGING};
        size_t index;
        size_t best = 0;
        if (!workload_generate(&generated, job_count, seed + (unsigned long)run)) return 0;
        jobs = (Job *)generated.data;
        simulation_result_init(&adaptive_result);
        if (!adaptive_run(jobs, generated.length, 2, 1, 4, 1,
                          &recommendation, &adaptive_result)) {
            dynamic_array_destroy(&generated);
            return 0;
        }
        for (index = 0; index < 4; index++) simulation_result_init(&candidate_results[index]);
        if (!simulate_fcfs(jobs, generated.length, 1, &candidate_results[0]) ||
            !simulate_sjf(jobs, generated.length, 0, 1, &candidate_results[1]) ||
            !simulate_round_robin(jobs, generated.length, 2, 1, &candidate_results[2]) ||
            !simulate_priority(jobs, generated.length, 1, 4, 1, 1, &candidate_results[3])) {
            simulation_result_destroy(&adaptive_result);
            for (index = 0; index < 4; index++) simulation_result_destroy(&candidate_results[index]);
            dynamic_array_destroy(&generated);
            return 0;
        }
        for (index = 1; index < 4; index++) {
            if (objective_value(&candidate_results[index], objective) <
                objective_value(&candidate_results[best], objective)) best = index;
        }
        validation->runs++;
        if (policies[best] == recommendation.policy) validation->matches++;
        simulation_result_destroy(&adaptive_result);
        for (index = 0; index < 4; index++) simulation_result_destroy(&candidate_results[index]);
        dynamic_array_destroy(&generated);
    }
    validation->accuracy = (double)validation->matches / (double)validation->runs;
    return 1;
}

int adaptive_write_report(FILE *file, const AdaptiveRecommendation *recommendation,
                          const AdaptiveValidation *validation)
{
    if (file == NULL || recommendation == NULL || validation == NULL) return 0;
    return fprintf(file, "Adaptive policy: %s\nReason: %s\nValidation: %zu/%zu "
                         "matches (%.2f%%).\n", algorithm_name(recommendation->policy),
                   recommendation->explanation, validation->matches,
                   validation->runs, validation->accuracy * 100.0) >= 0;
}

