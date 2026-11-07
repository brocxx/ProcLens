#ifdef __APPLE__

#include "collector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copy_text(const char *text)
{
    size_t length = strlen(text) + 1;
    char *copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, text, length);
    }
    return copy;
}

int collector_init(Collector *collector)
{
    if (collector == NULL) {
        return 0;
    }
    collector->sample_index = 0;
    return 1;
}

void collector_destroy(Collector *collector)
{
    if (collector != NULL) {
        collector->sample_index = 0;
    }
}

int collector_sample(Collector *collector, unsigned long sample_index,
                     unsigned long long timestamp_ms,
                     ProcessRecord **records, size_t *count)
{
    FILE *pipe;
    char line[512];
    ProcessRecord *items = NULL;
    size_t length = 0;
    size_t capacity = 0;
    if (collector == NULL || records == NULL || count == NULL) {
        return 0;
    }
    *records = NULL;
    *count = 0;
    pipe = popen("ps -axo pid=,ppid=,time=,pri=,rss=,comm=", "r");
    if (pipe == NULL) {
        return 0;
    }
    while (fgets(line, sizeof(line), pipe) != NULL) {
        ProcessRecord record;
        unsigned long pid;
        unsigned long ppid;
        unsigned long long cpu_seconds;
        long priority;
        unsigned long long memory;
        char name[256];
        if (sscanf(line, "%lu %lu %llu %ld %llu %255[^\n]", &pid, &ppid,
                   &cpu_seconds, &priority, &memory, name) != 6) {
            continue;
        }
        process_record_init(&record);
        record.pid = pid;
        record.ppid = ppid;
        record.name = copy_text(name);
        record.start_time = copy_text("unavailable");
        record.os = copy_text("macos");
        record.cpu_time_ms = cpu_seconds * 1000ULL;
        record.priority_raw = priority;
        record.memory_kb = memory;
        record.sample_index = sample_index;
        record.timestamp_ms = timestamp_ms;
        if (record.name == NULL || record.start_time == NULL || record.os == NULL) {
            process_record_destroy(&record);
            collector_free_records(items, length);
            pclose(pipe);
            return 0;
        }
        if (length == capacity) {
            size_t next_capacity = capacity == 0 ? 32 : capacity * 2;
            ProcessRecord *grown = realloc(items, next_capacity * sizeof(*items));
            if (grown == NULL) {
                process_record_destroy(&record);
                collector_free_records(items, length);
                pclose(pipe);
                return 0;
            }
            items = grown;
            capacity = next_capacity;
        }
        items[length++] = record;
    }
    pclose(pipe);
    collector->sample_index = sample_index;
    *records = items;
    *count = length;
    return 1;
}

void collector_free_records(ProcessRecord *records, size_t count)
{
    size_t index;
    if (records == NULL) {
        return;
    }
    for (index = 0; index < count; index++) {
        process_record_destroy(&records[index]);
    }
    free(records);
}

int collector_wait_ms(unsigned long milliseconds)
{
    usleep(milliseconds * 1000U);
    return 1;
}

const char *collector_platform_name(void) { return "macos"; }

#else
typedef int proclens_mac_collector_translation_unit;
#endif

