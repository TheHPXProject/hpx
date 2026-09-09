#!/usr/bin/env python3

# Copyright (c) 2026 Anshuman Agrawal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

"""Exercise LSU entry.sh in a shared workspace without Slurm or network access."""

import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import unittest


JENKINS = Path(__file__).resolve().parents[1]
BASH = os.environ.get("HPX_TEST_BASH", os.environ.get("BASH", "bash"))
SUFFIXES = (
    ".out",
    ".err",
    "-ctest-status.txt",
    "-cdash-build-id.txt",
    "-cdash-submission.txt",
)

SBATCH_STUB = r'''
import json
import os
from pathlib import Path
import time

lane = os.environ["configuration_name_with_build_type"]
prefix = "jenkins-hpx-" + lane
snapshot = {p.name: p.read_text() for p in Path(".").iterdir() if p.is_file()}
Path("snapshot-" + lane + ".json").write_text(json.dumps(snapshot))
for suffix, content in (
    (".out", "stdout " + lane),
    (".err", "stderr " + lane),
    ("-ctest-status.txt", "0"),
    ("-cdash-build-id.txt", "123"),
    ("-cdash-submission.txt", "submitted " + lane),
):
    Path(prefix + suffix).write_text(content)
Path("ready-" + lane).touch()
if os.environ.get("HOLD_LANE") == lane:
    deadline = time.monotonic() + 15
    while not Path("release-" + lane).exists():
        if time.monotonic() >= deadline:
            raise SystemExit("Timed out waiting for the other matrix lane")
        time.sleep(0.01)
print("12345")
'''


class MatrixArtifactsTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.workspace = Path(self.temporary.name)
        lsu = self.workspace / ".jenkins" / "lsu"
        lsu.mkdir(parents=True)
        shutil.copy2(JENKINS / "lsu" / "entry.sh", lsu)
        common = self.workspace / ".jenkins" / "common"
        shutil.copytree(JENKINS / "common", common)
        status = common / "set_github_status.sh"
        status.write_text("#!/bin/sh\nexit 0\n")
        status.chmod(0o755)
        for configuration in ("gcc-12", "gcc-12-cuda-12"):
            (lsu / f"slurm-configuration-{configuration}.sh").write_text(
                "configuration_slurm_num_nodes=1\n"
                "configuration_slurm_partition=fixture\n"
            )
        self.bin = self.workspace / "bin"
        self.bin.mkdir()
        self.stub("sbatch", f"#!{sys.executable}\n" + SBATCH_STUB)
        for command in ("sleep", "scancel", "squeue"):
            self.stub(command, "#!/bin/sh\nexit 0\n")
        self.environment = {
            "PATH": str(self.bin) + os.pathsep + os.environ["PATH"],
            "HOME": str(self.workspace),
            "GIT_BRANCH": "origin/fixture",
            "GIT_COMMIT": "fixture-commit",
            "GITHUB_TOKEN": "fixture-token",
        }

    def stub(self, command, content):
        script = self.bin / command
        script.write_text(content)
        script.chmod(0o755)

    def start_lane(self, configuration, build_type, hold=False):
        environment = dict(self.environment)
        environment.update(configuration_name=configuration, build_type=build_type)
        lane = f"{configuration}-{build_type.lower()}"
        if hold:
            environment["HOLD_LANE"] = lane
        process = subprocess.Popen(
            [BASH, ".jenkins/lsu/entry.sh"],
            cwd=self.workspace,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            start_new_session=True,
        )
        self.addCleanup(self.stop_process, process)
        return process

    @staticmethod
    def stop_process(process):
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGTERM)
        process.communicate()

    def assert_success(self, process):
        stdout, stderr = process.communicate(timeout=20)
        self.assertEqual(process.returncode, 0, stdout + stderr)

    def seed_lane(self, lane):
        files = {f"jenkins-hpx-{lane}{suffix}": "old" for suffix in SUFFIXES}
        for name, content in files.items():
            (self.workspace / name).write_text(content)
        return files

    def test_cleanup_removes_only_exact_current_lane_artifacts(self):
        current = self.seed_lane("gcc-12-release")
        retained = {}
        for lane in (
            "gcc-12-debug",
            "gcc-12-cuda-12-release",
            "gcc-12-release-extra",
            "clang-17-release",
        ):
            retained.update(self.seed_lane(lane))
        for name in (
            "jenkins-hpx-gcc-12-release.out.backup",
            "jenkins-hpx-unrelated",
            "gcc-12-Testing",
            "other-Testing",
            "unrelated.txt",
        ):
            retained[name] = "keep"
            (self.workspace / name).write_text("keep")
        testing = self.workspace / "build/gcc-12-debug/Testing"
        testing.mkdir(parents=True)
        (testing / "TAG").write_text("keep")

        self.assert_success(self.start_lane("gcc-12", "Release"))

        snapshot = json.loads(
            (self.workspace / "snapshot-gcc-12-release.json").read_text()
        )
        for name in current:
            self.assertNotIn(name, snapshot, name)
        for name, content in retained.items():
            self.assertEqual(snapshot.get(name), content, name)
            self.assertEqual((self.workspace / name).read_text(), content)
        self.assertEqual((testing / "TAG").read_text(), "keep")

    def test_starting_another_lane_preserves_running_lane_outputs(self):
        first_lane = "gcc-12-cuda-12-release"
        first = self.start_lane("gcc-12-cuda-12", "Release", hold=True)
        ready = self.workspace / f"ready-{first_lane}"
        deadline = time.monotonic() + 10
        while not ready.exists():
            if first.poll() is not None or time.monotonic() >= deadline:
                self.fail("First lane did not reach the stubbed Slurm job")
            time.sleep(0.01)
        expected = {
            suffix: (self.workspace / f"jenkins-hpx-{first_lane}{suffix}")
            .read_text()
            for suffix in SUFFIXES
        }

        self.assert_success(self.start_lane("gcc-12", "Debug"))
        for suffix, content in expected.items():
            path = self.workspace / f"jenkins-hpx-{first_lane}{suffix}"
            self.assertTrue(path.exists(), str(path))
            self.assertEqual(path.read_text(), content)
        (self.workspace / f"release-{first_lane}").touch()
        self.assert_success(first)


if __name__ == "__main__":
    unittest.main()
