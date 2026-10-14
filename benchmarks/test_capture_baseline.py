#!/usr/bin/env python3
"""Hermetic tests for the baseline-capture command-line workflow."""

from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SCRIPT = Path(__file__).with_name("capture_baseline.py")
SPEC = importlib.util.spec_from_file_location("capture_baseline", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
capture_baseline = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(capture_baseline)


class CaptureBaselineTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        self.binary = self.root / "fake_benchmark.py"
        self.binary.write_text(
            "#!/usr/bin/env python3\n"
            "import json\n"
            "import sys\n"
            "from pathlib import Path\n"
            "out = next(arg.split('=', 1)[1] for arg in sys.argv "
            "if arg.startswith('--benchmark_out='))\n"
            "Path(out).write_text(json.dumps({'context': {'source': 'fake'}, "
            "'benchmarks': []}))\n"
        )
        self.binary.chmod(0o755)

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def test_capture_tags_json_and_protects_machine_commit_artifact(self) -> None:
        output_dir = self.root / "baselines"
        metadata = {
            "captured_at": "2026-07-20T10:00:00+00:00",
            "machine_id": "machine-a",
            "os": "test-os",
            "cpu_model": "test-cpu",
            "compiler": "test-compiler",
            "compiler_flags": "-O3",
            "build_type": "Release",
            "hdf5_version": "1.14.3",
            "filesystem": "testfs",
            "slurm_partition": None,
            "commit_sha": "deadbeef",
            "commit_dirty": False,
        }
        common_args = [
            "--binary", str(self.binary),
            "--machine-id", "../Machine A",
            "--output-dir", str(output_dir),
        ]
        args = [
            *common_args,
            "--", "--benchmark_min_time=1x",
        ]

        with mock.patch.object(capture_baseline, "build_metadata", return_value=metadata):
            self.assertEqual(capture_baseline.main(args), 0)

            artifact = output_dir / "machine-a" / "deadbeef.json"
            self.assertTrue(artifact.is_file())
            captured = json.loads(artifact.read_text())
            self.assertEqual(captured["context"]["source"], "fake")
            self.assertEqual(captured["context"]["imas_baseline_metadata"], metadata)

            self.assertEqual(capture_baseline.main(args), 1)
            self.assertEqual(
                capture_baseline.main([*common_args, "--force", "--", "--benchmark_min_time=1x"]),
                0,
            )

    def test_detect_cpu_model_reads_apple_silicon_chip(self) -> None:
        with mock.patch.object(capture_baseline.platform, "system", return_value="Darwin"), \
             mock.patch.object(
                 capture_baseline,
                 "best_effort_output",
                 side_effect=[None, "Hardware:\n    Chip: Apple M5 Pro\n"],
             ):
            self.assertEqual(capture_baseline.detect_cpu_model(), "Apple M5 Pro")

    def test_explicit_machine_id_is_sanitized(self) -> None:
        args = capture_baseline.parse_args(
            ["--binary", str(self.binary), "--machine-id", "../../outside"]
        )
        self.assertEqual(args.machine_id, "outside")


if __name__ == "__main__":
    unittest.main()
