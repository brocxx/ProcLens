#include <stdio.h>
#include <stdlib.h>

#include "../analysis/report.h"
#include "../collector/collector.h"
#include "../collector/snapshot.h"

static void destroy_records(DynamicArray *records)
{
    size_t index;
    for (index = 0; index < records->length; index++) {
        process_record_destroy(dynamic_array_at(records, index));
    }
    dynamic_array_destroy(records);
}

static int make_demo_jobs(DynamicArray *jobs)
{
    Job job;
    if (!dynamic_array_init(jobs, sizeof(Job))) return 0;
    job_init(&job); job.pid = 101; job.burst_tick = 7; job.priority = 2;
    if (!dynamic_array_push(jobs, &job)) return 0;
    job_init(&job); job.pid = 102; job.arrival_tick = 1; job.burst_tick = 3; job.priority = 1;
    if (!dynamic_array_push(jobs, &job)) return 0;
    job_init(&job); job.pid = 103; job.arrival_tick = 2; job.burst_tick = 5; job.priority = 3;
    return dynamic_array_push(jobs, &job);
}

static void show_comparison(unsigned long long quantum)
{
    DynamicArray jobs;
    ComparisonReport report;
    AlgorithmKind algorithms[] = {ALGORITHM_FCFS, ALGORITHM_SJF,
                                   ALGORITHM_SRTF, ALGORITHM_ROUND_ROBIN,
                                   ALGORITHM_PRIORITY_AGING};
    if (!make_demo_jobs(&jobs)) return;
    comparison_report_init(&report);
    if (comparison_report_run((Job *)jobs.data, jobs.length, algorithms, 5, quantum,
                              1, 4, 1, &report)) {
        report_write_text(stdout, &report);
        report_write_tradeoffs(stdout, &report);
    }
    comparison_report_destroy(&report);
    dynamic_array_destroy(&jobs);
}

static void capture_snapshot(void)
{
    Collector collector;
    DynamicArray records;
    if (!collector_init(&collector) ||
        !snapshot_collect(&collector, 1, 1000, &records)) {
        puts("Capture failed. Check collector permissions and platform support.");
        return;
    }
    printf("Captured %zu observed process records from %s.\n", records.length,
           collector_platform_name());
    if (snapshot_save_csv("data/latest_snapshot.csv", (ProcessRecord *)records.data, records.length)) {
        puts("Saved data/latest_snapshot.csv");
    }
    destroy_records(&records);
    collector_destroy(&collector);
}

int main(void)
{
    unsigned long long quantum = 2;
    int choice;
    puts("ProcLens - read-only process analysis and CPU scheduling what-if tool");
    puts("HONESTY: this is a single-CPU simulation, not the real OS scheduler.");
    puts("HONESTY: real processes are never controlled, paused, killed, or reprioritized.");
    for (;;) {
        puts("\n1. Capture process snapshot\n2. Run comparison\n3. Set Round Robin quantum\n0. Exit");
        printf("Choice: ");
        if (scanf("%d", &choice) != 1) return 1;
        if (choice == 0) break;
        if (choice == 1) capture_snapshot();
        else if (choice == 2) show_comparison(quantum);
        else if (choice == 3) {
            printf("Quantum in ticks: ");
            if (scanf("%llu", &quantum) != 1 || quantum == 0) quantum = 2;
        } else puts("Unknown choice.");
    }
    return 0;
}

