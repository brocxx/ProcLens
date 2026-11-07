#include "test_framework.h"

#include "model.h"
#include "simulation.h"
#include "structures.h"
#include "report.h"
#include "groups.h"
#include "snapshot.h"
#include "adaptive.h"
#include "advisor.h"

#include <stdlib.h>
#include <string.h>

static int compare_ints(const void *left, const void *right)
{
    const int a = *(const int *)left;
    const int b = *(const int *)right;
    return a < b ? -1 : (a > b ? 1 : 0);
}

static int test_process_record_defaults(void)
{
    ProcessRecord record;

    process_record_init(&record);
    TEST_ASSERT(record.pid == 0);
    TEST_ASSERT(record.name == NULL);
    TEST_ASSERT(record.cpu_time_ms == 0);
    process_record_destroy(&record);
    return 0;
}

static int test_job_defaults(void)
{
    Job job;

    job_init(&job);
    TEST_ASSERT(job.pid == 0);
    TEST_ASSERT(job.remaining_tick == 0);
    TEST_ASSERT(job.priority == 0);
    job_destroy(&job);
    return 0;
}

static int test_dynamic_array(void)
{
    DynamicArray array;
    int index;
    TEST_ASSERT(dynamic_array_init(&array, sizeof(int)));
    for (index = 0; index < 10; index++) {
        TEST_ASSERT(dynamic_array_push(&array, &index));
    }
    TEST_ASSERT(array.length == 10);
    TEST_ASSERT(*(int *)dynamic_array_at(&array, 9) == 9);
    TEST_ASSERT(dynamic_array_at(&array, 10) == NULL);
    dynamic_array_destroy(&array);
    return 0;
}

static int test_queues_and_heap(void)
{
    CircularQueue ring;
    BinaryHeap heap;
    int values[] = {3, 1, 2};
    TEST_ASSERT(circular_queue_init(&ring, 2));
    TEST_ASSERT(circular_queue_push(&ring, &values[0]));
    TEST_ASSERT(circular_queue_push(&ring, &values[1]));
    TEST_ASSERT(!circular_queue_push(&ring, &values[2]));
    TEST_ASSERT(*(int *)circular_queue_pop(&ring) == 3);
    TEST_ASSERT(circular_queue_push(&ring, &values[2]));
    TEST_ASSERT(*(int *)circular_queue_pop(&ring) == 1);
    TEST_ASSERT(*(int *)circular_queue_pop(&ring) == 2);
    circular_queue_destroy(&ring);
    TEST_ASSERT(binary_heap_init(&heap, compare_ints));
    TEST_ASSERT(binary_heap_push(&heap, &values[0]));
    TEST_ASSERT(binary_heap_push(&heap, &values[1]));
    TEST_ASSERT(binary_heap_push(&heap, &values[2]));
    TEST_ASSERT(*(int *)binary_heap_pop(&heap) == 1);
    TEST_ASSERT(*(int *)binary_heap_pop(&heap) == 2);
    TEST_ASSERT(*(int *)binary_heap_pop(&heap) == 3);
    binary_heap_destroy(&heap);
    return 0;
}

static int test_process_tree(void)
{
    ProcessTree tree;
    ProcessTreeNode *root;
    process_tree_init(&tree);
    root = process_tree_add(&tree, 10, 0, "root");
    TEST_ASSERT(root != NULL);
    TEST_ASSERT(process_tree_add(&tree, 11, 10, "child") != NULL);
    TEST_ASSERT(process_tree_add(&tree, 12, 10, "child2") != NULL);
    TEST_ASSERT(process_tree_descendant_count(root) == 2);
    TEST_ASSERT(process_tree_find(&tree, 12) != NULL);
    process_tree_destroy(&tree);
    return 0;
}

static int test_fcfs_exact_workload(void)
{
    Job jobs[3];
    SimulationResult result;
    const GanttEvent *first;
    const GanttEvent *second;
    const GanttEvent *third;
    job_init(&jobs[0]);
    job_init(&jobs[1]);
    job_init(&jobs[2]);
    jobs[0].pid = 1; jobs[0].arrival_tick = 0; jobs[0].burst_tick = 5;
    jobs[1].pid = 2; jobs[1].arrival_tick = 1; jobs[1].burst_tick = 3;
    jobs[2].pid = 3; jobs[2].arrival_tick = 2; jobs[2].burst_tick = 2;
    simulation_result_init(&result);
    TEST_ASSERT(simulate_fcfs(jobs, 3, 0, &result));
    TEST_ASSERT(simulation_validate(jobs, 3, &result));
    first = dynamic_array_at(&result.events, 0);
    second = dynamic_array_at(&result.events, 1);
    third = dynamic_array_at(&result.events, 2);
    TEST_ASSERT(first->pid == 1 && first->start_tick == 0 && first->end_tick == 5);
    TEST_ASSERT(second->pid == 2 && second->start_tick == 5 && second->end_tick == 8);
    TEST_ASSERT(third->pid == 3 && third->start_tick == 8 && third->end_tick == 10);
    TEST_ASSERT(result.average_waiting > 3.32 && result.average_waiting < 3.34);
    TEST_ASSERT(result.utilization == 1.0);
    simulation_result_destroy(&result);
    return 0;
}

static int test_sjf_srtf_and_round_robin(void)
{
    Job jobs[3];
    SimulationResult result;
    const GanttEvent *event;
    size_t index;
    for (index = 0; index < 3; index++) {
        job_init(&jobs[index]);
        jobs[index].pid = (unsigned long)(index + 1);
        jobs[index].arrival_tick = 0;
    }
    jobs[0].burst_tick = 5;
    jobs[1].burst_tick = 2;
    jobs[2].burst_tick = 1;
    simulation_result_init(&result);
    TEST_ASSERT(simulate_sjf(jobs, 3, 0, 0, &result));
    TEST_ASSERT(simulation_validate(jobs, 3, &result));
    event = dynamic_array_at(&result.events, 0);
    TEST_ASSERT(event->pid == 3);
    event = dynamic_array_at(&result.events, 1);
    TEST_ASSERT(event->pid == 2);
    event = dynamic_array_at(&result.events, 2);
    TEST_ASSERT(event->pid == 1);
    simulation_result_destroy(&result);

    jobs[0].arrival_tick = 0;
    jobs[0].burst_tick = 5;
    jobs[1].arrival_tick = 1;
    jobs[1].burst_tick = 2;
    jobs[2].arrival_tick = 20;
    jobs[2].burst_tick = 1;
    simulation_result_init(&result);
    TEST_ASSERT(simulate_sjf(jobs, 3, 1, 0, &result));
    TEST_ASSERT(simulation_validate(jobs, 3, &result));
    event = dynamic_array_at(&result.events, 0);
    TEST_ASSERT(event->pid == 1 && event->start_tick == 0 && event->end_tick == 1);
    event = dynamic_array_at(&result.events, 1);
    TEST_ASSERT(event->pid == 2 && event->start_tick == 1 && event->end_tick == 3);
    simulation_result_destroy(&result);

    jobs[0].arrival_tick = 0;
    jobs[0].burst_tick = 3;
    jobs[1].arrival_tick = 0;
    jobs[1].burst_tick = 2;
    simulation_result_init(&result);
    TEST_ASSERT(simulate_round_robin(jobs, 2, 2, 0, &result));
    TEST_ASSERT(simulation_validate(jobs, 2, &result));
    event = dynamic_array_at(&result.events, 0);
    TEST_ASSERT(event->pid == 1 && event->start_tick == 0 && event->end_tick == 2);
    event = dynamic_array_at(&result.events, 1);
    TEST_ASSERT(event->pid == 2 && event->start_tick == 2 && event->end_tick == 4);
    event = dynamic_array_at(&result.events, 2);
    TEST_ASSERT(event->pid == 1 && event->start_tick == 4 && event->end_tick == 5);
    simulation_result_destroy(&result);
    return 0;
}

static int test_priority_aging_and_context_switch(void)
{
    Job jobs[2];
    SimulationResult result;
    const GanttEvent *event;
    job_init(&jobs[0]);
    job_init(&jobs[1]);
    jobs[0].pid = 1; jobs[0].burst_tick = 2; jobs[0].priority = 1;
    jobs[1].pid = 2; jobs[1].burst_tick = 1; jobs[1].priority = 2;
    simulation_result_init(&result);
    TEST_ASSERT(simulate_priority(jobs, 2, 0, 0, 0, 3, &result));
    TEST_ASSERT(simulation_validate(jobs, 2, &result));
    TEST_ASSERT(result.context_switches == 1);
    event = dynamic_array_at(&result.events, 1);
    TEST_ASSERT(event->kind == EVENT_CONTEXT_SWITCH && event->start_tick == 2 &&
                event->end_tick == 5);
    simulation_result_destroy(&result);

    jobs[0].pid = 10; jobs[0].arrival_tick = 0; jobs[0].burst_tick = 6;
    jobs[0].priority = 9;
    jobs[1].pid = 11; jobs[1].arrival_tick = 1; jobs[1].burst_tick = 1;
    jobs[1].priority = 0;
    simulation_result_init(&result);
    TEST_ASSERT(simulate_priority(jobs, 2, 1, 2, 1, 0, &result));
    TEST_ASSERT(simulation_validate(jobs, 2, &result));
    TEST_ASSERT(result.starved_count == 0);
    simulation_result_destroy(&result);
    return 0;
}

static int test_comparison_and_reports(void)
{
    Job jobs[2];
    AlgorithmKind algorithms[] = {ALGORITHM_FCFS, ALGORITHM_ROUND_ROBIN,
                                  ALGORITHM_SJF};
    ComparisonReport report;
    FILE *file;
    job_init(&jobs[0]);
    job_init(&jobs[1]);
    jobs[0].pid = 1; jobs[0].burst_tick = 4;
    jobs[1].pid = 2; jobs[1].arrival_tick = 1; jobs[1].burst_tick = 1;
    comparison_report_init(&report);
    TEST_ASSERT(comparison_report_run(jobs, 2, algorithms, 3, 2, 0, 2, 1,
                                      &report));
    TEST_ASSERT(report.count == 3);
    TEST_ASSERT(comparison_best_waiting(&report) != NULL);
    TEST_ASSERT(comparison_best_response(&report) != NULL);
    file = tmpfile();
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(report_write_text(file, &report));
    TEST_ASSERT(report_write_csv(file, &report));
    TEST_ASSERT(report_write_json(file, &report));
    TEST_ASSERT(report_write_gantt(file, &report.runs[0].result));
    TEST_ASSERT(report_write_tradeoffs(file, &report));
    fclose(file);
    comparison_report_destroy(&report);
    return 0;
}

static int test_group_summary_and_conversion(void)
{
    ProcessRecord records[2];
    DynamicArray groups;
    DynamicArray jobs;
    process_record_init(&records[0]);
    process_record_init(&records[1]);
    records[0].pid = 10; records[0].ppid = 0; records[0].cpu_time_ms = 20;
    records[0].memory_kb = 100;
    records[1].pid = 11; records[1].ppid = 10; records[1].cpu_time_ms = 30;
    records[1].memory_kb = 200;
    TEST_ASSERT(process_group_summarize(records, 2, &groups));
    TEST_ASSERT(groups.length == 1);
    TEST_ASSERT(((ProcessGroupSummary *)groups.data)->cpu_time_ms == 50);
    TEST_ASSERT(snapshot_records_to_jobs(records, 2, 1000, 0.5, &jobs));
    TEST_ASSERT(jobs.length == 2);
    TEST_ASSERT(((Job *)jobs.data)->burst_tick > 0);
    TEST_ASSERT(((Job *)jobs.data)->name != NULL);
    dynamic_array_destroy(&groups);
    dynamic_array_destroy(&jobs);
    process_record_destroy(&records[0]);
    process_record_destroy(&records[1]);
    return 0;
}

static int test_sample_conversion_aggregates_processes(void)
{
    ProcessRecord records[3];
    DynamicArray jobs;
    Job *job;
    size_t index;
    for (index = 0; index < 3; index++) process_record_init(&records[index]);
    records[0].pid = 20; records[0].ppid = 1; records[0].cpu_time_ms = 100;
    records[0].sample_index = 0; records[0].timestamp_ms = 0;
    records[1].pid = 20; records[1].ppid = 1; records[1].cpu_time_ms = 180;
    records[1].sample_index = 1; records[1].timestamp_ms = 1000;
    records[1].name = malloc(sizeof("worker"));
    TEST_ASSERT(records[1].name != NULL);
    memcpy(records[1].name, "worker", sizeof("worker"));
    records[2].pid = 21; records[2].cpu_time_ms = 1000;
    records[2].sample_index = 0; records[2].timestamp_ms = 0;
    TEST_ASSERT(snapshot_records_to_jobs(records, 3, 1000, 0.5, &jobs));
    TEST_ASSERT(jobs.length == 2);
    job = dynamic_array_at(&jobs, 0);
    TEST_ASSERT(job->pid == 20);
    TEST_ASSERT(job->arrival_tick == 0);
    TEST_ASSERT(job->burst_tick == 540);
    TEST_ASSERT(strcmp(job->name, "worker") == 0);
    dynamic_array_destroy(&jobs);
    for (index = 0; index < 3; index++) process_record_destroy(&records[index]);
    return 0;
}

static int test_adaptive_and_advisor(void)
{
    Job jobs[3];
    AdaptiveRecommendation recommendation;
    AdaptiveValidation validation;
    SimulationResult before;
    SimulationResult after;
    AdvisorReport advisor;
    ProcessGroupSummary groups[1];
    FILE *file;
    size_t index;
    for (index = 0; index < 3; index++) {
        job_init(&jobs[index]);
        jobs[index].pid = (unsigned long)(index + 1);
        jobs[index].burst_tick = (unsigned long long)(index + 1);
        jobs[index].priority = (unsigned int)(index + 1);
    }
    TEST_ASSERT(adaptive_recommend(jobs, 3, &recommendation));
    TEST_ASSERT(recommendation.explanation[0] != '\0');
    simulation_result_init(&before);
    TEST_ASSERT(adaptive_run(jobs, 3, 2, 0, 4, 1, &recommendation, &before));
    TEST_ASSERT(simulation_validate(jobs, 3, &before));
    TEST_ASSERT(adaptive_validate(3, 42, ADAPTIVE_OBJECTIVE_WAITING, &validation));
    TEST_ASSERT(validation.runs == 3);
    groups[0].root_pid = 1;
    groups[0].cpu_time_ms = 80;
    groups[0].memory_kb = 90;
    groups[0].process_count = 3;
    advisor_report_init(&advisor);
    TEST_ASSERT(advisor_analyze(groups, 1, &advisor));
    simulation_result_init(&after);
    TEST_ASSERT(advisor_model_priority_change(jobs, 3, 1, 0, 2, 0,
                                              &before, &after));
    file = tmpfile();
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(adaptive_write_report(file, &recommendation, &validation));
    TEST_ASSERT(advisor_write(file, &advisor));
    fclose(file);
    simulation_result_destroy(&before);
    simulation_result_destroy(&after);
    advisor_report_destroy(&advisor);
    return 0;
}

int main(void)
{
    if (test_process_record_defaults() != 0) {
        return 1;
    }
    if (test_job_defaults() != 0) {
        return 1;
    }
    if (test_dynamic_array() != 0 || test_queues_and_heap() != 0 ||
        test_process_tree() != 0 || test_fcfs_exact_workload() != 0 ||
        test_sjf_srtf_and_round_robin() != 0 ||
        test_priority_aging_and_context_switch() != 0 ||
        test_comparison_and_reports() != 0 ||
        test_group_summary_and_conversion() != 0 ||
        test_sample_conversion_aggregates_processes() != 0 ||
        test_adaptive_and_advisor() != 0) {
        return 1;
    }
    puts("PASS: Phase 1 through Phase 8 tests (12 test groups)");
    return 0;
}
