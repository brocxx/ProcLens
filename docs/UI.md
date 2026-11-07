# ProcLens local interface

The UI is plain HTML, CSS and an ES module. There are no CDN assets, web fonts,
frameworks, package installs or frontend build steps. The C server serves a fixed
allowlist of assets from `ui/`; open the server URL, not the HTML as a file.

## Design system

- Graphite surfaces with mint accent, amber warnings and rose errors.
- Dark-first and persistent light theme, system fonts and tabular numerals.
- Shared 8px spacing rhythm, bordered cards, restrained 150–300ms motion.
- Responsive navigation and grids; wide data tables and Gantt charts scroll inside
  their containers. No whole-page horizontal scroll is intended.
- Visible keyboard focus, labeled controls, skip link, status announcements,
  native dialog focus trapping and reduced-motion support.
- Ctrl/Cmd+K opens screen navigation; Escape closes it.

## Screens

Overview, observed/simulated process explorer, parent-child tree, simulator,
side-by-side Gantt playback, policy comparison, aging comparison, adaptive/advisor,
decision log, and exports/About.

Scheduling, elapsed times, fairness, resource totals, recommendations and experiment
results are computed in C. JavaScript performs rendering, filtering, sorting,
chart scaling and playback only. Missing results remain explicit empty states.
The demo is labeled synthetic, not a captured workload.

## Important current scope

- Capture progress is indeterminate while the synchronous server samples; the UI
  does not invent a percentage.
- The process table renders at most 500 matching rows. Search reduces that window.
- The advisor currently exposes the backend's single-job priority experiment.
  It does not claim to change an entire process group or the real OS priority.
- Starvation is a finite-run waiting threshold, not proof of infinite starvation.
- About reports the current linear ready-job selection implementation honestly;
  the planned heap integration is not asserted as implemented.
- Validation must finish in the backend before any accuracy is displayed.

## Manual checks

1. Load demo, select all six policies and run; inspect both Gantt selectors,
   play/pause, scrubber, comparison table and decision log.
2. Capture a short read-only sample; check observed versus simulated tabs,
   process-name/PID search, tree collapse and group totals.
3. Run recommendation, seeded validation, quantum sweep and a modeled priority
   change; check the model labels and export each format.
4. Inspect every screen in both themes and a narrow viewport; use keyboard-only
   navigation and the command palette.
5. Stop the server and refresh: an explicit unavailable-engine error must appear,
   never invented data. Restart and reconnect using Refresh.
