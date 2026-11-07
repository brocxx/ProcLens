# ProcLens Collector

The collector is read-only. It enumerates process metadata but never opens a process for control, pauses it, terminates it, or changes its priority.

The platform-neutral interface is [collector.h](../src/collector/collector.h). Windows code is isolated in [collector_win.c](../src/collector/collector_win.c); macOS code is isolated in [collector_mac.c](../src/collector/collector_mac.c). The rest of the project depends only on the interface.

Phase 6 samples at a configurable interval, stores observed records in the snapshot CSV format, groups processes by their ancestor PID, and converts observations into estimated jobs. The conversion is a what-if model: CPU time is not an exact future burst time.

## Windows verification checklist

On Windows, verify that:

1. A normal user can capture processes with accessible metadata.
2. Access-denied or exiting processes do not abort the full sample.
3. Parent PIDs are populated when Windows reports them.
4. The saved CSV can be loaded back with the same scalar fields.
5. No process is paused, terminated, reprioritized, or otherwise controlled.

## macOS verification checklist

On macOS, verify the same behavior and confirm that the `ps` command is available. The macOS implementation intentionally reports unavailable start times as `unavailable` until a native start-time source is added.

