#ifndef PROCLENS_MODEL_H
#define PROCLENS_MODEL_H

#include <stddef.h>

/* Observed data collected from the operating system. */
typedef struct {
    unsigned long pid;
    unsigned long ppid;
    char *name;
    char *start_time;
    char *os;
    unsigned long long cpu_time_ms;
    long priority_raw;
    unsigned long long memory_kb;
    unsigned long sample_index;
    unsigned long long timestamp_ms;
} ProcessRecord;

/* Simulated data derived from one or more observed records. */
typedef struct {
    unsigned long pid;
    unsigned long ppid;
    char *name;
    unsigned long long arrival_tick;
    unsigned long long burst_tick;
    unsigned long long remaining_tick;
    unsigned int priority;
} Job;

void process_record_init(ProcessRecord *record);
void process_record_destroy(ProcessRecord *record);
void job_init(Job *job);
void job_destroy(Job *job);

#endif

