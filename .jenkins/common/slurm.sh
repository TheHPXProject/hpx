#!/bin/bash

# Copyright (c) 2026 Anshuman Agrawal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# GNU timeout bounds controller RPCs as well as time spent in the queue.
hpx_slurm_cancel_previous()
{
    local job_name="$1"
    local deadline=$((SECONDS + ${2:-120}))
    local jobs remaining
    local user_id
    user_id=$(id -u) || return "$?"

    remaining=$((deadline - SECONDS))
    (( remaining > 0 )) || return 124
    timeout --foreground --kill-after=5s "${remaining}s" \
        scancel --user="${user_id}" --name="${job_name}" || return "$?"
    while (( SECONDS < deadline )); do
        remaining=$((deadline - SECONDS))
        (( remaining > 0 )) || break
        jobs=$(timeout --foreground --kill-after=5s "${remaining}s" \
            squeue --user="${user_id}" --name="${job_name}" --noheader) || return "$?"
        [[ -n "${jobs}" ]] || return 0
        sleep 1
    done
    echo "Timed out waiting for previous Slurm jobs: ${job_name}" >&2
    return 124
}

# Called while the locals of hpx_slurm_run are still in scope. Never cancel
# by name here: a newer Jenkins build may already have submitted the same lane.
hpx_slurm_cleanup()
{
    local result="$1"
    local job_id="" cluster="" extra=""
    local grace=$((SECONDS + 5))

    # A second abort must not interrupt cancellation of the owned job.
    trap '' HUP INT TERM
    if (( result != 0 )); then
        # Allow an in-flight submission to return its ID before stopping sbatch.
        while [[ ! -s "${submission_file}" ]] && \
            kill -0 "${submission_pid}" 2>/dev/null && \
            (( SECONDS < grace )); do
            sleep 1
        done
        # timeout forwards TERM to sbatch and enforces its five-second kill grace.
        kill -TERM "${submission_pid}" 2>/dev/null || true
        wait "${submission_pid}" 2>/dev/null || true
    fi

    IFS=';' read -r job_id cluster extra < "${submission_file}" || true
    if [[ "${job_id}" =~ ^[1-9][0-9]*$ && -z "${extra}" &&
        "${cluster}" =~ ^[a-zA-Z0-9_-]*$ ]]; then
        echo "Slurm job: ${job_id}${cluster:+;${cluster}}" >&2
        if (( result != 0 )); then
            if ! timeout --foreground --kill-after=5s 30s scancel \
                ${cluster:+"--clusters=${cluster}"} "${job_id}"; then
                echo "Failed to cancel Slurm job ${job_id}; check it manually" >&2
            fi
        fi
    elif (( result == 0 )); then
        echo "sbatch returned no valid job ID" >&2
        result=1
    else
        echo "No Slurm job ID received; submission may need reconciliation" >&2
    fi
    rm -f "${submission_file}"
    return "${result}"
}

# The limit includes submission, queueing and execution, unlike sbatch --time.
# Leave one hour beyond the lane's runtime limit for queueing. Cleanup can add
# at most five seconds for submission plus two five-second kill graces and one
# thirty-second cancellation RPC. SIGKILL/host loss or a lost submission reply
# still require administrator reconciliation; no name-based fallback is safe.
hpx_slurm_run()
{
    local limit="$1"
    shift
    if [[ ! "${limit}" =~ ^[1-9][0-9]*[smhd]$ ]]; then
        echo "Slurm timeout must be a positive integer followed by s/m/h/d" >&2
        return 2
    fi
    local submission_file submission_pid result abort_status=0
    # Defer signals until the child PID and EXIT cleanup are installed.
    trap 'abort_status=129' HUP
    trap 'abort_status=130' INT
    trap 'abort_status=143' TERM
    if ! submission_file=$(mktemp); then
        trap - HUP INT TERM
        return 1
    fi
    # Install EXIT after starting the child so cleanup always has a valid PID.
    timeout --foreground --kill-after=5s "${limit}" sbatch --parsable --wait "$@" \
        > "${submission_file}" &
    submission_pid=$!
    trap 'hpx_slurm_cleanup "$?"' EXIT
    trap 'exit 129' HUP
    trap 'exit 130' INT
    trap 'exit 143' TERM
    (( abort_status == 0 )) || exit "${abort_status}"
    if wait "${submission_pid}"; then
        result=0
    else
        result=$?
    fi
    if hpx_slurm_cleanup "${result}"; then
        result=0
    else
        result=$?
    fi
    trap - EXIT HUP INT TERM
    return "${result}"
}
