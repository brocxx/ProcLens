# ProcLens CLI and Local API

Phase 9 adds [main.c](../src/cli/main.c), a standalone CLI fallback. It can capture a snapshot, run a comparison, and configure the Round Robin quantum.

The active C socket runtime is in [http_runtime.c](../src/server/http_runtime.c),
with routes and JSON serialization in [api.c](../src/server/api.c). The server
binds only to `127.0.0.1`, so it is not exposed to the network.

## Build

```text
make cli
make server
```

With CMake, build the `proclens_cli` and `proclens_server` targets.

## Routes

- `GET /` - local landing page
- `GET /api/health` - server status and read-only declaration
- `GET /api/state` - current observed records, simulated jobs, runs, recommendations and groups
- `POST /api/demo` - load a deterministic synthetic workload
- `POST /api/capture?seconds=1..10` - read-only sample and bounded simulated workload
- `POST /api/simulate` - run selected scheduling policies
- `POST /api/recommend` and `POST /api/validate` - heuristic recommendation and seeded validation
- `POST /api/sweep` and `POST /api/advisor` - Round Robin quantum and modeled priority experiments
- `GET /api/export?format=json|csv|text` - export the latest simulation

Capture keeps the latest observed record per PID. Simulated jobs are estimated
from CPU-time changes across samples. To stay within the simulator's safety
bounds, the capture workload includes at most 128 jobs and 20,000 total estimated
ticks; the response reports `omitted_jobs` when observations do not fit. Observed
records remain separate from those simulated jobs. Observed records and group
summaries include `memory_available`; a missing measurement is reported as
unavailable rather than as a measured zero.

All mutating routes use POST. The server binds only to loopback and accepts
same-origin requests without request bodies.
