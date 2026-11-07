# Priority Scheduling and Aging

Priority values use the project convention that smaller numbers mean higher priority. The simulator supports preemptive and non-preemptive priority scheduling.

When aging is enabled, a waiting process receives an effective priority improvement every configured aging interval:

```text
effective_priority = max(0, base_priority - floor(wait_time / aging_interval) * aging_amount)
```

The base priority is never modified. This makes the result deterministic and keeps observed priority separate from the simulated effective priority.

A process is flagged as starved when its waiting time exceeds five times the workload's average burst by default. The result reports the number of flagged jobs so the UI can compare priority scheduling with and without aging.

