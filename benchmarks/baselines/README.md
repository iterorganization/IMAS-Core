# Committed baseline layout

```
benchmarks/baselines/<machine-id>/<commit-sha>.json
```

Each file is the raw Google Benchmark JSON that `al_benchmarks` emitted for
that run — Google Benchmark's own top-level `context` (date, host_name,
num_cpus, ...) and `benchmarks` (one entry per `BENCHMARK()` registration,
repetitions, and `_mean`/`_median`/`_stddev`/`_cv` rows) — with one field
spliced into `context`:

```json
{
  "imas_baseline_metadata": {
    "captured_at": "2026-07-20T09:50:42.347134+00:00",
    "machine_id": "sdcc-gpfs",
    "os": "Linux-5.14.0-x86_64",
    "cpu_model": "Intel(R) Xeon(R) Gold 6248R",
    "compiler": "GNU 12.3.0",
    "compiler_flags": "-O3 -DNDEBUG",
    "build_type": "Release",
    "hdf5_version": "1.14.3",
    "filesystem": "gpfs",
    "slurm_partition": "iter",
    "commit_sha": "7246824127a3339e6864f91238eb434a8ebb2afc",
    "commit_dirty": false
  }
}
```

(the object above nests under `context.imas_baseline_metadata`; only that
field is shown, not the full committed file)

`imas_baseline_metadata` is what makes two files comparable — or tells you why
they aren't (different `compiler_flags`, different `filesystem`, etc). See
[`../BASELINE_CAPTURE.md`](../BASELINE_CAPTURE.md) for how these files are
produced.

## Naming

- `<machine-id>` is whatever `--machine-id` was passed to
  `capture_baseline.py` (default: the sanitized hostname). Use a stable,
  descriptive id for anything meant to be a measurement of record, e.g.
  `sdcc-gpfs` for the ITER SDCC cluster, rather than a raw login-node
  hostname that might change.
- `<commit-sha>` is the full `git rev-parse HEAD` of the run. A dirty working
  tree is still captured (for laptop iteration) but is flagged
  `commit_dirty: true` in the metadata — that file is not reproducible from
  the SHA alone and should not be treated as a measurement of record.

## Comparing two baselines

There is no comparison tool here by design (PRD #50: "Not tested: the
numeric results themselves... a manual/analysis step over the committed
JSON, not a CI gate"). To compare, match `benchmarks[].name` between two
files and look at the `_mean`/`_median` rows' `real_time`/`cpu_time`, e.g.
with `jq`:

```sh
jq '.benchmarks[] | select(.name | endswith("_mean")) | {name, real_time, cpu_time}' \
  benchmarks/baselines/sdcc-gpfs/<old-sha>.json

jq '.benchmarks[] | select(.name | endswith("_mean")) | {name, real_time, cpu_time}' \
  benchmarks/baselines/sdcc-gpfs/<new-sha>.json
```
