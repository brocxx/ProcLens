#include "groups.h"

#include <stddef.h>

static unsigned long find_root(const ProcessRecord *records, size_t count,
                               unsigned long pid)
{
    size_t steps = 0;
    while (steps++ <= count) {
        size_t index;
        int found = 0;
        for (index = 0; index < count; index++) {
            if (records[index].pid == pid) {
                found = 1;
                if (records[index].ppid == 0 || records[index].ppid == pid) {
                    return records[index].pid;
                }
                pid = records[index].ppid;
                break;
            }
        }
        if (!found) {
            return pid;
        }
    }
    return pid;
}

int process_group_summarize(const ProcessRecord *records, size_t count,
                            DynamicArray *groups)
{
    size_t index;
    if (records == NULL || groups == NULL ||
        !dynamic_array_init(groups, sizeof(ProcessGroupSummary))) {
        return 0;
    }
    for (index = 0; index < count; index++) {
        unsigned long root_pid = find_root(records, count, records[index].pid);
        size_t group_index;
        ProcessGroupSummary *group = NULL;
        for (group_index = 0; group_index < groups->length; group_index++) {
            ProcessGroupSummary *candidate = dynamic_array_at(groups, group_index);
            if (candidate->root_pid == root_pid) {
                group = candidate;
                break;
            }
        }
        if (group == NULL) {
            ProcessGroupSummary new_group;
            new_group.root_pid = root_pid;
            new_group.cpu_time_ms = 0;
            new_group.memory_kb = 0;
            new_group.process_count = 0;
            if (!dynamic_array_push(groups, &new_group)) {
                dynamic_array_destroy(groups);
                return 0;
            }
            group = dynamic_array_at(groups, groups->length - 1);
        }
        group->cpu_time_ms += records[index].cpu_time_ms;
        group->memory_kb += records[index].memory_kb;
        group->process_count++;
    }
    return 1;
}

