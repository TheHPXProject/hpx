#!/usr/bin/env python3
# Copyright (c) 2026 Anshuman Agrawal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

"""Check report publication decisions without Slurm or network access."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "lsu-perftests/entry.sh"


class PerformanceResults(unittest.TestCase):
    def run_entry(self, *, slurm=0, status=None, report=False, stale=False,
                  pull_request=True):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            lane = root / ".jenkins/lsu-perftests"
            common = root / ".jenkins/common"
            lane.mkdir(parents=True)
            common.mkdir()
            shutil.copyfile(SCRIPT, lane / "entry.sh")
            (lane / "slurm-constraint-perftests.sh").write_text(
                "configuration_slurm_partition=test\n"
                "configuration_slurm_nodelist=test\n")
            (common / "slurm.sh").write_text('''
hpx_slurm_cancel_previous() { return 0; }
hpx_slurm_run() {
    if [[ -n "${BUILD_STATUS}" ]]; then
        echo "${BUILD_STATUS}" > jenkins-hpx-perftests-ctest-status.txt
    fi
    if [[ "${CREATE_REPORT}" == 1 ]]; then
        mkdir -p perftests-reports/reference-comparison
        echo current > perftests-reports/reference-comparison/index.html
    fi
    return "${SLURM_STATUS}"
}
''')
            comment = lane / "comment_github.sh"
            comment.write_text("#!/bin/sh\ntouch comment-called\n")
            comment.chmod(0o755)
            sleep = root / "sleep"
            sleep.write_text("#!/bin/sh\nexit 0\n")
            sleep.chmod(0o755)
            if stale:
                (root / "jenkins-hpx-perftests-ctest-status.txt").write_text("1")
                old_report = root / "perftests-reports/reference-comparison"
                old_report.mkdir(parents=True)
                (old_report / "index.html").write_text("previous run")
            env = dict(os.environ, PATH=str(root) + ":" + os.environ["PATH"],
                       configuration_name="perftests", GIT_BRANCH="origin/master",
                       BUILD_STATUS="" if status is None else str(status),
                       SLURM_STATUS=str(slurm), CREATE_REPORT=str(int(report)),
                       ghprbPullId="123" if pull_request else "")
            result = subprocess.run(
                [os.environ.get("HPX_TEST_BASH", "bash"), str(lane / "entry.sh")],
                cwd=root, env=env, capture_output=True, text=True, timeout=10)
            return result.returncode, (root / "comment-called").exists()

    def test_timeout_without_results_does_not_comment(self):
        self.assertEqual(self.run_entry(slurm=124), (124, False))

    def test_timeout_does_not_publish_stale_results(self):
        self.assertEqual(self.run_entry(slurm=124, stale=True), (124, False))

    def test_success_does_not_comment(self):
        self.assertEqual(self.run_entry(status=0, report=True), (0, False))

    def test_failed_comparison_publishes_its_report(self):
        self.assertEqual(self.run_entry(slurm=1, status=1, report=True), (1, True))

    def test_build_failure_without_report_does_not_comment(self):
        self.assertEqual(self.run_entry(slurm=1, status=1), (1, False))

    def test_branch_failure_does_not_comment_on_a_pull_request(self):
        self.assertEqual(self.run_entry(slurm=1, status=1, report=True,
                                        pull_request=False), (1, False))

    def test_missing_status_is_a_failure(self):
        self.assertEqual(self.run_entry(), (1, False))


if __name__ == "__main__":
    unittest.main()
