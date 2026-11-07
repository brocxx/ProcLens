#include "model.h"

#include <stdlib.h>

void process_record_init(ProcessRecord *record)
{
    if (record == NULL) {
        return;
    }
    record->pid = 0;
    record->ppid = 0;
    record->name = NULL;
    record->start_time = NULL;
    record->os = NULL;
    record->cpu_time_ms = 0;
    record->priority_raw = 0;
    record->memory_kb = 0;
    record->sample_index = 0;
    record->timestamp_ms = 0;
}

void process_record_destroy(ProcessRecord *record)
{
    if (record == NULL) {
        return;
    }
    free(record->name);
    free(record->start_time);
    free(record->os);
    process_record_init(record);
}

void job_init(Job *job)
{
    if (job == NULL) {
        return;
    }
    job->pid = 0;
    job->ppid = 0;
    job->name = NULL;
    job->arrival_tick = 0;
    job->burst_tick = 0;
    job->remaining_tick = 0;
    job->priority = 0;
}

void job_destroy(Job *job)
{
    if (job == NULL) {
        return;
    }
    free(job->name);
    job_init(job);
}

