# ProcLens Exam Notes

## What the project demonstrates

ProcLens combines process metadata collection, data structures, scheduling
simulation, algorithm comparison, and reporting. It is a read-only,
single-CPU what-if tool. The simulated workload is not the operating system's
real ready queue, and its Gantt chart is not a trace of real CPU execution.

## Observed records versus simulated jobs

`ProcessRecord` contains observations such as PID, parent PID, process name,
sample time, cumulative CPU time, and memory availability. A `Job` contains
simulation inputs such as arrival tick, estimated burst, and normalized
priority. The collector does not provide a guaranteed future CPU burst: the
simulator estimates one from sampled CPU-time changes.

One tick is one millisecond. Lower normalized priority values mean higher
priority. Deterministic ties use earlier arrival first, then lower PID.

## Scheduling policies

| Policy | Selection rule | Preemption |
|---|---|---|
| FCFS | Earliest arrival | No |
| SJF | Shortest available burst | No |
| SRTF | Shortest remaining burst | Yes |
| Round Robin | FIFO ready order, bounded by quantum | At quantum expiry |
| Priority | Lowest numeric priority | Yes |
| Priority with Aging | Priority selection with waiting-based priority improvement | Yes |

These are simulator policies. Their trade-offs depend on the input workload and
configuration; no policy is universally best.

## Metrics

For each job:

```text
turnaround = completion - arrival
waiting    = turnaround - burst
response   = first_run - arrival
```

For a run:

- Average waiting, turnaround, and response are arithmetic means across jobs.
- Utilization is simulated busy ticks divided by makespan; context-switch
  overhead contributes to makespan but not busy ticks.
- Throughput is completed jobs divided by makespan.
- Fairness is Jain's index over each job's `burst / turnaround` share.
- The displayed starvation count uses the project's finite-run threshold:
  waiting greater than five times the workload's average burst. It is an
  indicator, not proof of indefinite starvation.

## Aging, context switches, and determinism

Aging periodically improves a waiting job's effective priority by the configured
amount, clamped at the best priority. The project's convention resets the
waiting counter when the job runs. Context-switch cost is charged only when the
CPU changes to another process; there is no charge for the first dispatch, a
continuation of the same process, or idle time.

The same input and configuration should produce the same event order and
metrics. Synthetic workloads use an explicit seed so validation runs can be
reproduced.

## Reading the results critically

- SJF/SRTF assume burst estimates are useful; sampled bursts are only estimates.
- Round Robin trades response opportunities against extra dispatches as the
  quantum changes.
- Priority scheduling can delay low-priority jobs; aging mitigates this in the
  model but does not promise a universal wait bound.
- An adaptive recommendation is explainable rule-based guidance. Validation
  measures its agreement with a selected objective over seeded synthetic
  workloads; it is not proof of optimality.
