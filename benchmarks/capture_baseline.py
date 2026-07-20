#!/usr/bin/env python3
"""Run the al_benchmarks binary and capture a metadata-tagged baseline artifact.

Wraps whatever Google Benchmark JSON the harness (benchmarks/al_benchmarks,
issues #52/#53) emits and tags it with the comparability metadata a committed
baseline needs (NORTH_STAR.md §8.4, issue #55): OS, CPU model, compiler +
flags, HDF5 version, filesystem, SLURM partition, and commit SHA. The tagged
JSON is written to the committed-artifact layout, keyed by machine identifier
and commit SHA:

    benchmarks/baselines/<machine-id>/<commit-sha>.json

The SLURM partition is never hard-coded: pass --slurm-partition, or run under
an allocation that sets $SLURM_JOB_PARTITION, or leave both unset (recorded as
null -- e.g. a laptop smoke run has no partition at all).

Comparing two baselines (dividing matching "real_time"/"cpu_time" entries) is
a manual analysis step over the committed JSON, not something this script
does (PRD #50 "Not tested: the numeric results themselves").
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import re
import socket
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUTPUT_DIR = REPO_ROOT / "benchmarks" / "baselines"


def run_git(*args: str) -> str:
    return subprocess.check_output(
        ["git", *args], cwd=REPO_ROOT, text=True
    ).strip()


def commit_metadata() -> dict:
    sha = run_git("rev-parse", "HEAD")
    dirty = bool(run_git("status", "--porcelain"))
    return {"commit_sha": sha, "commit_dirty": dirty}


def sanitize_machine_id(raw: str) -> str:
    slug = re.sub(r"[^a-zA-Z0-9]+", "-", raw).strip("-").lower()
    return slug or "unknown-machine"


def best_effort_output(cmd: list[str]) -> str | None:
    """Run `cmd`, returning stripped stdout, or None if it can't be run."""
    try:
        return subprocess.check_output(
            cmd, text=True, stderr=subprocess.STDOUT
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def detect_os() -> str:
    return platform.platform()


def detect_cpu_model() -> str:
    system = platform.system()
    if system == "Darwin":
        brand = best_effort_output(["sysctl", "-n", "machdep.cpu.brand_string"])
        if brand:
            return brand
    elif system == "Linux":
        try:
            with open("/proc/cpuinfo") as f:
                for line in f:
                    if line.lower().startswith("model name"):
                        return line.split(":", 1)[1].strip()
        except OSError:
            pass
    return platform.processor() or "unknown"


def read_cmake_cache(build_dir: Path) -> dict[str, str]:
    cache_path = build_dir / "CMakeCache.txt"
    entries = {}
    if not cache_path.is_file():
        return entries
    for line in cache_path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("//"):
            continue
        if ":" not in line or "=" not in line:
            continue
        key_type, value = line.split("=", 1)
        key = key_type.split(":", 1)[0]
        entries[key] = value
    return entries


def find_build_dir(binary: Path, explicit: Path | None) -> Path | None:
    if explicit is not None:
        return explicit
    for candidate in binary.resolve().parents:
        if (candidate / "CMakeCache.txt").is_file():
            return candidate
    return None


def detect_compiler(cache: dict[str, str]) -> str:
    compiler_id = cache.get("CMAKE_CXX_COMPILER_ID")
    compiler_version = cache.get("CMAKE_CXX_COMPILER_VERSION")
    if compiler_id and compiler_version:
        return f"{compiler_id} {compiler_version}"
    compiler_path = cache.get("CMAKE_CXX_COMPILER")
    if compiler_path:
        version_output = best_effort_output([compiler_path, "--version"])
        if version_output:
            return version_output.splitlines()[0]
    return "unknown"


def detect_compiler_flags(cache: dict[str, str]) -> str:
    build_type = cache.get("CMAKE_BUILD_TYPE", "")
    parts = [cache.get("CMAKE_CXX_FLAGS", "")]
    if build_type:
        parts.append(cache.get(f"CMAKE_CXX_FLAGS_{build_type.upper()}", ""))
    return " ".join(p for p in parts if p).strip() or "unknown"


def detect_hdf5_version(cache: dict[str, str]) -> str:
    hdf5_dir = cache.get("HDF5_DIR")
    if hdf5_dir:
        version_file = Path(hdf5_dir) / "hdf5-config-version.cmake"
        if version_file.is_file():
            match = re.search(
                r'set\(PACKAGE_VERSION\s+"([^"]+)"\)', version_file.read_text()
            )
            if match:
                return match.group(1)
    version_output = best_effort_output(["h5dump", "--version"])
    if version_output:
        match = re.search(r"Version\s+([0-9.]+)", version_output)
        if match:
            return match.group(1)
    return "unknown"


def detect_filesystem(path: Path, override: str | None) -> str:
    if override:
        return override
    if platform.system() == "Linux":
        fstype = best_effort_output(["findmnt", "-no", "FSTYPE", "--target", str(path)])
        if fstype:
            return fstype
    elif platform.system() == "Darwin":
        df_output = best_effort_output(["df", "-P", str(path)])
        mount_output = best_effort_output(["mount"])
        if df_output and mount_output:
            device = df_output.splitlines()[-1].split()[0]
            for line in mount_output.splitlines():
                if line.startswith(device + " "):
                    match = re.search(r"\(([^,]+)", line)
                    if match:
                        return match.group(1)
    return "unknown"


def build_metadata(args: argparse.Namespace, build_dir: Path | None) -> dict[str, object]:
    cache = read_cmake_cache(build_dir) if build_dir else {}
    metadata = {
        "captured_at": datetime.now(timezone.utc).isoformat(),
        "machine_id": args.machine_id,
        "os": detect_os(),
        "cpu_model": detect_cpu_model(),
        "compiler": detect_compiler(cache),
        "compiler_flags": detect_compiler_flags(cache),
        "build_type": cache.get("CMAKE_BUILD_TYPE", "unknown"),
        "hdf5_version": detect_hdf5_version(cache),
        "filesystem": detect_filesystem(
            build_dir or REPO_ROOT, args.filesystem
        ),
        "slurm_partition": args.slurm_partition,
    }
    metadata.update(commit_metadata())
    return metadata


def run_benchmark(binary: Path, extra_args: list[str], out_path: Path) -> str | None:
    """Run the benchmark binary; return an error message, or None on success."""
    try:
        subprocess.run(
            [
                str(binary),
                f"--benchmark_out={out_path}",
                "--benchmark_out_format=json",
                *extra_args,
            ],
            check=True,
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        return f"failed to run {binary}: {exc}"
    return None


def merge_metadata(json_path: Path, metadata: dict[str, object]) -> dict:
    result = json.loads(json_path.read_text())
    result.setdefault("context", {})["imas_baseline_metadata"] = metadata
    return result


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="capture_baseline.py",
        description=(
            "Run al_benchmarks and write a metadata-tagged, committable "
            "baseline artifact under benchmarks/baselines/."
        ),
    )
    parser.add_argument(
        "--binary", required=True, type=Path, help="path to the al_benchmarks executable"
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=None,
        help="CMake build dir to read compiler/flags/HDF5 metadata from "
        "(default: discovered by walking up from --binary for CMakeCache.txt)",
    )
    parser.add_argument(
        "--machine-id",
        default=sanitize_machine_id(socket.gethostname()),
        help="identifier this baseline is filed under (default: sanitized hostname)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT_DIR,
        help="root of the committed baseline layout (default: benchmarks/baselines)",
    )
    parser.add_argument(
        "--filesystem",
        default=None,
        help="filesystem name to record (default: best-effort auto-detect, "
        "e.g. pass 'gpfs' explicitly on the cluster)",
    )
    parser.add_argument(
        "--slurm-partition",
        default=None,
        help="SLURM partition to record (default: $SLURM_JOB_PARTITION, or null "
        "if unset -- this is always a runtime parameter, never hard-coded)",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="overwrite an existing committed baseline file for this machine+commit",
    )
    parser.add_argument(
        "benchmark_args",
        nargs=argparse.REMAINDER,
        help="remaining args forwarded verbatim to al_benchmarks "
        "(e.g. -- --benchmark_filter=Equilibrium --benchmark_min_time=1x)",
    )
    args = parser.parse_args(argv)
    if args.benchmark_args[:1] == ["--"]:
        args.benchmark_args = args.benchmark_args[1:]
    if args.slurm_partition is None:
        args.slurm_partition = os.environ.get("SLURM_JOB_PARTITION") or None
    return args


def main(argv: list[str]) -> int:
    args = parse_args(argv)

    if not args.binary.is_file():
        print(f"error: benchmark binary not found: {args.binary}", file=sys.stderr)
        return 1

    build_dir = find_build_dir(args.binary, args.build_dir)
    if build_dir is None:
        print(
            "warning: could not locate a CMakeCache.txt; compiler/flags/HDF5 "
            "metadata will be recorded as 'unknown' (pass --build-dir to fix this)",
            file=sys.stderr,
        )

    metadata = build_metadata(args, build_dir)

    if metadata["commit_dirty"]:
        print(
            "warning: working tree is dirty -- this baseline is not reproducible "
            "from the recorded commit SHA alone (metadata still records commit_dirty=true)",
            file=sys.stderr,
        )
    if metadata["build_type"] != "Release":
        print(
            f"warning: build type is '{metadata['build_type']}', not Release -- "
            "NORTH_STAR.md §8.4 numbers are only comparable across Release builds",
            file=sys.stderr,
        )

    machine_dir = args.output_dir / args.machine_id
    out_path = machine_dir / f"{metadata['commit_sha']}.json"
    if out_path.exists() and not args.force:
        print(
            f"error: {out_path} already exists (pass --force to overwrite a "
            "committed baseline)",
            file=sys.stderr,
        )
        return 1

    with tempfile.TemporaryDirectory() as tmp:
        raw_json = Path(tmp) / "benchmark_raw.json"
        error = run_benchmark(args.binary, args.benchmark_args, raw_json)
        if error is not None:
            print(f"error: {error}", file=sys.stderr)
            return 1
        tagged = merge_metadata(raw_json, metadata)

    machine_dir.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(tagged, indent=2) + "\n")

    print(f"wrote baseline: {out_path}")
    width = max(len(key) for key in metadata)
    for key, value in metadata.items():
        print(f"  {key:<{width}} = {value}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
