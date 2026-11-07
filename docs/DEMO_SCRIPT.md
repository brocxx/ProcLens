# ProcLens Demo Script

Suggested length: 6-8 minutes. Use the synthetic demo workload for repeatable
results. Keep the server running from the repository root and open
<http://127.0.0.1:8080>.

## 1. Set expectations (30 seconds)

**Say:** “ProcLens is read-only. It samples process metadata but does not control
real processes. The schedule and Gantt chart are a counterfactual, single-CPU
simulation; captured burst times are estimates.”

Start on Overview and point out the separate observed-process and simulated-job
counts.

## 2. Load a repeatable workload (30 seconds)

Choose **Load demo workload**. Explain that this is synthetic data, not a live
capture. Open **Processes** and switch between observed records and simulated
jobs. Point out PID/name search and the estimated burst field. Open **Process
tree** to show parent-child relationships and group summaries.

## 3. Compare scheduling policies (2 minutes)

Open **Simulator**, use the demo workload, select all six policies, and run the
simulation. Mention the Round Robin quantum and context-switch cost settings.

Open **Compare** and compare waiting, response, turnaround, utilization, and
fairness rather than declaring one policy universally best. Open **Gantt
timeline**, choose two different policies, play and pause playback, then scrub
the timeline. Explain that each segment is a simulated run, idle interval, or
context-switch cost.

## 4. Explain the decisions (1 minute)

Open **Aging & starvation** to contrast priority scheduling with aging. Note
that the starvation count is a finite-run threshold, not a proof of indefinite
starvation.

Open **Adaptive & advisor** and run **Analyze workload** and
**Run 500-workload validation**. Explain that the recommendation is a heuristic
and the validation measures its match rate on seeded synthetic workloads.
Change a target PID's priority and choose **Model change**. Emphasize that the
before/after values model one job only; the OS priority is unchanged.

## 5. Show traceability and export (1 minute)

Open **Decision log** and connect selection entries to the simulated timeline.
Open **Export & about** and download JSON, CSV, and text reports. Mention that
exports describe simulation results rather than raw operating-system scheduling
decisions.

## 6. Optional live capture (1-2 minutes)

If time and permissions allow, use **Capture processes** for a short sample.
Show the observed records separately from the bounded simulated workload.
Explain that burst estimates come from sampled CPU-time changes and that omitted
jobs are reported when the workload safety limits are reached. Do not interpret
the resulting Gantt chart as actual process execution.

## 7. Close (15 seconds)

Return to Overview and restate the boundary: “The collector observes; the C
engine simulates; no real process is modified.” Stop the local server after the
demo.
