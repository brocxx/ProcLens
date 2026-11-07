#ifndef PROCLENS_API_H
#define PROCLENS_API_H

#include <stdio.h>
#include "../analysis/adaptive.h"
#include "../analysis/groups.h"

/* Owns all observations, jobs and simulation results for one local session. */
typedef struct {
    DynamicArray observed;
    DynamicArray jobs;
    ComparisonReport report;
    AdaptiveRecommendation recommendation;
    AdaptiveValidation validation;
    int recommended;
    int validated;
    int captured;
    size_t omitted_jobs;
} ApiState;

void api_init(ApiState *state);
void api_destroy(ApiState *state);
int api_handle(ApiState *state, const char *method, const char *path,
               const char *query, FILE *output, const char **content_type);

#endif
