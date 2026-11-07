#ifdef _WIN32

#include "collector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tlhelp32.h>

static char *copy_text(const char *text)
{
    size_t length = strlen(text) + 1;
    char *copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, text, length);
    }
    return copy;
}

static void clear_record(ProcessRecord *record)
{
    process_record_destroy(record);
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

static unsigned long long filetime_to_ms(const FILETIME *kernel,
                                         const FILETIME *user)
{
    ULARGE_INTEGER kernel_value;
    ULARGE_INTEGER user_value;
    kernel_value.LowPart = kernel->dwLowDateTime;
    kernel_value.HighPart = kernel->dwHighDateTime;
    user_value.LowPart = user->dwLowDateTime;
    user_value.HighPart = user->dwHighDateTime;
    return (kernel_value.QuadPart + user_value.QuadPart) / 10000ULL;
}

int collector_sample(Collector *collector, unsigned long sample_index,
                     unsigned long long timestamp_ms,
                     ProcessRecord **records, size_t *count)
{
    HANDLE snapshot;
    PROCESSENTRY32 entry;
    ProcessRecord *items = NULL;
    size_t length = 0;
    size_t capacity = 0;
    if (collector == NULL || records == NULL || count == NULL) {
        return 0;
    }
    *records = NULL;
    *count = 0;
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    entry.dwSize = sizeof(entry);
    if (!Process32First(snapshot, &entry)) {
        CloseHandle(snapshot);
        return 1;
    }
    do {
        ProcessRecord record;
        char start_buffer[64];
        HANDLE process;
        FILETIME creation;
        FILETIME exit_time;
        FILETIME kernel;
        FILETIME user;
        process_record_init(&record);
        record.pid = entry.th32ProcessID;
        record.ppid = entry.th32ParentProcessID;
        record.name = copy_text(entry.szExeFile);
        record.start_time = copy_text("unavailable");
        record.os = copy_text("windows");
        record.sample_index = sample_index;
        record.timestamp_ms = timestamp_ms;
        record.priority_raw = 0;
        if (record.name == NULL || record.start_time == NULL || record.os == NULL) {
            clear_record(&record);
            collector_free_records(items, length);
            CloseHandle(snapshot);
            return 0;
        }
        process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                              entry.th32ProcessID);
        if (process != NULL) {
            record.priority_raw = (long)GetPriorityClass(process);
            if (GetProcessTimes(process, &creation, &exit_time, &kernel, &user)) {
                record.cpu_time_ms = filetime_to_ms(&kernel, &user);
                (void)snprintf(start_buffer, sizeof(start_buffer), "%lu:%lu",
                               creation.dwHighDateTime, creation.dwLowDateTime);
                free(record.start_time);
                record.start_time = copy_text(start_buffer);
            }
            CloseHandle(process);
        }
        if (length == capacity) {
            size_t next_capacity = capacity == 0 ? 32 : capacity * 2;
            ProcessRecord *grown = realloc(items, next_capacity * sizeof(*items));
            if (grown == NULL) {
                clear_record(&record);
                collector_free_records(items, length);
                CloseHandle(snapshot);
                return 0;
            }
            items = grown;
            capacity = next_capacity;
        }
        items[length++] = record;
    } while (Process32Next(snapshot, &entry));
    CloseHandle(snapshot);
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
    Sleep(milliseconds);
    return 1;
}

const char *collector_platform_name(void) { return "windows"; }

#else
typedef int proclens_windows_collector_translation_unit;
#endif

