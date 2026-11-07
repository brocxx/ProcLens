# ProcLens

ProcLens is a read-only process-analysis and CPU-scheduling laboratory for DSA
and Operating Systems coursework. It captures process metadata, estimates a
counterfactual job workload, and compares single-CPU scheduling policies. It
does not reproduce the host operating system's scheduler.

## Safety and interpretation

- The collector only reads process information; it never pauses, kills, or
  reprioritizes real processes.
- Captured process observations and simulated jobs are displayed separately.
- Burst times derived from CPU samples are estimates, not predictions of future
  CPU demand.
- A Gantt chart shows the simulator's decisions, not actual operating-system
  execution.
- Recommendations and advisor results are modeled guidance, not guarantees.

## Features

- FCFS, SJF, SRTF, Round Robin, Priority, and Priority with Aging simulation.
- Gantt events, per-job metrics, decision logs, fairness/starvation indicators,
  policy comparisons, and text/CSV/JSON exports.
- Windows and macOS collector backends behind a platform-neutral C interface.
- Seeded synthetic demo workloads, explainable recommendations, validation, and
  a modeled priority what-if.
- Offline HTML/CSS/JavaScript interface served by a loopback-only C HTTP server.

## Build and test

Requirements: GCC with C99 support, GNU Make, and Node.js. From the repository
root in an MSYS2 UCRT64 shell:

```sh
make test
make cli
make server
```

`make test` uses strict C99 warnings and runs the C suite, JavaScript syntax
check, and UI renderer checks. The renderer tests use a minimal DOM double; they
are not browser visual tests.

## Run

Start the local server from the repository root:

```sh
./build/proclens_server
```

Then open <http://127.0.0.1:8080>. The server only binds to loopback. Keep the
server running while using the interface.

For the standalone terminal interface:

```sh
./build/proclens_cli
```

## Live API contract test

With the server running in one terminal, run this in another:

```sh
node tests/ui_contract.mjs
```

The script checks the real local C server, API routes, exports, validation,
capture bounds, and capture-to-simulation flow. It refuses to test a non-loopback
host.

## Project guide

- [Implementation phases and current status](./PROGRESS.md)
- [Exam notes and core scheduling concepts](./docs/EXAM_NOTES.md)
- [Presentation/demo walkthrough](./docs/DEMO_SCRIPT.md)
- [CLI and API reference](./docs/CLI_AND_API.md)
- [UI behavior and manual checks](./docs/UI.md)
- [Collector scope and platform checklist](./docs/COLLECTOR.md)

## Current limitations

The ready-job selection implementation uses linear scans rather than heaps.
The advisor models a single-job priority change, and the adaptive scheduler is
rule-based. The Windows collector reports memory as unavailable. macOS collector
verification requires a macOS host. See [PROGRESS.md](./PROGRESS.md) for the
current validation record and outstanding platform check.
