# ProcLens Project Rules

ProcLens is a read-only, single-CPU, counterfactual process-analysis and CPU-scheduling tool for the DSA and Operating Systems lab.

## Non-negotiable honesty rules

- The Gantt chart is produced by the simulator; it does not reproduce the real operating-system scheduler.
- The collector never controls, pauses, kills, or reprioritizes real processes.
- Burst times are estimates derived from sampled CPU time.
- Observed process data must remain visibly separate from simulated job data.

## Engineering rules

- Backend: C99 only. Core logic is platform-neutral and uses no external libraries.
- OS-specific code belongs only in `src/collector/`.
- Compile with `-std=c99 -Wall -Wextra -Wpedantic -Werror -g`.
- Prefer small, readable functions with explicit ownership and checked allocation.
- Tie-breaks are deterministic: earlier arrival first, then lower PID.
- Every phase must have tests and an updated `PROGRESS.md`.
- Do not begin a later phase until the user explicitly says to proceed.

## Current development target

The current development session is on Windows. Phase 0 must keep the core foundation portable and must not implement OS-specific collection yet.

