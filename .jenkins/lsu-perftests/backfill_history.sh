#!/bin/bash -l

# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Adds the runs of the master commits that are missing from the performance
# history, called by entry.sh after a master build has added its own run. A
# commit is missing if it is a first parent of master, has no run in runs.tsv,
# isn't listed in backfill/skip_commits.txt and was merged more than an hour
# ago: its build was never started, or its run failed or wasn't uploaded. The
# runs are made by backfill/backfill.sh (see its README.md) in an allocation
# of the history node, which this script asks for and waits for, and are
# pushed with the token of the build.
#
# If anything is missing up to the built commit, the allocation is asked for.
# The request may wait in the queue for hours, so what is missing is found out
# again once it is granted, then up to the tip of master. If nothing is
# missing by then, the node is released at once.
#
# A commit gets one run: right before it starts a commit, backfill.sh looks
# at the history on GitHub again, and upload_history.sh doesn't add a master
# build's run if a backfill added that commit's run while the build waited
# for the node. Nothing is done if another build is already adding the
# missing runs. A failure is reported, it doesn't change the status
# of the build.
#
# Usage: backfill_history.sh
#
#   WORKSPACE, GIT_COMMIT, BUILD_NUMBER, GITHUB_TOKEN  from Jenkins
#   configuration_slurm_partition, configuration_slurm_nodelist  from entry.sh
#   PERFTEST_BACKFILL_DIR  the work directory of backfill.sh (default next to
#                          the workspace, kept between builds)
#   PERFTEST_BACKFILL_MAX  at most this many commits per build (default 6)
#   PERFTEST_HISTORY_REPO  the data repository (default hpx-performance)
#   PERFTEST_HPX_REPO      where master is fetched from (default TheHPXProject/hpx)
#   PERFTEST_BACKFILL_DRY_RUN  if set, only list the missing commits

# Never trace the token the credential helper passes to git.
set -u

source "$(dirname "$0")/report_common.sh"

workspace="${WORKSPACE:?the Jenkins workspace}"
commit="${GIT_COMMIT:?the built commit}"
partition="${configuration_slurm_partition:?the Slurm partition}"
node="${configuration_slurm_nodelist:?the history node}"
history_repo="${PERFTEST_HISTORY_REPO:-https://github.com/TheHPXProject/hpx-performance.git}"
max_commits="${PERFTEST_BACKFILL_MAX:-6}"
hpx_repo="${PERFTEST_HPX_REPO:-https://github.com/TheHPXProject/hpx.git}"
job_name="jenkins-hpx-master-perftests"
backfill_job_name="${job_name}-backfill"

# The work directory and the tree the commits are checked out and built in sit
# next to the workspace. The tree's path has as many characters as the
# workspace's, as backfill.sh demands: the length of the source and build paths
# moves the code, so runs built at paths of other lengths aren't comparable.
workspace_name="${workspace##*/}"
export PERFTEST_BACKFILL_DIR="${PERFTEST_BACKFILL_DIR:-${workspace%/*}/${workspace_name}-backfill}"
export PERFTEST_BACKFILL_SRC="${workspace%/*}/${workspace_name:0:$((${#workspace_name} - 3))}-bf"
export PERFTEST_HISTORY_REPO="${history_repo}"
export PERFTEST_BACKFILL_BY="Jenkins build ${BUILD_NUMBER:-unknown} of ${commit:0:10}"
base="${PERFTEST_BACKFILL_DIR}"
src="${PERFTEST_BACKFILL_SRC}"
data="${base}/upload/data"
mkdir -p "${base}/upload" || exit 1

# The token is read from the environment when git asks for it, as in
# upload_history.sh
git_token() {
    git -c credential.helper= \
        -c credential.helper='!f() { echo username=x-access-token; echo "password=${GITHUB_TOKEN}"; }; f' \
        "$@"
}

# One build adds the missing runs at a time
exec 9> "${base}/backfill.lock"
if ! flock -n 9; then
    perftest_log "Another build is adding the missing runs to the history"
    exit 0
fi

# Brings the clone of the history up to date, the runs other builds added
# included. Runs a rejected push left committed are added again from their
# reports (kept in runs/ of the work directory).
refresh_history() {
    if [[ -d "${data}/.git" ]]; then
        git_token -C "${data}" fetch --quiet --depth 1 --filter=blob:none \
            origin main &&
            git -C "${data}" reset --quiet --hard origin/main &&
            git -C "${data}" clean --quiet -fdx ||
            rm -rf "${data}"
    fi
    if [[ ! -d "${data}/.git" ]]; then
        git_token -C "${base}/upload" clone --quiet --depth 1 \
            --filter=blob:none --sparse "${history_repo}" data || {
            echo "Warning: could not clone ${history_repo}" >&2
            return 1
        }
    fi
}

# Sets missing to the master commits up to $2 in the repository $1 that have
# no run in runs.tsv, oldest first (as missing_commits.sh lists them), without
# those left out on purpose and those merged in the last hour (their builds
# may not be in Slurm yet), at most max_commits of them; count to their number
skip_file="${workspace}/.jenkins/lsu-perftests/backfill/skip_commits.txt"
list_missing() {
    local repo=$1 tip=$2 runs="${data}/runs.tsv" since recent total
    since=$(awk -F'\t' 'NR == FNR { if (FNR > 1) run[$2] = 1; next }
        $1 in run { print; exit }' "${runs}" \
        <(git -C "${repo}" log --first-parent --reverse --format=%H "${tip}"))
    missing=""
    count=0
    [[ -n "${since}" ]] || return 0
    recent=$(date -d '1 hour ago' +%s)
    missing=$(git -C "${repo}" log --first-parent --reverse --format='%H %ct' \
        "${since}..${tip}" |
        awk -v recent="${recent}" '$2 < recent { print $1 }' |
        grep -vxF -f <(cut -f2 "${runs}") |
        grep -vxF -f <(grep -o '^[0-9a-f]\{40\}' "${skip_file}") || true)
    total=$(grep -c . <<< "${missing}")
    missing=$(head -n "${max_commits}" <<< "${missing}")
    count=$(grep -c . <<< "${missing}")
    if (( total > count )); then
        perftest_log "${total} master commits are missing from the" \
            "performance history, adding the oldest ${count}"
    elif (( count > 0 )); then
        perftest_log "${count} master commits are missing from the" \
            "performance history"
    fi
}

# A first look, so that no allocation is asked for if nothing is missing. The
# list is made again once the allocation is granted, as the request may wait
# in the queue for hours.
refresh_history || exit 1
list_missing "${workspace}" "${commit}"
if (( count == 0 )); then
    perftest_log "No master commit is missing from the performance history"
    exit 0
fi
if [[ -n "${PERFTEST_BACKFILL_DRY_RUN:-}" ]]; then
    perftest_log "Dry run, the commits would be:" ${missing}
    exit 0
fi

# The scripts as of the built commit, also for drawing the reports: the tree
# they run from must not be the one the commits are checked out in. The
# upload copies the license from there into a history that lacks it.
scripts="${base}/scripts"
rm -rf "${scripts}"
mkdir -p "${scripts}/.jenkins" "${scripts}/tools" &&
    cp -r "${workspace}/.jenkins/lsu-perftests" "${scripts}/.jenkins/" &&
    cp "${workspace}/LICENSE_1_0.txt" "${scripts}/" &&
    cp -r "${workspace}/tools/perftests_ci" "${scripts}/tools/" || {
    echo "Warning: could not copy the scripts, not adding missing runs" >&2
    exit 1
}

# A clone of the workspace to check the commits out in
if [[ ! -d "${src}/.git" ]]; then
    rm -rf "${src}"
    git clone --quiet --no-checkout "${workspace}" "${src}" || {
        echo "Warning: could not clone the workspace, not adding missing" \
            "runs" >&2
        exit 1
    }
fi
git -C "${src}" fetch --quiet "${workspace}" "${commit}" || {
    echo "Warning: could not fetch ${commit} into ${src}, not adding missing" \
        "runs" >&2
    exit 1
}

# The allocation of the history node, for the most commits a build adds:
# half an hour per commit and half an hour more (backfill.sh stops when less
# than 30 minutes are left). It waits for the node as the build did.
minutes=$((30 * max_commits + 30))
perftest_log "Asking for ${node} for ${minutes} minutes"
alloc_log="${base}/salloc.log"
salloc --no-shell --exclusive --nodes=1 --partition="${partition}" \
    --nodelist="${node}" --time="${minutes}" \
    --job-name="${backfill_job_name}" > "${alloc_log}" 2>&1
alloc_status=$?
job=$(grep -o 'Granted job allocation [0-9]*' "${alloc_log}" | grep -o '[0-9]*$')
if [[ "${alloc_status}" -ne 0 || -z "${job}" ]]; then
    cat "${alloc_log}" >&2
    echo "Warning: could not get ${node}, not adding missing runs" >&2
    exit 1
fi
trap 'scancel "${job}" 2> /dev/null' EXIT
perftest_log "Got ${node} in job ${job}"

# What is missing now, up to the tip of master: builds that finished while
# this one waited may have added runs, or failed to.
refresh_history || exit 1
tip="${commit}"
if git -C "${src}" fetch --quiet "${hpx_repo}" master 2> /dev/null; then
    tip=$(git -C "${src}" rev-parse FETCH_HEAD)
else
    echo "Warning: could not fetch master from ${hpx_repo}, adding the" \
        "missing runs up to ${commit:0:10}" >&2
fi
list_missing "${src}" "${tip}"
if (( count == 0 )); then
    perftest_log "Nothing is missing anymore, releasing ${node}"
    exit 0
fi
echo "${missing}" > "${base}/commits.txt"
perftest_log "Adding the runs of" ${missing}

backfill="${scripts}/.jenkins/lsu-perftests/backfill/backfill.sh"
log_start=$(wc -l < "${base}/backfill.log" 2> /dev/null || echo 0)
status=0
"${backfill}" "${job}" >> "${base}/backfill.log" 2>&1 || status=$?

# Push, never forced. A push rejected because another build pushed meanwhile
# is tried again on top of it: the runs are added again from their reports
# (backfill.sh doesn't run the benchmarks again, but needs the allocation),
# as the README describes.
pushed=0 ahead=0
for attempt in 1 2 3; do
    ahead=$(git -C "${data}" rev-list --count origin/main..HEAD 2> /dev/null ||
        echo 0)
    if [[ "${ahead}" == 0 ]]; then
        break
    fi
    if git_token -C "${data}" push --quiet origin HEAD:main; then
        pushed=${ahead}
        ahead=0
        break
    fi
    perftest_log "The push was rejected, adding the runs again on top of" \
        "the history (attempt ${attempt})"
    rm -rf "${data}"
    if ! git_token -C "${base}/upload" clone --quiet --depth 1 \
        --filter=blob:none --sparse "${history_repo}" data; then
        status=1
        break
    fi
    "${backfill}" "${job}" >> "${base}/backfill.log" 2>&1 || status=$?
done
scancel "${job}" 2> /dev/null
trap - EXIT
tail -n "+$((log_start + 1))" "${base}/backfill.log"
perftest_log "Added ${pushed} of the ${count} missing runs to the" \
    "performance history"
if [[ "${ahead}" != 0 || "${status}" != 0 ]]; then
    echo "Warning: not all missing runs were added; the reports of the runs" \
        "made are kept in ${base}/runs for the next build" >&2
    exit 1
fi

# The reports of runs that are in the history now aren't needed anymore (a
# run's directory is runs/<position>-<commit>, see backfill.sh)
for dir in "${base}"/runs/*-*/; do
    short=${dir%/}
    short=${short##*-}
    if [[ -n "${short}" ]] && grep -q "	${short}" "${data}/runs.tsv"; then
        rm -rf "${dir:?}"
    fi
done
exit 0
