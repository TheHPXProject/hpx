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
    def run_entry(self, *, slurm=0, status=None, report=False, changed=False,
                  stale=False, pull_request=True, cdash_build_id=None,
                  upload=0, comment_body=None, branch="origin/master",
                  record=True, history_upload=0, idle_nodes=None,
                  partition_nodes="first second third", queue_timeout=None,
                  nodes="first second", new_references=False,
                  history_node=None):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            lane = root / ".jenkins/lsu-perftests"
            common = root / ".jenkins/common"
            lane.mkdir(parents=True)
            common.mkdir()
            shutil.copyfile(SCRIPT, lane / "entry.sh")
            shutil.copyfile(SCRIPT.parent / "report_common.sh",
                            lane / "report_common.sh")
            (lane / "slurm-constraint-perftests.sh").write_text(
                "configuration_slurm_partition=test\n"
                "configuration_slurm_nodes='" + nodes + "'\n" +
                ("" if history_node is None else
                 "configuration_slurm_history_node=" + history_node + "\n"))
            (common / "slurm.sh").write_text('''
hpx_slurm_cancel_previous() { return 0; }
hpx_slurm_seconds() {
    case "$1" in
        *s) echo "${1%s}" ;;
        *m) echo $(( ${1%m} * 60 )) ;;
        *h) echo $(( ${1%h} * 3600 )) ;;
        *d) echo $(( ${1%d} * 86400 )) ;;
    esac
}
hpx_slurm_run() {
    echo "${HPX_PERFTESTS_REPORT_ALL:-}" > report-all.txt
    echo "${HPX_PERFTESTS_HISTORY:-}" > history.txt
    echo "${HPX_SLURM_QUEUE_TIMEOUT:-}" > queue-timeout.txt
    for arg in "$@"; do
        case "${arg}" in --nodelist=*) echo "${arg#*=}" > node.txt ;; esac
    done
    if [[ "${HPX_PERFTESTS_HISTORY:-}" == 1 && "${CREATE_RECORD}" == 1 ]]; then
        mkdir -p perftests-reports/history
        echo 2026/first/run > perftests-reports/history/path.txt
    fi
    if [[ -n "${BUILD_STATUS}" ]]; then
        echo "${BUILD_STATUS}" > jenkins-hpx-perftests-ctest-status.txt
    fi
    if [[ "${CREATE_NEW_REFERENCES}" == 1 ]]; then
        mkdir -p perftests-reports/reference-comparison/new-references/first
        echo '{}' > \\
            perftests-reports/reference-comparison/new-references/first/a.json
    fi
    if [[ "${CREATE_REPORT}" == 1 ]]; then
        mkdir -p perftests-reports/reference-comparison
        echo current > perftests-reports/reference-comparison/index.html
        if [[ "${CREATE_CHANGED}" == 1 ]]; then
            echo "test executor +" > \\
                perftests-reports/reference-comparison/changed.txt
        fi
    fi
    return "${SLURM_STATUS}"
}
''')
            comment = lane / "comment_github.sh"
            comment.write_text(
                "#!/bin/sh\necho \"${PERFTEST_IMAGES_URL}\" > comment-called\n")
            comment.chmod(0o755)
            upload_stub = lane / "upload_images.sh"
            upload_stub.write_text(
                "#!/bin/sh\nprintf '%s\\n' \"$@\" >> upload-args.txt\n"
                "[ \"${UPLOAD_STATUS}\" = 0 ] || exit 1\n"
                "echo https://images.example/abc/pr/123/7\n")
            upload_stub.chmod(0o755)
            history_stub = lane / "upload_history.sh"
            history_stub.write_text(
                "#!/bin/sh\nprintf '%s\\n' \"$@\" > history-args.txt\n"
                "[ \"${HISTORY_UPLOAD_STATUS}\" = 0 ]\n")
            history_stub.chmod(0o755)
            github_status = common / "set_github_status.sh"
            github_status.write_text(
                "#!/bin/sh\nshift\nprintf '%s\\n' \"$@\" > status-args.txt\n")
            github_status.chmod(0o755)
            if cdash_build_id is not None:
                (root / "jenkins-hpx-perftests-cdash-build-id.txt").write_text(
                    cdash_build_id)
            sleep = root / "sleep"
            sleep.write_text("#!/bin/sh\nexit 0\n")
            sleep.chmod(0o755)
            # Without the stub, sinfo can't be run, as without Slurm. The
            # partition has the nodes in partition_nodes, the idle ones are
            # idle_nodes (padded, as sinfo may print them). Like sinfo, it
            # rejects options it doesn't know.
            if idle_nodes is not None:
                sinfo = root / "sinfo"
                sinfo.write_text(
                    "#!/bin/sh\nfor option in \"$@\"; do case \"$option\" in\n"
                    "  --noheader|--Node|--partition=*|--states=*|--format=*) ;;\n"
                    "  *) echo \"sinfo: unrecognized option '$option'\" >&2;"
                    " exit 1 ;;\n"
                    "esac; done\n"
                    "case \"$*\" in\n"
                    "  *--states=idle*) nodes='" + idle_nodes + "' ;;\n"
                    "  *) nodes='" + partition_nodes + "' ;;\n"
                    "esac\n"
                    "for node in $nodes; do echo \"$node  \"; done\n")
                sinfo.chmod(0o755)
            if stale:
                (root / "jenkins-hpx-perftests-ctest-status.txt").write_text("1")
                old_report = root / "perftests-reports/reference-comparison"
                old_report.mkdir(parents=True)
                (old_report / "index.html").write_text("previous run")
                (old_report / "new-references/first").mkdir(parents=True)
            env = dict(os.environ, PATH=str(root) + ":" + os.environ["PATH"],
                       configuration_name="perftests", GIT_BRANCH=branch,
                       CREATE_RECORD=str(int(record)),
                       HISTORY_UPLOAD_STATUS=str(history_upload),
                       BUILD_STATUS="" if status is None else str(status),
                       SLURM_STATUS=str(slurm), CREATE_REPORT=str(int(report)),
                       CREATE_CHANGED=str(int(changed)),
                       CREATE_NEW_REFERENCES=str(int(new_references)),
                       UPLOAD_STATUS=str(upload), BUILD_NUMBER="7",
                       ghprbPullId="123" if pull_request else "",
                       GITHUB_TOKEN="dummy-token", GIT_COMMIT="branch-sha",
                       ghprbActualCommit="pr-sha",
                       ghprbPullLink="https://github.com/TheHPXProject/hpx/pull/123")
            for name in ("HPX_PERFTESTS_REPORT_ALL", "HPX_PERFTESTS_HISTORY",
                         "HPX_SLURM_QUEUE_TIMEOUT"):
                env.pop(name, None)
            if comment_body is not None:
                env["ghprbCommentBody"] = comment_body
            if queue_timeout is not None:
                env["HPX_SLURM_QUEUE_TIMEOUT"] = queue_timeout
            result = subprocess.run(
                [os.environ.get("HPX_TEST_BASH", "bash"), str(lane / "entry.sh")],
                cwd=root, env=env, capture_output=True, text=True, timeout=10)
            status_args = root / "status-args.txt"
            self.status_args = (status_args.read_text().splitlines()
                                if status_args.exists() else None)
            upload_args = root / "upload-args.txt"
            self.upload_args = (upload_args.read_text().splitlines()
                                if upload_args.exists() else None)
            def read(name):
                path = root / name
                return path.read_text().strip() if path.exists() else None
            self.report_all = read("report-all.txt")
            self.history = read("history.txt")
            self.queue_timeout = read("queue-timeout.txt")
            self.node = read("node.txt")
            self.history_args = read("history-args.txt")
            comment_called = root / "comment-called"
            self.images_url = (comment_called.read_text().strip()
                               if comment_called.exists() else None)
            return result.returncode, comment_called.exists()

    def test_timeout_without_results_does_not_comment(self):
        self.assertEqual(self.run_entry(slurm=124), (124, False))

    def test_timeout_does_not_publish_stale_results(self):
        self.assertEqual(self.run_entry(slurm=124, stale=True), (124, False))

    def test_success_publishes_its_report(self):
        self.assertEqual(self.run_entry(status=0, report=True, changed=True),
                         (0, True))
        self.assertEqual(self.images_url, "https://images.example/abc/pr/123/7")

    def test_unchanged_report_does_not_comment(self):
        self.assertEqual(self.run_entry(status=0, report=True), (0, False))
        self.assertEqual(self.status_args[2], "success")

    def test_failure_without_changes_does_not_comment(self):
        self.assertEqual(self.run_entry(slurm=1, status=1, report=True),
                         (1, False))
        self.assertEqual(self.status_args[2], "failure")

    def test_failed_comparison_publishes_its_report(self):
        self.assertEqual(self.run_entry(slurm=1, status=1, report=True,
                                        changed=True), (1, True))
        self.assertEqual(self.upload_args, [
            "perftests-reports/reference-comparison", "123", "7"])
        self.assertEqual(self.images_url, "https://images.example/abc/pr/123/7")

    def test_failed_upload_publishes_the_report_without_plots(self):
        self.assertEqual(self.run_entry(slurm=1, status=1, report=True,
                                        changed=True, upload=1), (1, True))
        self.assertEqual(self.images_url, "")

    def test_build_failure_without_report_does_not_comment(self):
        self.assertEqual(self.run_entry(slurm=1, status=1), (1, False))

    def test_branch_failure_does_not_comment_on_a_pull_request(self):
        self.assertEqual(self.run_entry(slurm=1, status=1, report=True,
                                        changed=True, pull_request=False),
                         (1, False))

    def test_full_report_is_requested_by_a_comment(self):
        for comment in ("retest lsu perf full", "Retest LSU Perf Full\n",
                        "retest full"):
            with self.subTest(comment=comment):
                self.run_entry(status=0, comment_body=comment)
                self.assertEqual(self.report_all, "1")

    def test_other_comments_dont_request_a_full_report(self):
        for comment in (None, "", "retest lsu perf", "retest"):
            with self.subTest(comment=comment):
                self.run_entry(status=0, comment_body=comment)
                self.assertEqual(self.report_all, "")

    def test_success_reports_status_with_cdash_build(self):
        self.assertEqual(self.run_entry(status=0, cdash_build_id="42"),
                         (0, False))
        self.assertEqual(self.status_args, [
            "TheHPXProject/hpx", "pr-sha", "success", "perftests", "42",
            "jenkins/lsu-perftests"])

    def test_timeout_reports_failure_without_cdash_build(self):
        self.assertEqual(self.run_entry(slurm=124, pull_request=False),
                         (124, False))
        self.assertEqual(self.status_args, [
            "TheHPXProject/hpx", "branch-sha", "failure", "perftests", "",
            "jenkins/lsu-perftests"])

    def test_master_build_adds_the_run_to_the_history(self):
        self.assertEqual(self.run_entry(status=0, report=True,
                                        pull_request=False), (0, False))
        self.assertEqual(self.history, "1")
        self.assertEqual(self.report_all, "1")
        self.assertEqual(self.history_args, "perftests-reports")
        self.assertEqual(self.status_args[2], "success")

    def test_master_build_waits_for_the_history_node(self):
        # also if another node is idle
        self.run_entry(status=0, pull_request=False, history_node="third",
                       idle_nodes="first second")
        self.assertEqual(self.node, "third")
        # the default of hpx_slurm_run (a day), not the 3 hours of a PR
        self.assertEqual(self.queue_timeout, "")

    def test_master_build_waits_for_the_first_node_without_history_node(self):
        self.run_entry(status=0, pull_request=False)
        self.assertEqual(self.node, "first")

    def test_pull_request_build_is_not_added_to_the_history(self):
        self.run_entry(status=0, report=True)
        self.assertEqual(self.history, "")
        self.assertIsNone(self.history_args)
        # sinfo can't be run here, the job is submitted for the first node
        self.assertEqual(self.node, "first")
        self.assertEqual(self.queue_timeout, "3h")

    def test_pull_request_build_runs_on_an_idle_node(self):
        self.assertEqual(self.run_entry(status=0, idle_nodes="second"),
                         (0, False))
        self.assertEqual(self.node, "second")
        # what is left of the 3 hours for the queue
        self.assertRegex(self.queue_timeout, r"^1[0-9]{4}s$")

    def test_nodes_outside_the_partition_are_not_used(self):
        # "other" is idle, but not in the partition, it would never start
        self.assertEqual(self.run_entry(status=0, nodes="other second",
                                        idle_nodes="other second",
                                        partition_nodes="first second"),
                         (0, False))
        self.assertEqual(self.node, "second")

    def test_pull_request_build_waits_for_an_idle_node(self):
        self.assertEqual(self.run_entry(status=0, idle_nodes="",
                                        queue_timeout="1s"), (124, False))
        # never submitted
        self.assertIsNone(self.node)
        self.assertEqual(self.status_args[2], "failure")

    def test_master_build_doesnt_wait_for_an_idle_node(self):
        self.assertEqual(self.run_entry(status=0, pull_request=False,
                                        idle_nodes="", history_node="third"),
                         (0, False))
        self.assertEqual(self.node, "third")

    def test_other_branch_is_not_added_to_the_history(self):
        self.assertEqual(self.run_entry(status=0, pull_request=False,
                                        branch="origin/release-2.0.X"),
                         (0, False))
        self.assertEqual(self.history, "")
        self.assertEqual(self.report_all, "")
        self.assertIsNone(self.history_args)

    def test_failed_history_upload_is_a_failure(self):
        self.assertEqual(self.run_entry(status=0, pull_request=False,
                                        history_upload=1), (1, False))
        self.assertEqual(self.status_args[2], "failure")

    def test_missing_history_record_is_a_failure(self):
        self.assertEqual(self.run_entry(status=0, pull_request=False,
                                        record=False), (1, False))
        self.assertIsNone(self.history_args)
        self.assertEqual(self.status_args[2], "failure")

    def test_failed_master_build_is_still_added_to_the_history(self):
        self.assertEqual(self.run_entry(slurm=1, status=1, pull_request=False),
                         (1, False))
        self.assertEqual(self.history_args, "perftests-reports")

    def test_new_references_are_uploaded_without_a_comment(self):
        self.assertEqual(self.run_entry(status=0, report=True,
                                        new_references=True), (0, False))
        self.assertEqual(self.upload_args, [
            "perftests-reports/reference-comparison", "123", "7"])
        self.assertEqual(self.status_args[2], "success")

    def test_new_references_are_uploaded_once_with_a_comment(self):
        self.assertEqual(self.run_entry(status=0, report=True, changed=True,
                                        new_references=True), (0, True))
        self.assertEqual(self.upload_args, [
            "perftests-reports/reference-comparison", "123", "7"])

    def test_new_references_of_a_branch_build_are_uploaded(self):
        self.assertEqual(self.run_entry(status=0, report=True,
                                        pull_request=False,
                                        new_references=True), (0, False))
        self.assertEqual(self.upload_args, [
            "perftests-reports/reference-comparison", "master", "7"])

    def test_failed_upload_of_new_references_is_a_failure(self):
        self.assertEqual(self.run_entry(status=0, report=True, upload=1,
                                        new_references=True), (1, False))
        self.assertEqual(self.status_args[2], "failure")

    def test_failed_upload_with_a_comment_and_new_references_is_a_failure(self):
        self.assertEqual(self.run_entry(status=0, report=True, changed=True,
                                        upload=1, new_references=True),
                         (1, True))
        self.assertEqual(self.status_args[2], "failure")

    def test_without_new_references_nothing_is_uploaded(self):
        self.run_entry(status=0, report=True)
        self.assertIsNone(self.upload_args)

    def test_stale_new_references_are_not_uploaded(self):
        self.assertEqual(self.run_entry(slurm=124, stale=True), (124, False))
        self.assertIsNone(self.upload_args)

    def test_missing_status_is_a_failure(self):
        self.assertEqual(self.run_entry(), (1, False))


if __name__ == "__main__":
    unittest.main()
