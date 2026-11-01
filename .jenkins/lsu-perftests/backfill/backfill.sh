#!/bin/bash -l

# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Adds the runs of the master commits in commits.txt (oldest first) to the
# performance history as the HPX-performance-history job would: the steps of
# the lane's batch.sh with HPX_PERFTESTS_HISTORY=1 and HPX_PERFTESTS_REPORT_ALL=1
# on the node of a held allocation (without the CDash submits), then the steps
# of upload_history.sh into the local clone upload/data, without pushing
# (push.sh pushes). No Jenkins build: BUILD_NUMBER is unset (the directories
# of the runs end in -b0), the build line of each run's README says it ran
# outside Jenkins. Commits already in runs.tsv are skipped, so the script can
# be run again with the same list. A commit whose reports an earlier start
# kept in runs/ (for example a run committed but not pushed before the clone
# was replaced) is added again from them, without running the benchmarks.
#
# The benchmarks and the lane scripts are those of each commit. The reports
# and the history are drawn by perftests_plot.py, perftests_timeline.py and
# upload_history.sh of the HPX tree this script is in, so that all runs added
# are drawn the same way.
#
# The commits are checked out in a tree at a path of the same length as the
# Jenkins workspace (/work/jenkins/workspace/HPX-performance-history): the
# length of the source and build paths moves the code (__FILE__ strings), so
# runs built at paths of other lengths aren't comparable with those of
# Jenkins.
#
# Usage: backfill.sh <job id of the held allocation>
#
#   PERFTEST_BACKFILL_DIR  the work directory: commits.txt, the log of every
#                          run (runs/), the clone of the data repository
#                          (upload/data) and the generated scripts (gen/)
#   PERFTEST_BACKFILL_SRC  the tree the commits are checked out in, a worktree
#                          of the HPX tree this script is in (added if missing)
#   PERFTEST_BACKFILL_CPUS the cores of the lane steps (default 40)
#   PERFTEST_HISTORY_REPO  the data repository (default hpx-performance)
#
# Runs on the login node (the upload needs the network), the lane steps via
# srun in the allocation. See README.md.
set -u
base="${PERFTEST_BACKFILL_DIR:?the work directory}"
src="${PERFTEST_BACKFILL_SRC:?the tree the commits are checked out in}"
job="${1:?the job id of the held allocation}"
cpus="${PERFTEST_BACKFILL_CPUS:-40}"
history_repo="${PERFTEST_HISTORY_REPO:-https://github.com/TheHPXProject/hpx-performance.git}"
jenkins_src=/work/jenkins/workspace/HPX-performance-history
hpx="$(cd "$(dirname "$0")/../../.." && pwd)"
lane=${src}/.jenkins/lsu-perftests
gen=${base}/gen
export JOB_URL=https://rostam.cct.lsu.edu/jenkins/job/HPX-performance-history/
export PERFTEST_HISTORY_WORK_DIR=${base}/upload
log() { echo "$(date '+%F %T') $*"; }

if (( ${#src} != ${#jenkins_src} )); then
    log "${src} has ${#src} characters, ${jenkins_src} has ${#jenkins_src}, stopping"
    exit 1
fi
node=$(squeue -h -j "${job}" -o %N 2> /dev/null)
if [[ -z "${node}" ]]; then
    log "job ${job} isn't running, stopping"
    exit 1
fi
mkdir -p "${gen}" "${base}/runs" "${base}/upload"
if [[ ! -d "${src}" ]]; then
    git -C "${hpx}" worktree add --detach "${src}" > /dev/null ||
        { log "could not add the worktree ${src}, stopping"; exit 1; }
fi
if [[ ! -d "${base}/upload/data" ]]; then
    git -C "${base}/upload" clone --quiet --depth 1 --filter=blob:none --sparse \
        "${history_repo}" data || { log "could not clone ${history_repo}, stopping"; exit 1; }
fi

# upload_history.sh with the clone in upload/, without the clone, the cleanup
# and the push
awk '
    /^work_dir="\$\(mktemp -d\)"$/ { print "work_dir=\"${PERFTEST_HISTORY_WORK_DIR:?}\""; next }
    /^trap .rm -rf "\$\{work_dir\}". EXIT$/ { next }
    /^git_history clone / { skip = 1 }
    skip { if (/exit 1; }$/) skip = 0; next }
    /^# Other builds may push at the same time/ { print "exit 0"; exit }
    { print }' "${hpx}/.jenkins/lsu-perftests/upload_history.sh" |
    sed "s|\$(dirname \"\$0\")|${hpx}/.jenkins/lsu-perftests|g" > "${gen}/upload-local.sh"
chmod +x "${gen}/upload-local.sh"
if grep -q 'mktemp\|push\|git_history clone\|dirname "\$0"' "${gen}/upload-local.sh" ||
    ! grep -q '^exit 0$' "${gen}/upload-local.sh"; then
    log "could not make the upload copy, stopping"
    exit 1
fi

# Whether the reports in $1 have every file the upload reads
# (perftests_record.py writes them one after the other)
recorded() {
    local f
    for f in metadata.json summary.json path.txt intro.md run.tsv; do
        [[ -s "$1/history/${f}" ]] || return 1
    done
}

# Adds the run of the reports in $1 to upload/data, the log in $2
add_reports() {
    local reports=$1 upload_log=$2
    sed -i 's/^Jenkins build unknown, ran on /Ran outside Jenkins with the steps of the lane (without the CDash submits) on /' \
        "${reports}/history/intro.md"
    grep -q '^Ran outside Jenkins' "${reports}/history/intro.md" ||
        log "[${n}/${total}] ${short}: build line of intro.md not reworded"
    log "[${n}/${total}] ${short}: adding $(cat "${reports}/history/path.txt")"
    if "${gen}/upload-local.sh" "${reports}" > "${upload_log}" 2>&1; then
        log "[${n}/${total}] ${short}: added, $(git -C "${base}/upload/data" log -1 --format='%h %s')"
    else
        log "[${n}/${total}] ${short}: adding failed, see ${upload_log}"
        # upload_history.sh adds the run to runs.tsv before it commits: drop
        # what it left, or the next start would skip this commit
        git -C "${base}/upload/data" reset --quiet --hard HEAD
        git -C "${base}/upload/data" clean --quiet -fd
    fi
}

total=$(grep -c . "${base}/commits.txt")
n=0
while read -r commit; do
    n=$((n + 1))
    short=${commit:0:10}
    run_dir=${base}/runs/$(printf %02d "${n}")-${short}
    if grep -q "	${commit}	" "${base}/upload/data/runs.tsv" 2> /dev/null; then
        log "[${n}/${total}] ${short}: already in runs.tsv, skipped"
        continue
    fi
    kept=$(ls -d "${base}"/runs/*-"${short}"/perftests-reports 2> /dev/null | tail -1)
    if [[ -n "${kept}" ]] && recorded "${kept}"; then
        log "[${n}/${total}] ${short}: reports kept in ${kept%/perftests-reports}"
        add_reports "${kept}" "${kept%/perftests-reports}/upload.log"
        continue
    fi

    end=$(squeue -h -j "${job}" -o %e 2> /dev/null)
    if [[ -z "${end}" ]] ||
        (( $(date -d "${end}" +%s) - $(date +%s) < 1800 )); then
        log "[${n}/${total}] ${short}: less than 30 min left in job ${job}, stopping"
        exit 1
    fi

    log "[${n}/${total}] ${short}: checkout"
    if ! git -C "${src}" checkout -q -f --detach "${commit}"; then
        log "[${n}/${total}] ${short}: checkout failed, skipped"
        continue
    fi
    cp "${hpx}/tools/perftests_ci/perftests_plot.py" \
        "${hpx}/tools/perftests_ci/perftests_timeline.py" "${src}/tools/perftests_ci/"

    # ctest.cmake without its ctest_submit(...) calls, and batch.sh running it
    awk '/^ctest_submit\(/ { skip = 1 } !skip { print } skip && /^\)/ { skip = 0 }' \
        "${lane}/ctest.cmake" > "${gen}/ctest-nosubmit.cmake"
    sed "s|\"\${src_dir}/.jenkins/lsu-perftests/ctest.cmake\"|\"${gen}/ctest-nosubmit.cmake\"|" \
        "${lane}/batch.sh" > "${gen}/batch-local.sh"
    chmod +x "${gen}/batch-local.sh"
    if grep -q ctest_submit "${gen}/ctest-nosubmit.cmake" ||
        ! grep -q ctest-nosubmit "${gen}/batch-local.sh"; then
        log "[${n}/${total}] ${short}: could not make the lane copies, stopping"
        exit 1
    fi

    # entry.sh's cleanup, and the reports of the previous run
    (cd "${src}" && rm -rf perftests-reports perftests-Testing \
        jenkins-hpx-perftests.out jenkins-hpx-perftests.err \
        jenkins-hpx-perftests-*.txt)

    log "[${n}/${total}] ${short}: lane steps on job ${job} (${node})"
    srun --jobid="${job}" --overlap -N1 -n1 -c "${cpus}" bash -lc "
        cd '${src}' &&
        export configuration_name=perftests git_local_branch=master \
            GIT_COMMIT='${commit}' HPX_PERFTESTS_HISTORY=1 \
            HPX_PERFTESTS_REPORT_ALL=1 PERFTEST_NODE='${node}' &&
        unset BUILD_NUMBER BUILD_URL ghprbPullId &&
        '${gen}/batch-local.sh' > jenkins-hpx-perftests.out \
            2> jenkins-hpx-perftests.err" < /dev/null
    rc=$?
    log "[${n}/${total}] ${short}: srun ${rc}, status" \
        "$(cat "${src}/jenkins-hpx-perftests-ctest-status.txt" 2> /dev/null || echo none)"

    # an earlier start's output of a run that wasn't recorded
    rm -rf "${run_dir:?}"
    mkdir -p "${run_dir}"
    cp -r "${src}"/jenkins-hpx-perftests.out "${src}"/jenkins-hpx-perftests.err \
        "${src}"/jenkins-hpx-perftests-*.txt "${src}/perftests-reports" \
        "${run_dir}" 2> /dev/null

    if ! recorded "${run_dir}/perftests-reports"; then
        log "[${n}/${total}] ${short}: the run wasn't recorded, not added"
        continue
    fi
    add_reports "${run_dir}/perftests-reports" "${run_dir}/upload.log"
done < "${base}/commits.txt"
log "done"
