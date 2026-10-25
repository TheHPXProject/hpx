#!/bin/bash -l

# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Make undefined variables errors, print each command
set -eux

status_file="jenkins-hpx-${configuration_name}-ctest-status.txt"
report_file="${configuration_name}-reports/reference-comparison/index.html"
changed_file="${configuration_name}-reports/reference-comparison/changed.txt"
new_references_dir="${configuration_name}-reports/reference-comparison/new-references"
history_dir="${configuration_name}-reports/history"
rm -f "jenkins-hpx-${configuration_name}.out" \
    "jenkins-hpx-${configuration_name}.err" "${status_file}" "${report_file}" \
    "${changed_file}"
rm -rf "${new_references_dir}" "${history_dir}"

source .jenkins/common/slurm.sh
source .jenkins/lsu-perftests/report_common.sh

source .jenkins/lsu-perftests/slurm-constraint-${configuration_name}.sh

perftest_history=0
if [[ -z "${ghprbPullId:-}" ]]; then
    # Set name of branch if not building a pull request
    export git_local_branch=$(echo ${GIT_BRANCH} | cut -f2 -d'/')
    job_name="jenkins-hpx-${git_local_branch}-${configuration_name}"

    # Every master commit that is built adds its results to the performance
    # history: all measurements are reported (and plotted), and batch.sh
    # records the run, which is uploaded to the performance data repository
    # below (see upload_history.sh). Older master builds are not cancelled,
    # each of them is a point of the history.
    if [[ "${git_local_branch}" == "master" ]]; then
        perftest_history=1
        export HPX_PERFTESTS_HISTORY=1
        export HPX_PERFTESTS_REPORT_ALL=1
        perftest_log "Build ${BUILD_NUMBER:-} of master commit" \
            "${GIT_COMMIT:-unknown}, its results are added to the" \
            "performance history"
    else
        perftest_log "Build ${BUILD_NUMBER:-} of branch ${git_local_branch}," \
            "commit ${GIT_COMMIT:-unknown}"
    fi
else
    job_name="jenkins-hpx-${ghprbPullId}-${configuration_name}"
    perftest_log "Build ${BUILD_NUMBER:-} of pull request #${ghprbPullId}," \
        "commit ${ghprbActualCommit:-unknown}"

    # Cancel currently running builds on the same branch, but only for pull
    # requests. Old jobs that are slow to go away must not stop this build,
    # or the lane ends here without running or reporting anything.
    perftest_log "Cancelling the earlier Slurm jobs of the pull request" \
        "(${job_name}), if any"
    if ! hpx_slurm_cancel_previous "${job_name}"; then
        echo "Warning: could not clear previous Slurm jobs for ${job_name}," \
            "starting this build anyway" >&2
    fi
fi

# delay things for a random amount of time
sleep $[(RANDOM % 10) + 1].$[(RANDOM % 10)]s

# The job needs its node exclusively, so it waits in the queue for as long as
# another job runs there. Fail after a few hours instead of keeping the pull
# request pending for up to a day (the default of hpx_slurm_run). A master
# build waits for up to a day, so that its commit isn't missing from the
# history.
if [[ "${perftest_history}" != 1 ]]; then
    export HPX_SLURM_QUEUE_TIMEOUT="${HPX_SLURM_QUEUE_TIMEOUT:-3h}"
fi

# Run on the first of the nodes that is idle, as the job needs a node for
# itself. This comes after cancelling the previous jobs of the pull request, so
# that the node they ran on counts as idle. The comparison uses the references
# of the node the job runs on, so the nodes that have references come first.
references_dir="tools/perftests_ci/perftest/references"
reference_nodes=""
candidate_nodes=""
for node in ${configuration_slurm_nodes}; do
    if [[ -e "${references_dir}/${node}.tar.gz" ]]; then
        reference_nodes="${reference_nodes} ${node}"
        candidate_nodes="${candidate_nodes} ${node}"
    fi
done
for node in ${configuration_slurm_nodes}; do
    if [[ ! -e "${references_dir}/${node}.tar.gz" ]]; then
        candidate_nodes="${candidate_nodes} ${node}"
    fi
done
perftest_log "Nodes with references:${reference_nodes:- none}"

# Prints the nodes of the partition, one per line, optionally only those in
# the given states (--states=...). Fails with 2 if Slurm can't be asked.
partition_nodes() {
    local nodes
    nodes=$(timeout --kill-after=5s 30s sinfo --noheader --Node \
        --partition="${configuration_slurm_partition}" "$@" --format=%N) ||
        return 2
    awk 'NF { print $1 }' <<< "${nodes}" | sort -u
}

# Only the nodes of the partition can be used, a job asking for others
# would never start
all_partition_nodes=$(partition_nodes) || all_partition_nodes=""
if [[ -n "${all_partition_nodes}" ]]; then
    usable_nodes="" skipped_nodes=""
    for node in ${candidate_nodes}; do
        if grep -qx "${node}" <<< "${all_partition_nodes}"; then
            usable_nodes="${usable_nodes} ${node}"
        else
            skipped_nodes="${skipped_nodes} ${node}"
        fi
    done
    if [[ -n "${skipped_nodes}" ]]; then
        perftest_log "Not in partition ${configuration_slurm_partition}," \
            "skipped:${skipped_nodes}"
    fi
    if [[ -n "${usable_nodes}" ]]; then
        candidate_nodes="${usable_nodes}"
    else
        echo "Warning: none of the nodes is in partition" \
            "${configuration_slurm_partition}" >&2
    fi
else
    echo "Warning: could not find the nodes of partition" \
        "${configuration_slurm_partition}" >&2
fi

# Sets idle_node to the first of the candidate nodes that is idle, and
# idle_nodes to all idle nodes of the partition. Fails with 1 if none of the
# candidates is idle, and with 2 if Slurm can't be asked.
idle_candidate_node() {
    local node
    idle_node=""
    idle_nodes=$(partition_nodes --states=idle) || return 2
    for node in ${candidate_nodes}; do
        if grep -qx "${node}" <<< "${idle_nodes}"; then
            idle_node="${node}"
            return 0
        fi
    done
    return 1
}

configuration_slurm_nodelist=""
node_status=0
if [[ "${perftest_history}" == 1 ]]; then
    # The history compares the runs with each other, so they all run on the
    # same node (the first one, if none is configured), waiting in the queue
    # for it if needed
    configuration_slurm_nodelist="${configuration_slurm_history_node:-}"
    if [[ -z "${configuration_slurm_nodelist}" ]]; then
        configuration_slurm_nodelist="${configuration_slurm_nodes%% *}"
    fi
    perftest_log "The runs of the history all run on" \
        "${configuration_slurm_nodelist}, waiting for it if it is busy"
else
    # A job submitted for a busy node waits for that node, even if others
    # become idle in the meantime, so wait for any of them to be idle before
    # submitting, out of the same time budget as the queue. The idle nodes are
    # reported whenever they change, the progress every
    # HPX_SLURM_PROGRESS_INTERVAL seconds, without tracing the polls.
    { set +x; } 2> /dev/null
    queue_seconds=$(hpx_slurm_seconds "${HPX_SLURM_QUEUE_TIMEOUT}")
    poll_seconds="${HPX_SLURM_POLL_INTERVAL:-30}"
    progress_seconds="${HPX_SLURM_PROGRESS_INTERVAL:-600}"
    node_deadline=$((SECONDS + queue_seconds))
    next_progress=$((SECONDS + progress_seconds))
    reported_idle_nodes="unknown"
    perftest_log "Looking for an idle node, candidates in this order" \
        "(with references first):${candidate_nodes}"
    while true; do
        node_status=0
        idle_candidate_node || node_status=$?
        if [[ "${node_status}" -ne 2 &&
            "${idle_nodes}" != "${reported_idle_nodes}" ]]; then
            perftest_log "Idle nodes of partition" \
                "${configuration_slurm_partition}:" ${idle_nodes:-none}
            reported_idle_nodes="${idle_nodes}"
        fi
        if [[ "${node_status}" -ne 1 ]]; then
            break
        fi
        if [[ "${SECONDS}" -ge "${node_deadline}" ]]; then
            perftest_log "None of the candidate nodes became idle within" \
                "${HPX_SLURM_QUEUE_TIMEOUT}, the performance tests don't run"
            break
        fi
        if [[ "${SECONDS}" -ge "${next_progress}" ]]; then
            perftest_log "Still waiting for one of the candidate nodes to" \
                "become idle ($(( (SECONDS - node_deadline + queue_seconds) / 60 ))" \
                "of $((queue_seconds / 60)) min)"
            next_progress=$((SECONDS + progress_seconds))
        fi
        sleep "${poll_seconds}"
    done
    set -x
    configuration_slurm_nodelist="${idle_node}"
    # The rest of the time budget is left for the queue
    if [[ "${node_status}" -eq 0 ]]; then
        queue_left=$((node_deadline - SECONDS))
        export HPX_SLURM_QUEUE_TIMEOUT="$((queue_left > 60 ? queue_left : 60))s"
    fi
    # If Slurm can't be asked, submit for the first node as before
    if [[ "${node_status}" -eq 2 ]]; then
        echo "Warning: could not find the idle nodes, submitting for" \
            "${configuration_slurm_nodes%% *}" >&2
        configuration_slurm_nodelist="${configuration_slurm_nodes%% *}"
        node_status=0
    fi
fi
if [[ "${node_status}" -eq 0 ]]; then
    if [[ " ${reference_nodes} " == *" ${configuration_slurm_nodelist} "* ]]; then
        perftest_log "Running the performance tests on" \
            "${configuration_slurm_nodelist}, comparing with its references"
    else
        perftest_log "Running the performance tests on" \
            "${configuration_slurm_nodelist}, which has no references yet:" \
            "the report shows only the results, and they are kept as its" \
            "new references"
    fi
fi
# The report names the node (see report_common.sh)
export PERFTEST_NODE="${configuration_slurm_nodelist}"

# A build triggered by the comment "retest lsu perf full" reports every
# measurement, and comments on the pull request, regardless of how much they
# changed (see perftests_plot.py). The trigger phrase of the Jenkins job has to
# accept the comment.
trigger_comment="${ghprbCommentBody:-}"
if [[ "${trigger_comment,,}" =~ retest([[:space:]]+lsu)?([[:space:]]+perf)?[[:space:]]+full ]]; then
    echo "Full performance report requested by a pull request comment"
    export HPX_PERFTESTS_REPORT_ALL=1
fi
if [[ -n "${HPX_PERFTESTS_REPORT_ALL:-}" ]]; then
    perftest_log "Reporting all measurements"
else
    perftest_log "Reporting only the measurements that changed by more" \
        "than 5%"
fi

# Start the actual build, unless no node became idle (a timeout, as if the
# job had waited in the queue for that long)
set +e
if [[ "${node_status}" -eq 0 ]]; then
    perftest_log "Submitting the Slurm job ${job_name}, waiting up to" \
        "${HPX_SLURM_QUEUE_TIMEOUT:-24h} for it to start and up to" \
        "${HPX_SLURM_TIMEOUT:-4h} for it to finish"
    hpx_slurm_run "${HPX_SLURM_TIMEOUT:-4h}" \
        --verbose --verbose --verbose --verbose \
        --exclusive \
        --job-name="${job_name}" \
        --nodes="1" \
        --partition="${configuration_slurm_partition}" \
        --nodelist="${configuration_slurm_nodelist}" \
        --time="03:00:00" \
        --output="jenkins-hpx-${configuration_name}.out" \
        --error="jenkins-hpx-${configuration_name}.err" \
        .jenkins/lsu-perftests/batch.sh
    slurm_status=$?
    perftest_log "The Slurm job ended with status ${slurm_status}"
else
    slurm_status=124
    touch "jenkins-hpx-${configuration_name}.out" \
        "jenkins-hpx-${configuration_name}.err"
fi

# Print slurm logs
echo "= stdout =================================================="
cat jenkins-hpx-${configuration_name}.out

echo "= stderr =================================================="
cat jenkins-hpx-${configuration_name}.err

if [[ -s "${status_file}" ]]; then
    perftest_log "Status of the performance tests: $(cat "${status_file}")" \
        "(0 if all benchmarks were built, run and compared)"
else
    perftest_log "The performance tests didn't write a status"
fi

# Comment on a pull request only if a measurement got more than 5% faster or
# slower than its reference (see perftests_plot.py). Failures don't produce a
# comment, they turn the status red. Uploading the plots uploads the new
# references as well, if there are any.
new_references=$(compgen -G "${new_references_dir}/*/" || true)
references_status=0
if [[ -n "${new_references}" ]]; then
    perftest_log "New references of all benchmarks of" \
        $(basename -a ${new_references}) "were made by this run"
fi
images_uploaded=0
if [[ -n "${ghprbPullId:-}" ]]; then
    if [[ ! -s "${report_file}" ]]; then
        perftest_log "No report, not commenting on the pull request"
    elif [[ ! -s "${changed_file}" ]]; then
        perftest_log "No measurement to report, not commenting on the pull" \
            "request"
    else
        perftest_log "Commenting on pull request #${ghprbPullId} about" \
            "$(grep -c . "${changed_file}") measurements"
        # Post the report without plots if they can't be uploaded
        images_uploaded=1
        if ! PERFTEST_IMAGES_URL="$(./.jenkins/lsu-perftests/upload_images.sh \
            "$(dirname "${report_file}")" "${ghprbPullId}" "${BUILD_NUMBER}")"; then
            echo "Warning: could not upload the plots of the report" >&2
            PERFTEST_IMAGES_URL=""
            if [[ -n "${new_references}" ]]; then
                references_status=1
            fi
        fi
        PERFTEST_IMAGES_URL="${PERFTEST_IMAGES_URL}" \
            ./.jenkins/lsu-perftests/comment_github.sh
    fi
fi

# New references are uploaded in any case, also without a comment and for a
# build of a branch. They also stay in the Jenkins artifacts, but a failed
# upload turns the status red.
if [[ -n "${new_references}" && "${images_uploaded}" == 0 ]]; then
    perftest_log "Uploading the new references"
    if ! ./.jenkins/lsu-perftests/upload_images.sh \
        "$(dirname "${report_file}")" "${ghprbPullId:-${git_local_branch}}" \
        "${BUILD_NUMBER}" > /dev/null; then
        echo "Warning: could not upload the new references" >&2
        references_status=1
    fi
fi

# Add the run of a master commit to the performance history, also if some of
# the benchmarks failed (the record says which). A run that couldn't be
# uploaded turns the status red, its record stays in the Jenkins artifacts
# (perftests-reports/history).
history_status=0
if [[ "${perftest_history}" == 1 ]]; then
    if [[ -s "${history_dir}/path.txt" ]]; then
        perftest_log "Adding the run to the performance history"
        if ! ./.jenkins/lsu-perftests/upload_history.sh \
            "${configuration_name}-reports"; then
            echo "Warning: could not upload the run to the performance" \
                "history" >&2
            history_status=1
        fi
    else
        echo "Warning: the run wasn't recorded, it is missing from the" \
            "performance history" >&2
        history_status=1
    fi
fi

# Set GitHub status with CDash url
if [[ "${slurm_status}" -eq 0 && "${history_status}" -eq 0 &&
    "${references_status}" -eq 0 &&
    -s "${status_file}" && "$(cat "${status_file}")" == 0 ]]; then
    github_commit_status="success"
else
    github_commit_status="failure"
fi
cdash_build_id_file="jenkins-hpx-${configuration_name}-cdash-build-id.txt"
cdash_build_id=""
if [[ -s "${cdash_build_id_file}" ]]; then
    cdash_build_id="$(cat "${cdash_build_id_file}")"
fi
perftest_log "Setting the GitHub status to ${github_commit_status}" \
    "(Slurm ${slurm_status}, history ${history_status}," \
    "references ${references_status}," \
    "CDash build ${cdash_build_id:-none})"

# Do not print the token passed to the status helper.
set +x
if [[ -z "${ghprbPullId:-}" ]]; then
    .jenkins/common/set_github_status.sh \
        "${GITHUB_TOKEN}" \
        "TheHPXProject/hpx" \
        "${GIT_COMMIT}" \
        "${github_commit_status}" \
        "${configuration_name}" \
        "${cdash_build_id}" \
        "jenkins/lsu-perftests"
else
    # Extract just the organization and repo names "org/repo" from the full URL
    github_commit_repo="$(echo $ghprbPullLink | sed -n 's/https:\/\/github.com\/\(.*\)\/pull\/[0-9]*/\1/p')"

    .jenkins/common/set_github_status.sh \
        "${GITHUB_TOKEN}" \
        "${github_commit_repo}" \
        "${ghprbActualCommit}" \
        "${github_commit_status}" \
        "${configuration_name}" \
        "${cdash_build_id}" \
        "jenkins/lsu-perftests"
fi
set -x

set -e
if [[ "${slurm_status}" -ne 0 ]]; then
    exit "${slurm_status}"
fi
if [[ "${history_status}" -ne 0 ]]; then
    exit "${history_status}"
fi
if [[ "${references_status}" -ne 0 ]]; then
    exit "${references_status}"
fi
if [[ ! -s "${status_file}" ]]; then
    echo "Missing performance test status: ${status_file}" >&2
    exit 1
fi
exit "$(cat "${status_file}")"
