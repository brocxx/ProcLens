# ProcLens Implementation Plan

## Phase 0 - Foundations

Create the repository layout, warning-controlled build, hand-written test harness, project conventions, snapshot format, and the observed/simulated core structs.

## Phase 1 - Data structures

Implement and test the dynamic array, linked list, queues, heaps, and process tree.

## Phase 2 - Simulation engine and FCFS

Implement jobs, the shared simulation model, Gantt events, decision logs, metrics, invariant checks, seeded synthetic workloads, and FCFS.

## Phase 3 - SJF, SRTF, and Round Robin

Implement heap-based SJF/SRTF and circular-queue Round Robin with quantum and context-switch cost.

## Phase 4 - Priority, aging, and starvation

Implement preemptive/non-preemptive priority scheduling, aging, starvation detection, and a demonstration workload.

## Phase 5 - Comparison and reports

Add multi-algorithm comparison, trade-off explanations, ASCII Gantt charts, and text/CSV/JSON export.

## Phase 6 - Collector and workload builder

Add the Windows and macOS collectors, sampling, snapshot persistence, process trees, group analysis, and observed-to-simulated conversion.

## Phase 7 - Adaptive scheduler and validation

Add explicit workload features, documented rule-based selection, a hybrid policy, and seeded validation against stated objectives.

## Phase 8 - Advisor

Add modeled process-group what-if analysis. Keep all output read-only and clearly labeled as modeled.

## Phase 9 - CLI

Build a complete CLI fallback for capture, loading, viewing, configuration, simulation, comparison, recommendation, logs, and export.

## Phase 10 - Local API server

Add a loopback-only C HTTP server, static-file serving, JSON API, validation, and curl tests.

## Phase 11 - UI design system

Create the offline, framework-free visual foundation, themes, components, accessibility states, and motion rules.

## Phase 12 - UI screens

Implement and connect the overview, processes, tree, simulator, Gantt, compare, aging, adaptive, decision-log, export, and About screens.

## Phase 13 - Polish and hand-in

Complete README, exam notes, demo script, full test run, bug sweep, and dead-code cleanup.

