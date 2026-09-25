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

# Never cancel by name here: a newer Jenkins build may already have submitted
# the same lane. Pass ownership explicitly because EXIT can run after the
# caller's local variables have gone out of scope.
hpx_slurm_cleanup()
{
    local result="$1"
    local submission_file="$2" submission_pid="$3"
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

# Convert the same duration syntax accepted by hpx_slurm_run to seconds.
hpx_slurm_seconds()
{
    local duration="$1" value
    if [[ ! "${duration}" =~ ^([1-9][0-9]{0,6})([smhd])$ ]]; then
        echo "Slurm timeout must be a positive integer followed by s/m/h/d" >&2
        return 2
    fi
    value="${BASH_REMATCH[1]}"
    case "${BASH_REMATCH[2]}" in
        m) value=$((value * 60)) ;;
        h) value=$((value * 3600)) ;;
        d) value=$((value * 86400)) ;;
    esac
    echo "${value}"
}

# sbatch --wait preserves the batch exit code. Watch only this submission while
# it is pending, then give it a separate execution budget. Polling stops once
# resources are allocated; sbatch --time still enforces the actual runtime.
hpx_slurm_wait()
{
    local submission_file="$1" submission_pid="$2"
    local queue_seconds="$3" runtime_seconds="$4" poll_seconds="$5"
    local deadline=$((SECONDS + queue_seconds)) next_poll=${SECONDS}
    local started=0 job_id="" cluster="" extra="" state remaining result

    while kill -0 "${submission_pid}" 2>/dev/null; do
        # Check once more at the queue deadline: a job may have started since
        # the last poll and must still receive its full execution budget.
        if (( !started && (SECONDS >= next_poll || SECONDS >= deadline) )) &&
            [[ -s "${submission_file}" ]]; then
            IFS=';' read -r job_id cluster extra < "${submission_file}" || true
            if [[ ! "${job_id}" =~ ^[1-9][0-9]*$ || -n "${extra}" ||
                ! "${cluster}" =~ ^[a-zA-Z0-9_-]*$ ]]; then
                echo "sbatch returned no valid job ID" >&2
                return 1
            fi
            remaining=$((deadline - SECONDS))
            (( remaining > 0 )) || remaining=1
            (( remaining <= 30 )) || remaining=30
            if state=$(timeout --foreground --kill-after=5s "${remaining}s" \
                squeue ${cluster:+"--clusters=${cluster}"} --jobs="${job_id}" \
                    --noheader --format=%T); then
                # An empty response means the job has already left the queue;
                # allow sbatch's next completion poll to return its exit code.
                if [[ "${state}" != "PENDING" ]]; then
                    started=1
                    deadline=$((SECONDS + runtime_seconds))
                    echo "Slurm job ${job_id}: execution wait started" >&2
                fi
            else
                result=$?
                # The job can finish while the controller query is in flight.
                kill -0 "${submission_pid}" 2>/dev/null || break
                echo "Failed to query Slurm job ${job_id}" >&2
                return "${result}"
            fi
            next_poll=$((SECONDS + poll_seconds))
        fi
        kill -0 "${submission_pid}" 2>/dev/null || break
        if (( SECONDS >= deadline )); then
            if (( started )); then
                echo "Slurm execution wait timed out: ${job_id}" >&2
            else
                echo "Slurm queue wait timed out: ${job_id:-submission}" >&2
            fi
            return 124
        fi
        sleep 1
    done
    wait "${submission_pid}"
}

# The argument limits execution waiting, independently of queueing. The queue
# budget defaults to one day and can be set with HPX_SLURM_QUEUE_TIMEOUT. The
# execution budget should exceed sbatch --time to allow completion polling and
# teardown. A total watchdog also bounds an unresponsive client. Cleanup adds
# at most five seconds for submission plus two five-second kill graces and one
# thirty-second cancellation RPC. SIGKILL/host loss or a lost submission reply
# still require administrator reconciliation; no name-based fallback is safe.
hpx_slurm_run()
{
    local limit="$1" runtime_seconds queue_seconds
    local poll_seconds="${HPX_SLURM_POLL_INTERVAL:-30}"
    shift
    runtime_seconds=$(hpx_slurm_seconds "${limit}") || return "$?"
    queue_seconds=$(hpx_slurm_seconds \
        "${HPX_SLURM_QUEUE_TIMEOUT:-24h}") || return "$?"
    if [[ ! "${poll_seconds}" =~ ^[1-9][0-9]{0,6}$ ]]; then
        echo "Slurm poll interval must be a positive integer in seconds" >&2
        return 2
    fi
    local submission_file submission_pid cleanup_trap result abort_status=0
    # Defer signals until the child PID and EXIT cleanup are installed.
    trap 'abort_status=129' HUP
    trap 'abort_status=130' INT
    trap 'abort_status=143' TERM
    if ! submission_file=$(mktemp); then
        trap - HUP INT TERM
        return 1
    fi
    # Install EXIT after starting the child so cleanup always has a valid PID.
    # The execution budget starts when a poll first sees the job running, which
    # can be a few seconds after the queue deadline. The watchdog only exists
    # to stop a client that no longer responds, so give it a minute on top of
    # both budgets rather than letting it end a job that is still within them.
    timeout --foreground --kill-after=5s \
        "$((queue_seconds + runtime_seconds + 60))s" \
        sbatch --parsable --wait "$@" > "${submission_file}" &
    submission_pid=$!
    printf -v cleanup_trap 'hpx_slurm_cleanup "$?" %q %q' \
        "${submission_file}" "${submission_pid}"
    # Expand the already quoted PID/path now; retain $? for trap execution.
    # shellcheck disable=SC2064
    trap "${cleanup_trap}" EXIT
    trap 'exit 129' HUP
    trap 'exit 130' INT
    trap 'exit 143' TERM
    (( abort_status == 0 )) || exit "${abort_status}"
    if hpx_slurm_wait "${submission_file}" "${submission_pid}" \
        "${queue_seconds}" "${runtime_seconds}" "${poll_seconds}"; then
        result=0
    else
        result=$?
    fi
    if hpx_slurm_cleanup "${result}" "${submission_file}" "${submission_pid}"; then
        result=0
    else
        result=$?
    fi
    trap - EXIT HUP INT TERM
    return "${result}"
}
