#include "snapshot.h"

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static char *copy_text(const char *text)
{
    size_t length;
    char *copy;
    if (text == NULL) {
        return NULL;
    }
    length = strlen(text) + 1;
    copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, text, length);
    }
    return copy;
}

static int copy_record(ProcessRecord *destination, const ProcessRecord *source)
{
    process_record_init(destination);
    destination->pid = source->pid;
    destination->ppid = source->ppid;
    destination->cpu_time_ms = source->cpu_time_ms;
    destination->priority_raw = source->priority_raw;
    destination->memory_kb = source->memory_kb;
    destination->sample_index = source->sample_index;
    destination->timestamp_ms = source->timestamp_ms;
    destination->name = copy_text(source->name == NULL ? "" : source->name);
    destination->start_time = copy_text(source->start_time == NULL ? "" : source->start_time);
    destination->os = copy_text(source->os == NULL ? "" : source->os);
    if (destination->name == NULL || destination->start_time == NULL ||
        destination->os == NULL) {
        process_record_destroy(destination);
        return 0;
    }
    return 1;
}

int snapshot_collect(Collector *collector, unsigned long seconds,
                     unsigned long interval_ms, DynamicArray *samples)
{
    unsigned long sample_count;
    unsigned long sample_index;
    if (collector == NULL || samples == NULL || interval_ms == 0 ||
        !dynamic_array_init(samples, sizeof(ProcessRecord))) {
        return 0;
    }
    sample_count = seconds == 0 ? 1 : (seconds * 1000U) / interval_ms + 1U;
    for (sample_index = 0; sample_index < sample_count; sample_index++) {
        ProcessRecord *records = NULL;
        size_t count = 0;
        size_t index;
        unsigned long long timestamp = (unsigned long long)sample_index * interval_ms;
        if (!collector_sample(collector, sample_index, timestamp, &records, &count)) {
            dynamic_array_destroy(samples);
            return 0;
        }
        for (index = 0; index < count; index++) {
            ProcessRecord copy;
            if (!copy_record(&copy, &records[index]) ||
                !dynamic_array_push(samples, &copy)) {
                process_record_destroy(&copy);
                collector_free_records(records, count);
                dynamic_array_destroy(samples);
                return 0;
            }
        }
        collector_free_records(records, count);
        if (sample_index + 1 < sample_count && !collector_wait_ms(interval_ms)) {
            dynamic_array_destroy(samples);
            return 0;
        }
    }
    return 1;
}

static void write_csv_field(FILE *file, const char *text)
{
    const char *cursor = text == NULL ? "" : text;
    int needs_quotes = strchr(cursor, ',') != NULL || strchr(cursor, '"') != NULL;
    if (!needs_quotes) {
        fputs(cursor, file);
        return;
    }
    fputc('"', file);
    while (*cursor != '\0') {
        if (*cursor == '"') {
            fputc('"', file);
        }
        fputc(*cursor, file);
        cursor++;
    }
    fputc('"', file);
}

int snapshot_save_csv(const char *path, const ProcessRecord *records, size_t count)
{
    FILE *file;
    size_t index;
    if (path == NULL || records == NULL) {
        return 0;
    }
    file = fopen(path, "w");
    if (file == NULL) {
        return 0;
    }
    fprintf(file, "sample_index,timestamp_ms,pid,ppid,name,start_time,cpu_time_ms,"
                  "priority_raw,memory_kb,os\n");
    for (index = 0; index < count; index++) {
        const ProcessRecord *record = &records[index];
        fprintf(file, "%lu,%llu,%lu,%lu,", record->sample_index,
                record->timestamp_ms, record->pid, record->ppid);
        write_csv_field(file, record->name);
        fprintf(file, ",");
        write_csv_field(file, record->start_time);
        fprintf(file, ",%llu,%ld,%llu,", record->cpu_time_ms,
                record->priority_raw, record->memory_kb);
        write_csv_field(file, record->os);
        fputc('\n', file);
    }
    return fclose(file) == 0;
}

int snapshot_load_csv(const char *path, DynamicArray *records)
{
    FILE *file;
    char line[1024];
    if (path == NULL || records == NULL ||
        !dynamic_array_init(records, sizeof(ProcessRecord))) {
        return 0;
    }
    file = fopen(path, "r");
    if (file == NULL) {
        return 0;
    }
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return 0;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        ProcessRecord record;
        char name[256];
        char start_time[128];
        char os[32];
        if (sscanf(line, "%lu,%llu,%lu,%lu,%255[^,],%127[^,],%llu,%ld,%llu,%31s",
                   &record.sample_index, &record.timestamp_ms, &record.pid,
                   &record.ppid, name, start_time, &record.cpu_time_ms,
                   &record.priority_raw, &record.memory_kb, os) != 10) {
            continue;
        }
        process_record_init(&record);
        /* Re-read scalar fields because initialization resets them. */
        sscanf(line, "%lu,%llu,%lu,%lu,%255[^,],%127[^,],%llu,%ld,%llu,%31s",
               &record.sample_index, &record.timestamp_ms, &record.pid,
               &record.ppid, name, start_time, &record.cpu_time_ms,
               &record.priority_raw, &record.memory_kb, os);
        record.name = copy_text(name);
        record.start_time = copy_text(start_time);
        record.os = copy_text(os);
        if (record.name == NULL || record.start_time == NULL || record.os == NULL ||
            !dynamic_array_push(records, &record)) {
            process_record_destroy(&record);
            fclose(file);
            dynamic_array_destroy(records);
            return 0;
        }
    }
    fclose(file);
    return 1;
}

int snapshot_records_to_jobs(const ProcessRecord *records, size_t count,
                             unsigned long long interval_ms, double alpha,
                             DynamicArray *jobs)
{
    size_t index;
    if (records == NULL || jobs == NULL || interval_ms == 0 ||
        alpha < 0.0 || alpha > 1.0 || !dynamic_array_init(jobs, sizeof(Job))) {
        return 0;
    }
    for (index = 0; index < count; index++) {
        Job job;
        const ProcessRecord *first = &records[index];
        const ProcessRecord *last = first;
        unsigned long long cpu_delta;
        double estimate;
        size_t other;
        int already_added = 0;
        unsigned int priority;
        for (other = 0; other < index; other++) {
            if (records[other].pid == records[index].pid) {
                already_added = 1;
                break;
            }
        }
        if (already_added) {
            continue;
        }
        for (other = index + 1; other < count; other++) {
            if (records[other].pid == records[index].pid &&
                records[other].sample_index >= last->sample_index) {
                last = &records[other];
            }
            if (records[other].pid == records[index].pid &&
                records[other].sample_index < first->sample_index) {
                first = &records[other];
            }
        }
        cpu_delta = last->cpu_time_ms >= first->cpu_time_ms ?
                    last->cpu_time_ms - first->cpu_time_ms : 0;
        estimate = alpha * (last->sample_index > first->sample_index ?
                            (double)cpu_delta : (double)last->cpu_time_ms) +
                   (1.0 - alpha) * (double)interval_ms;
        job_init(&job);
        job.pid = last->pid;
        job.ppid = last->ppid;
        job.name = copy_text(last->name == NULL ? "" : last->name);
        job.arrival_tick = first->timestamp_ms;
        job.burst_tick = estimate < 1.0 ? 1 :
                         estimate >= (double)ULLONG_MAX ? ULLONG_MAX :
                         (unsigned long long)estimate;
        if (job.burst_tick == 0) {
            job.burst_tick = 1;
        }
        job.remaining_tick = job.burst_tick;
        priority = last->priority_raw < 0 ? 0U : (unsigned int)last->priority_raw;
        job.priority = priority > 9U ? 9U : priority;
        if (job.name == NULL || !dynamic_array_push(jobs, &job)) {
            job_destroy(&job);
            dynamic_array_destroy(jobs);
            return 0;
        }
    }
    return 1;
}
