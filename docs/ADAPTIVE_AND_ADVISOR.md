# Adaptive Scheduler and Modeled Advisor

The adaptive scheduler is intentionally rule-based rather than machine-learning-based. This keeps every decision explainable in a lab exam.

## Features used

- Job count
- Mean burst
- Burst variance
- Short-job share
- Long-job share
- Arrival spread
- Priority spread
- CPU-heavy job count

The rules use named thresholds in [adaptive.c](../src/analysis/adaptive.c). The recommendation is a policy suggestion, not a universal optimum. Validation compares the adaptive choice with fixed policies over seeded synthetic workloads and reports its accuracy honestly for either average response time or average waiting time.

## Advisor behavior

The advisor aggregates process groups and flags groups contributing at least 50% of observed CPU time or memory in the supplied workload. Its priority what-if function copies the jobs, changes only the copied target priority, reruns the simulation, and compares the modeled results. It never changes a real process or OS priority.

