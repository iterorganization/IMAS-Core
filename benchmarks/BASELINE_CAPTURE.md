# Capturing a baseline

This documents the repeatable procedure behind `capture_baseline.py` (issue
#55): turning a run of `al_benchmarks` into a metadata-tagged, committable
JSON artifact under [`benchmarks/baselines/`](baselines/README.md).

The script wraps whatever benchmarks are registered — it does not know or
care which scenarios exist, so this procedure does not change as
`bench_*.cpp` grows beyond the current whole-IDS-read operation.

## Laptop (development / smoke — throwaway numbers)

Any machine, any state of the working tree. These numbers exist to prove the
capture flow works, never to be compared against anything.

```sh
cmake --preset benchmarks
cmake --build --preset benchmarks

python3 benchmarks/capture_baseline.py \
  --binary build-benchmarks/benchmarks/al_benchmarks \
  --machine-id "laptop-$(hostname -s)" \
  -- --benchmark_min_time=1x
```

`--benchmark_min_time=1x` caps every registered benchmark at one iteration —
the same flag the CTest smoke check (`benchmark_smoke_run`) uses — so this
finishes in well under a second regardless of how many scenarios exist.
Drop it to get real repetition counts (slower, and still not a measurement
of record without the cluster's isolation).

The script prints the metadata it captured (OS, CPU, compiler, HDF5 version,
filesystem, commit SHA/dirty flag, `slurm_partition: null`) and the path it
wrote to. On a laptop `commit_dirty` will often be `true` — that's expected
and is exactly why this file is a smoke artifact, not a baseline of record.

## ITER SDCC cluster (measurement of record)

Run on a clean checkout of `develop` at a known commit, before any P1–P6
hot-path change lands (PRD #50: the baseline becomes unrecoverable the
moment optimization starts).

1. **Allocate an exclusive node** on the partition your supervisor names as
   production-representative — the partition is a runtime parameter, never
   hard-coded into the script or this doc:

   ```sh
   salloc --exclusive --partition=<PARTITION> --time=00:30:00
   ```

   `$SLURM_JOB_PARTITION` is then set for the rest of the allocation, and
   `capture_baseline.py` picks it up automatically (or pass
   `--slurm-partition <PARTITION>` explicitly).

2. **Load the toolchain modules**:

   ```sh
   module load foss-2023b googletest
   ```

   `foss-2023b` provides the compiler and the HDF5 the benchmark links
   against; `googletest` is needed because the harness links
   `al_contract_fixtures` (shared with the contract suite).

3. **Configure and build in Release**, on GPFS (`/mnt/HPC_T2`), not on a
   login-node home directory:

   ```sh
   cd /mnt/HPC_T2/<your-workspace>/IMAS-Core
   cmake --preset benchmarks
   cmake --build --preset benchmarks
   ```

4. **Capture**, recording the filesystem explicitly (auto-detection is
   best-effort and GPFS mounts don't always self-report cleanly) and a
   `machine-id` that names the cluster, not the specific login/compute node:

   ```sh
   python3 benchmarks/capture_baseline.py \
     --binary build-benchmarks/benchmarks/al_benchmarks \
     --machine-id sdcc-gpfs \
     --filesystem gpfs
   ```

   No `--benchmark_min_time` here — let each scenario run its full
   repetition count so the committed baseline reports a real distribution
   (median, mean, stddev), per PRD #50 user story 8.

5. **Verify before committing**: the script refuses (unless `--force`) to
   overwrite a baseline already committed for this `machine-id` + commit SHA,
   and warns to stderr if the working tree is dirty or the build isn't
   Release. Resolve any such warning rather than force past it for a
   measurement of record.

6. **Commit** the resulting file under `benchmarks/baselines/sdcc-gpfs/` —
   see [`baselines/README.md`](baselines/README.md) for the layout and how
   later runs compare against it.

## What gets recorded either way

`captured_at`, `machine_id`, `os`, `cpu_model`, `compiler` (+ version),
`compiler_flags`, `build_type`, `hdf5_version`, `filesystem`,
`slurm_partition` (`null` off-cluster), `commit_sha`, `commit_dirty` — spliced
into the Google Benchmark JSON's `context` block as `imas_baseline_metadata`.
Compiler/flags/HDF5 version are read from the build directory's
`CMakeCache.txt` (found by walking up from `--binary`, or pass `--build-dir`
explicitly if your build layout doesn't nest the binary under it).
