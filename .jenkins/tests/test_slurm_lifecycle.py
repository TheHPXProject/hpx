#!/usr/bin/env python3
# Copyright (c) 2026 Anshuman Agrawal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

"""Run with Python 3; all Slurm commands are private executable stubs.

GNU timeout and Bash 4+ must be on PATH. Set HPX_TEST_BASH to select Bash.
"""

import json
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
BASH = os.environ.get("HPX_TEST_BASH", "bash")
STUB = r'''
import json
import os
from pathlib import Path
import signal
import sys
import time

name = Path(sys.argv[0]).name
with open(os.environ["CALL_LOG"], "a") as log:
    log.write(json.dumps([name] + sys.argv[1:]) + "\n")
if name == "sbatch":
    if os.environ.get("ARTIFACT_PREFIX"):
        prefix = os.environ["ARTIFACT_PREFIX"]
        for suffix in (".out", ".err", "-cdash-submission.txt",
                       "-cdash-build-id.txt", "-ctest-status.txt"):
            Path(prefix + suffix).write_text("0\n")
    if os.environ.get("IGNORE_TERM"):
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
    time.sleep(float(os.environ.get("SUBMIT_DELAY", "0")))
    if os.environ.get("JOB_ID", "12345"):
        print(os.environ.get("JOB_ID", "12345"), flush=True)
    Path(os.environ["READY"]).write_text(str(time.monotonic()))
    time.sleep(float(os.environ.get("JOB_DELAY", "0")))
    sys.exit(int(os.environ.get("JOB_EXIT", "0")))
if name == "scancel":
    time.sleep(float(os.environ.get("CANCEL_DELAY", "0")))
    sys.exit(int(os.environ.get("CANCEL_EXIT", "0")))
if name == "squeue":
    time.sleep(float(os.environ.get("QUEUE_DELAY", "0")))
    if any(arg.startswith("--jobs=") for arg in sys.argv):
        # sbatch prints the job ID just before it writes READY, so a query
        # can land in between. Treat that as a job submitted this moment.
        try:
            submitted = float(Path(os.environ["READY"]).read_text())
        except (FileNotFoundError, ValueError):
            submitted = time.monotonic()
        pending = float(os.environ.get("PENDING_DELAY", "0"))
        print("PENDING" if time.monotonic() - submitted < pending else
              os.environ.get("JOB_STATE", "RUNNING"))
    elif os.environ.get("QUEUE_JOBS"):
        print("12345")
    sys.exit(int(os.environ.get("QUEUE_EXIT", "0")))
'''


class SlurmLifecycle(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)
        for command in ("sbatch", "scancel", "squeue"):
            stub = self.path / command
            stub.write_text("#!" + sys.executable + "\n" + STUB)
            stub.chmod(0o755)
        self.log = self.path / "calls.jsonl"
        self.ready = self.path / "ready"
        self.env = dict(os.environ, PATH=str(self.path) + ":" + os.environ["PATH"],
                        CALL_LOG=str(self.log), READY=str(self.ready),
                        TMPDIR=str(self.path), HPX_SLURM_POLL_INTERVAL="1")

    def start(self, command='hpx_slurm_run 10s --job-name=lane batch.sh', **env):
        self.env.update(env)
        script = ('set -eu\nsource ' +
                  shlex.quote(str(ROOT / ".jenkins/common/slurm.sh")) +
                  '\n' + command)
        process = subprocess.Popen([BASH, "-c", script], env=self.env,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                start_new_session=True, cwd=self.path)
        self.addCleanup(self.stop, process)
        return process

    @staticmethod
    def stop(process):
        # This test owns the session, including timeout and any stub children.
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.communicate(timeout=3)

    def finish(self, process, timeout=12):
        try:
            out, err = process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
            self.fail("Slurm helper did not finish within its test bound")
        self.assertFalse(list(self.path.glob("tmp.*")), "temporary ID file leaked")
        return process.returncode, out.decode(), err.decode()

    def calls(self, name):
        lines = self.log.read_text().splitlines() if self.log.exists() else []
        return [row[1:] for row in map(json.loads, lines) if row[0] == name]

    def wait_ready(self):
        deadline = time.monotonic() + 3
        while not self.ready.exists() and time.monotonic() < deadline:
            time.sleep(0.01)
        self.assertTrue(self.ready.exists(), "stub submission never completed")

    def start_entry(self, lane, exclude=None, **env):
        jenkins = self.path / ".jenkins"
        target = jenkins / lane
        target.mkdir(parents=True, exist_ok=True)
        (jenkins / "common").mkdir(exist_ok=True)
        for relative in (lane + "/entry.sh", "common/slurm.sh"):
            shutil.copyfile(ROOT / ".jenkins" / relative, jenkins / relative)
        config = ("slurm-configuration-test.sh" if lane == "lsu" else
                  "slurm-constraint-test.sh")
        (target / config).write_text(
            'configuration_slurm_num_nodes=1\n'
            'configuration_slurm_partition=test\n'
            'configuration_slurm_nodelist=test\n' +
            ('' if exclude is None else
             'configuration_slurm_exclude=' + exclude + '\n'))
        status_script = jenkins / "common/set_github_status.sh"
        status_script.write_text(
            '#!/bin/sh\necho "$4" >> ' +
            shlex.quote(str(self.path / "statuses")) + '\n')
        status_script.chmod(0o755)
        comment_script = target / "comment_github.sh"
        comment_script.write_text("#!/bin/sh\nexit 0\n")
        comment_script.chmod(0o755)
        # No downloads, unpacking, or randomized entry delays in this fixture.
        for command in ("wget", "sha256sum", "tar", "sleep"):
            stub = self.path / command
            stub.write_text("#!/bin/sh\nexit 0\n")
            stub.chmod(0o755)
        (self.path / "grcov").touch()
        (self.path / "grcov.tar.bz2").touch()
        prefix = "jenkins-hpx-test" + ("-debug" if lane == "lsu" else "")
        self.env.update(configuration_name="test", build_type="Debug",
                        GIT_BRANCH="origin/master", GIT_COMMIT="fixture",
                        GITHUB_TOKEN="fixture-unused", ARTIFACT_PREFIX=prefix)
        self.env.update(env)
        process = subprocess.Popen([BASH, str(target / "entry.sh")], env=self.env,
                                cwd=self.path, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, start_new_session=True)
        self.addCleanup(self.stop, process)
        return process

    def test_entries_preserve_success(self):
        for lane in ("lsu", "lsu-perftests", "lsu-test-coverage"):
            with self.subTest(lane=lane):
                code, _, err = self.finish(self.start_entry(lane))
                self.assertEqual(code, 0, err)

    def test_entries_pass_the_configured_node_exclusion(self):
        for lane in ("lsu", "lsu-test-coverage"):
            for exclude, expected in ((None, []),
                                      ("node7,node9", ["--exclude=node7,node9"])):
                with self.subTest(lane=lane, exclude=exclude):
                    self.log.unlink(missing_ok=True)
                    code, _, err = self.finish(
                        self.start_entry(lane, exclude=exclude))
                    self.assertEqual(code, 0, err)
                    sbatch = self.calls("sbatch")[-1]
                    self.assertEqual([arg for arg in sbatch
                                      if arg.startswith("--exclude")], expected)

    def test_entries_run_when_previous_jobs_cannot_be_cleared(self):
        uid = str(os.getuid())
        pull_requests = (
            ("lsu", "jenkins-hpx-7-test-debug",
             dict(ghprbPullId="7", ghprbActualCommit="fixture",
                  ghprbPullLink="https://github.com/org/repo/pull/7")),
            ("lsu-perftests", "jenkins-hpx-7-test", dict(ghprbPullId="7")),
            ("lsu-test-coverage", "jenkins-hpx-7-test", dict(CHANGE_ID="7")),
        )
        for lane, job_name, env in pull_requests:
            with self.subTest(lane=lane):
                self.log.unlink(missing_ok=True)
                code, _, err = self.finish(
                    self.start_entry(lane, CANCEL_EXIT="8", **env))
                self.assertEqual(code, 0, err)
                self.assertIn("starting this build anyway", err)
                self.assertEqual(self.calls("scancel"),
                                 [["--user=" + uid, "--name=" + job_name]])
                self.assertEqual(len(self.calls("sbatch")), 1)
        # Only the build matrix posts commit statuses from its entry script.
        self.assertEqual((self.path / "statuses").read_text(), "success\n")

    def test_entries_reject_success_sentinel_after_slurm_failure(self):
        for lane in ("lsu", "lsu-perftests", "lsu-test-coverage"):
            with self.subTest(lane=lane):
                code, _, err = self.finish(self.start_entry(lane, JOB_EXIT="42"))
                self.assertEqual(code, 42, err)

    def test_entries_timeout_cancels_owned_job(self):
        for lane in ("lsu", "lsu-perftests", "lsu-test-coverage"):
            with self.subTest(lane=lane):
                code, _, err = self.finish(self.start_entry(
                    lane, JOB_DELAY="30", HPX_SLURM_TIMEOUT="1s"))
                self.assertEqual(code, 124, err)
        self.assertEqual(self.calls("scancel"), [["12345"]] * 3)

    def test_entries_abort_cancels_owned_job(self):
        for lane in ("lsu", "lsu-perftests", "lsu-test-coverage"):
            with self.subTest(lane=lane):
                self.ready.unlink(missing_ok=True)
                process = self.start_entry(lane, JOB_DELAY="30")
                self.wait_ready()
                process.send_signal(signal.SIGTERM)
                code, _, err = self.finish(process)
                self.assertEqual(code, 143, err)
        self.assertEqual(self.calls("scancel"), [["12345"]] * 3)

    def test_invalid_timeout_cannot_disable_bound(self):
        for limit in ("0", "0s", "-1h", "invalid"):
            with self.subTest(limit=limit):
                code, _, _ = self.finish(self.start(
                    "hpx_slurm_run " + limit + " batch.sh"))
                self.assertEqual(code, 2)
        self.assertEqual(self.calls("sbatch"), [])

    def test_success(self):
        code, _, _ = self.finish(self.start())
        self.assertEqual(code, 0)
        self.assertEqual(self.calls("scancel"), [])
        self.assertEqual(self.calls("sbatch")[0][:2], ["--parsable", "--wait"])

    def test_job_failure_preserved(self):
        code, _, _ = self.finish(self.start(JOB_EXIT="42"))
        self.assertEqual(code, 42)
        self.assertEqual(self.calls("scancel"), [["12345"]])

    def test_submission_failure_has_no_name_fallback(self):
        code, _, err = self.finish(self.start(JOB_ID="", JOB_EXIT="9"))
        self.assertEqual(code, 9)
        self.assertIn("No Slurm job ID", err)
        self.assertEqual(self.calls("scancel"), [])

    def test_missing_or_invalid_id_is_not_success(self):
        for job_id in ("", "0", "bad;id", "123;cluster;extra"):
            with self.subTest(job_id=job_id):
                code, _, _ = self.finish(self.start(JOB_ID=job_id))
                self.assertEqual(code, 1)
        self.assertEqual(self.calls("scancel"), [])

    def test_runtime_timeout(self):
        code, _, _ = self.finish(self.start(
            'hpx_slurm_run 1s batch.sh', JOB_DELAY="30"))
        self.assertEqual(code, 124)
        self.assertEqual(self.calls("scancel"), [["12345"]])

    def test_queue_wait_does_not_consume_runtime_budget(self):
        code, _, err = self.finish(self.start(
            'hpx_slurm_run 4s batch.sh', HPX_SLURM_QUEUE_TIMEOUT="5s",
            PENDING_DELAY="3", JOB_DELAY="6"))
        self.assertEqual(code, 0, err)
        self.assertEqual(self.calls("scancel"), [])

    def test_pending_job_has_a_separate_bound(self):
        code, _, err = self.finish(self.start(
            'hpx_slurm_run 10s batch.sh', HPX_SLURM_QUEUE_TIMEOUT="2s",
            PENDING_DELAY="30", JOB_DELAY="30"))
        self.assertEqual(code, 124, err)
        self.assertIn("queue wait timed out", err)
        self.assertEqual(self.calls("scancel"), [["12345"]])

    def test_start_between_last_poll_and_queue_deadline(self):
        code, _, err = self.finish(self.start(
            'hpx_slurm_run 5s batch.sh', HPX_SLURM_QUEUE_TIMEOUT="4s",
            HPX_SLURM_POLL_INTERVAL="30", PENDING_DELAY="2", JOB_DELAY="6"))
        self.assertEqual(code, 0, err)
        self.assertEqual(self.calls("scancel"), [])

    def test_watchdog_leaves_room_past_both_budgets(self):
        # A job allocated at the queue deadline is only seen running on the
        # next poll, so its execution budget ends after queue + runtime. Log
        # the watchdog's duration through a wrapper around the real timeout.
        wrapper = self.path / "timeout"
        wrapper.write_text(
            "#!" + sys.executable + "\n"
            "import json, os, sys\n"
            "with open(os.environ['CALL_LOG'], 'a') as log:\n"
            "    log.write(json.dumps(['timeout'] + sys.argv[1:]) + '\\n')\n"
            "os.execv(os.environ['REAL_TIMEOUT'],"
            " ['timeout'] + sys.argv[1:])\n")
        wrapper.chmod(0o755)
        code, _, err = self.finish(self.start(
            'hpx_slurm_run 4s batch.sh', HPX_SLURM_QUEUE_TIMEOUT="5s",
            REAL_TIMEOUT=shutil.which("timeout")))
        self.assertEqual(code, 0, err)
        watchdog = [args for args in self.calls("timeout") if "sbatch" in args]
        self.assertEqual(len(watchdog), 1, self.calls("timeout"))
        bound = next(arg for arg in watchdog[0] if arg.endswith("s") and
                     arg[:-1].isdigit())
        # queue (5 s) + runtime (4 s), plus more than the 30 s a queue poll
        # may take before it reports the job as running.
        self.assertGreater(int(bound[:-1]), 5 + 4 + 30)

    def test_completion_during_query_preserves_batch_failure(self):
        code, _, err = self.finish(self.start(
            JOB_DELAY="2", JOB_EXIT="42", QUEUE_DELAY="3", QUEUE_EXIT="6"))
        self.assertEqual(code, 42, err)

    def test_submission_without_id_is_bounded(self):
        code, _, err = self.finish(self.start(
            'hpx_slurm_run 1s batch.sh', HPX_SLURM_QUEUE_TIMEOUT="1s",
            SUBMIT_DELAY="30"))
        self.assertEqual(code, 124, err)
        self.assertEqual(self.calls("scancel"), [])

    def test_hung_owned_job_query_is_bounded(self):
        code, _, err = self.finish(self.start(
            HPX_SLURM_QUEUE_TIMEOUT="2s", JOB_DELAY="30", QUEUE_DELAY="30"))
        self.assertEqual(code, 124, err)
        self.assertEqual(self.calls("scancel"), [["12345"]])

    def test_owned_job_query_failure_cancels_only_owned_job(self):
        code, _, err = self.finish(self.start(JOB_DELAY="30", QUEUE_EXIT="6"))
        self.assertEqual(code, 6, err)
        self.assertEqual(self.calls("scancel"), [["12345"]])

    def test_completed_job_wait_preserves_exit_code(self):
        code, _, err = self.finish(self.start(
            JOB_DELAY="2", JOB_STATE="", JOB_EXIT="42"))
        self.assertEqual(code, 42, err)

    def test_cluster_query_uses_submission_cluster(self):
        code, _, err = self.finish(self.start(
            JOB_ID="12345;rostam", JOB_DELAY="2"))
        self.assertEqual(code, 0, err)
        self.assertEqual(self.calls("squeue"), [[
            "--clusters=rostam", "--jobs=12345", "--noheader", "--format=%T"]])

    def test_invalid_queue_bound_or_poll_interval_rejects_submission(self):
        for env in ({"HPX_SLURM_QUEUE_TIMEOUT": "0s"},
                    {"HPX_SLURM_QUEUE_TIMEOUT": "invalid"},
                    {"HPX_SLURM_POLL_INTERVAL": "0"}):
            with self.subTest(env=env):
                self.env.pop("HPX_SLURM_QUEUE_TIMEOUT", None)
                self.env["HPX_SLURM_POLL_INTERVAL"] = "1"
                code, _, _ = self.finish(self.start(**env))
                self.assertEqual(code, 2)
        self.assertEqual(self.calls("sbatch"), [])

    def test_timeout_kills_unresponsive_sbatch(self):
        code, _, _ = self.finish(self.start(
            'hpx_slurm_run 1s batch.sh', JOB_DELAY="30", IGNORE_TERM="1"))
        # Older GNU timeout (including 8.32 on Rostam) returns 124 after
        # --foreground --kill-after kills the child. Both indicate timeout;
        # finish also requires the child pipes to close within the bound.
        self.assertIn(code, (124, 137))
        self.assertEqual(self.calls("scancel"), [["12345"]])

    def test_hung_cleanup_rpc_is_bounded(self):
        code, _, err = self.finish(self.start(
            JOB_EXIT="42", CANCEL_DELAY="60"), timeout=40)
        self.assertEqual(code, 42)
        self.assertIn("Failed to cancel Slurm job 12345", err)

    def test_hung_previous_cancellation_rpc_is_bounded(self):
        code, _, _ = self.finish(self.start(
            'hpx_slurm_cancel_previous lane 1', CANCEL_DELAY="30"))
        self.assertEqual(code, 124)
        self.assertEqual(self.calls("squeue"), [])

    def test_hung_queue_rpc_is_bounded(self):
        code, _, _ = self.finish(self.start(
            'hpx_slurm_cancel_previous lane 2', QUEUE_DELAY="30"))
        self.assertEqual(code, 124)

    def test_cluster_job_id(self):
        code, _, _ = self.finish(self.start(JOB_ID="12345;rostam", JOB_EXIT="7"))
        self.assertEqual(code, 7)
        self.assertEqual(self.calls("scancel"), [["--clusters=rostam", "12345"]])

    def test_failed_cancellation_keeps_failure(self):
        code, _, err = self.finish(self.start(JOB_EXIT="42", CANCEL_EXIT="8"))
        self.assertEqual(code, 42)
        self.assertIn("Failed to cancel Slurm job 12345", err)

    def test_abort_signals_cancel_owned_id(self):
        for sig in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP):
            with self.subTest(sig=sig):
                self.ready.unlink(missing_ok=True)
                process = self.start(JOB_DELAY="30")
                self.wait_ready()
                process.send_signal(sig)
                code, _, _ = self.finish(process)
                self.assertEqual(code, 128 + sig)
        self.assertEqual(self.calls("scancel"), [["12345"]] * 3)

    def test_process_group_abort(self):
        process = self.start(JOB_DELAY="30")
        self.wait_ready()
        os.killpg(process.pid, signal.SIGTERM)
        code, _, _ = self.finish(process)
        self.assertEqual(code, 143)
        self.assertEqual(self.calls("scancel"), [["12345"]])

    def test_abort_during_submission_captures_late_id(self):
        process = self.start(SUBMIT_DELAY="0.3", JOB_DELAY="30")
        deadline = time.monotonic() + 3
        while not self.log.exists() and time.monotonic() < deadline:
            time.sleep(0.01)
        self.assertTrue(self.log.exists())
        process.send_signal(signal.SIGTERM)
        code, _, _ = self.finish(process)
        self.assertEqual(code, 143)
        self.assertEqual(self.calls("scancel"), [["12345"]])

    def test_previous_cancellation_success(self):
        code, _, _ = self.finish(self.start('hpx_slurm_cancel_previous lane 3'))
        self.assertEqual(code, 0)
        self.assertEqual(self.calls("scancel"),
                         [["--user=" + str(os.getuid()), "--name=lane"]])
        self.assertEqual(self.calls("squeue"),
                         [["--user=" + str(os.getuid()), "--name=lane",
                           "--noheader"]])
        self.assertEqual(self.calls("sbatch"), [])

    def test_previous_cancellation_is_bounded(self):
        code, _, _ = self.finish(self.start(
            'hpx_slurm_cancel_previous lane 1', QUEUE_JOBS="1"))
        self.assertEqual(code, 124)
        self.assertEqual(self.calls("sbatch"), [])

    def test_queue_error_is_failure(self):
        code, _, _ = self.finish(self.start(
            'hpx_slurm_cancel_previous lane 3', QUEUE_EXIT="6"))
        self.assertEqual(code, 6)

    def test_previous_cancel_error_is_failure(self):
        code, _, _ = self.finish(self.start(
            'hpx_slurm_cancel_previous lane 3', CANCEL_EXIT="8"))
        self.assertEqual(code, 8)


if __name__ == "__main__":
    unittest.main()
