#ifndef PROCLENS_SNAPSHOT_H
#define PROCLENS_SNAPSHOT_H

#include <stddef.h>

#include "collector.h"
#include "../ds/structures.h"

int snapshot_collect(Collector *collector, unsigned long seconds,
                     unsigned long interval_ms, DynamicArray *samples);
int snapshot_save_csv(const char *path, const ProcessRecord *records,
                      size_t count);
int snapshot_load_csv(const char *path, DynamicArray *records);
int snapshot_records_to_jobs(const ProcessRecord *records, size_t count,
                             unsigned long long interval_ms,
                             double alpha, DynamicArray *jobs);

#endif

