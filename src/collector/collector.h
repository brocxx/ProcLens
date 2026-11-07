#ifndef PROCLENS_COLLECTOR_H
#define PROCLENS_COLLECTOR_H

#include <stddef.h>

#include "../sim/model.h"

typedef struct {
    unsigned long sample_index;
} Collector;

int collector_init(Collector *collector);
void collector_destroy(Collector *collector);
int collector_sample(Collector *collector, unsigned long sample_index,
                     unsigned long long timestamp_ms,
                     ProcessRecord **records, size_t *count);
void collector_free_records(ProcessRecord *records, size_t count);
int collector_wait_ms(unsigned long milliseconds);
const char *collector_platform_name(void);

#endif

