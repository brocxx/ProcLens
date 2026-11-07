# ProcLens Comparison Reports

Phase 5 adds a comparison layer in [report.h](../src/analysis/report.h) and [report.c](../src/analysis/report.c). It runs a selected set of algorithms against the same immutable job workload and stores each `SimulationResult` independently.

## Output formats

- Text: readable comparison table and trade-off explanation.
- CSV: one row per algorithm, suitable for spreadsheets.
- JSON: one object per algorithm for the local UI/API phases.
- ASCII Gantt: event sequence containing process, idle, and context-switch intervals.

The report deliberately identifies the best waiting-time and response-time algorithms separately. It never presents a single universal winner. All explanations are based on the measured simulation results and the known trade-offs of each policy.

