# ProcLens Snapshot CSV Format

A snapshot file stores observations collected at one or more sampling times. The first line is the exact header below.

```text
sample_index,timestamp_ms,pid,ppid,name,start_time,cpu_time_ms,priority_raw,memory_kb,os
```

## Columns

| Column | Type | Meaning |
|---|---|---|
| `sample_index` | unsigned integer | Zero-based sample number |
| `timestamp_ms` | unsigned integer | Milliseconds from capture start |
| `pid` | unsigned integer | Operating-system process identifier |
| `ppid` | unsigned integer | Parent process identifier, or 0 when unavailable |
| `name` | escaped text | Process name |
| `start_time` | escaped text | OS-reported process start time |
| `cpu_time_ms` | unsigned integer | Accumulated CPU time reported by the OS |
| `priority_raw` | signed integer | OS-native priority value |
| `memory_kb` | unsigned integer | Resident or working-set memory in KiB |
| `os` | escaped text | Collector platform identifier, such as `windows` or `macos` |

CSV fields containing commas or quotes must use standard CSV quoting. A quote inside a quoted field is represented by two quotes.

