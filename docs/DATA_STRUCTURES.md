# ProcLens Data Structures

Phase 1 implements the following hand-written structures in [structures.h](../src/ds/structures.h) and [structures.c](../src/ds/structures.c). The implementation uses no external data-structure library.

| Structure | Use | Main operation complexity |
|---|---|---|
| Dynamic array | Jobs, Gantt events, samples | Index `O(1)`, append amortized `O(1)`, growth `O(n)` |
| Singly linked list | Decision log and linked queue | Push back `O(1)`, pop front `O(1)` |
| Linked queue | FCFS ready queue | Push/pop `O(1)` |
| Circular queue | Round Robin ready queue | Push/pop `O(1)` |
| Binary heap | SJF, SRTF, and priority ready queues | Push/pop `O(log n)`, peek `O(1)` |
| First-child/next-sibling tree | Process hierarchy | Add with current traversal `O(n)`, descendant count `O(d)` |

The heap accepts a comparator, so the same implementation can support minimum-oriented SJF behavior and maximum-oriented priority behavior. Phase 3 will connect the heap to SJF, SRTF, and Round Robin; Phase 4 will connect aging-aware priority scheduling.

