# ProcLens Progress

## Current phase

Phase 13 - Polish and hand-in (complete on Windows; macOS collector verification remains outstanding)

## Completed

- Project scope and engineering constraints recorded.
- Current development target identified as Windows.
- Phase 0 repository layout created.
- Warning-controlled Makefile and CMake test target created.
- Portable `ProcessRecord` and `Job` structs created with ownership-safe init/destroy functions.
- Simulation conventions and snapshot CSV format documented.
- Minimal hand-written test harness created with two foundation tests.
- Phase 1 data structures implemented and covered by boundary tests.
- Phase 2 FCFS simulation, metrics, Gantt events, decision log, invariant checker, seeded generator, and CSV loader implemented.
- Phase 3 SJF, SRTF, and Round Robin implementations added with quantum and context-switch handling.
- Phase 4 preemptive/non-preemptive priority scheduling, aging, starvation counting, and tests added.
- Phase 5 comparison runner, metric table, trade-off explanations, ASCII Gantt output, and text/CSV/JSON writers added.
- Phase 6 collector interface, Windows/macOS backends, sampling, snapshot CSV persistence, process-group summaries, and process-to-job conversion added.
- Phase 7 workload feature extraction, rule-based adaptive recommendation, seeded validation, and reporting added.
- Phase 8 modeled process-group advisor and read-only priority what-if comparison added.
- Phase 9 standalone CLI fallback added.
- Phase 10 loopback-only C socket server and initial health/comparison routes added.
- Phase 11 offline HTML/CSS/ES-module shell, graphite/mint design tokens,
  dark/light themes, responsive layout, focus states and command palette added.
- Phase 12 ten screens implemented: overview, processes, tree, simulator, Gantt,
  compare, aging, adaptive/advisor, decision log, and export/About.
- UI syntax check passed with Node. Renderer tests passed for all ten screens in
  both empty and populated states, HTML escaping, chart values and error output.
- Phase 12 server/API wiring compiled on Windows with strict C99 warning flags.
- Live HTTP contract passed 27 checks against the real C server, including
  encoded policy selection, all six scheduling policies, recommendations,
  validation, quantum sweep, advisor, downloads, invalid inputs, and live capture.
- Live capture now consolidates samples by PID, estimates bursts from sampled
  CPU-time changes, retains job names, exposes latest observations, and bounds
  the simulated subset to 128 jobs / 20,000 ticks with an omitted-job count.
- Captured memory availability is explicit in the API and unavailable values are
  not displayed as measured zeroes.
- Added C coverage for multi-sample process-to-job aggregation; the strict C
  suite passes 12 test groups.
- Fixed URL-encoded policy IDs from the UI and the initial secondary Gantt policy.
- Real Chrome checks covered all ten screens at desktop and narrow mobile widths,
  dark/light themes, no whole-page horizontal overflow, capture/simulation and
  playback controls, recommendation/validation/advisor actions, and the explicit
  engine-unavailable error state.
- Phase 13 hand-in materials added: root README, OS exam notes, and a timed
  demonstration script with explicit read-only and simulation caveats.
- `make test` now runs the strict C suite, JavaScript syntax check, and renderer
  tests. The stale unused `http_server.c` placeholder was removed; the live
  runtime remains `http_runtime.c`.
- Phase 13 verification passed: all 12 C test groups, UI syntax and rendering
  checks, strict CLI and server builds, CLI startup/exit smoke check, and 27 live
  API/UI contract checks against the rebuilt C server.
- Hand-in Markdown links were checked; the test server was stopped after the
  live contract run.

## Next

- Verify the macOS collector with the macOS teammate when a macOS host is
  available.

## Known limitations

- OS-specific process collection is intentionally not implemented in Phase 0.
- The macOS collector cannot be compiled or manually verified from this Windows session.
- The Windows collector currently reports memory as unavailable; the API exposes
  that state and the UI does not present it as a measured zero.
- Visual checks were performed in the available Chrome browser; they do not
  replace the macOS collector verification noted above.
- Earlier phase summaries overstated completeness: SJF/priority still select by
  linear scan, the adaptive module has no hybrid policy, and the advisor models
  one PID rather than a complete group. The UI labels these limitations honestly.
- Renderer tests use a minimal DOM double; real browser checks were also completed
  for responsive layout and core interactive workflows.
