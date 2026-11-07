# ProcLens Conventions

These conventions are fixed for the project unless a later design review explicitly changes them.

## Simulation

- One simulated tick is one millisecond and all simulated times are integer values.
- The simulator models one CPU only.
- Smaller priority numbers mean higher priority; the normalized range is 0-9.
- Context-switch cost is charged only when the CPU changes to a different process. It is not charged on the first dispatch, when the same process continues, or during idle time.
- Every tie is deterministic: earlier arrival first, then lower PID.

## Aging and starvation

- Every `aging_interval` ticks a job waits in the ready queue, its effective priority improves by `aging_amount`, clamped at 0.
- A job's wait counter resets when it runs.
- A job is starved when its waiting time exceeds `starvation_factor * average_burst`.
- The default starvation factor is 5.

## Fairness

Jain's fairness index is:

```text
J = (sum(x_i)^2) / (n * sum(x_i^2))
```

where `x_i = burst_i / turnaround_i`.

## Observed versus simulated values

Observed values come from the operating-system collector: PID, PPID, name, start time, CPU time, priority, memory, and sampling time.

Simulated values are estimates: arrival time, burst time, remaining burst, and normalized simulation priority.

- Arrival is the time of the first sample where the process was active, measured from capture start.
- Burst is CPU time used per sampling interval, smoothed by `tau_next = alpha * tau_current + (1 - alpha) * tau_previous`.
- The default smoothing factor is `alpha = 0.5`.
- The minimum estimated burst is 1 tick.
- OS priority is mapped to 0-9 using a documented per-OS table in the collector phase.

## Safety and determinism

- The collector is read-only toward the operating system.
- No mutable global state is permitted.
- Allocation failures and invalid indexes must be checked.
- Random generation must receive an explicit seed.

