#ifndef PROCLENS_SIMULATION_H
#define PROCLENS_SIMULATION_H

#include "model.h"
#include "../ds/structures.h"

typedef enum {
    EVENT_RUN = 1,
    EVENT_IDLE = 2,
    EVENT_CONTEXT_SWITCH = 3
} EventKind;

typedef struct {
    unsigned long pid;
    unsigned long long start_tick;
    unsigned long long end_tick;
    EventKind kind;
} GanttEvent;

typedef struct {
    unsigned long pid;
    unsigned long long completion_tick;
    unsigned long long turnaround_tick;
    unsigned long long waiting_tick;
    unsigned long long response_tick;
} JobMetrics;

typedef struct {
    DynamicArray events;
    DynamicArray metrics;
    LinkedList decisions;
    unsigned long long makespan;
    unsigned long long context_switches;
    double average_waiting;
    double average_turnaround;
    double average_response;
    double throughput;
    double utilization;
    double fairness;
    unsigned long long max_waiting;
    size_t starved_count;
} SimulationResult;

void simulation_result_init(SimulationResult *result);
void simulation_result_destroy(SimulationResult *result);
int simulate_fcfs(const Job *jobs, size_t count, unsigned long long context_cost,
                  SimulationResult *result);
int simulate_sjf(const Job *jobs, size_t count, int preemptive,
                 unsigned long long context_cost, SimulationResult *result);
int simulate_round_robin(const Job *jobs, size_t count, unsigned long long quantum,
                         unsigned long long context_cost, SimulationResult *result);
int simulate_priority(const Job *jobs, size_t count, int preemptive,
                      unsigned long long aging_interval,
                      unsigned int aging_amount,
                      unsigned long long context_cost,
                      SimulationResult *result);
int simulation_validate(const Job *jobs, size_t count,
                        const SimulationResult *result);
int workload_generate(DynamicArray *jobs, size_t count, unsigned long seed);
int workload_load_csv(const char *path, DynamicArray *jobs);

#endif

