#ifndef PROCLENS_GROUPS_H
#define PROCLENS_GROUPS_H

#include <stddef.h>

#include "../sim/model.h"
#include "../ds/structures.h"

typedef struct {
    unsigned long root_pid;
    unsigned long long cpu_time_ms;
    unsigned long long memory_kb;
    size_t process_count;
} ProcessGroupSummary;

int process_group_summarize(const ProcessRecord *records, size_t count,
                            DynamicArray *groups);

#endif

