#!/bin/bash

# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Waits for a process (a backfill.sh that stops when its allocation runs out,
# or anything else using the node) to exit, then, if commits of commits.txt
# are not yet in runs.tsv, waits for the job to run and starts backfill.sh in
# it. Nothing is pushed.
#
# Usage: after.sh <pid> <job id of the held allocation>
#
# With the environment of backfill.sh. See README.md.
set -u
base="${PERFTEST_BACKFILL_DIR:?the work directory}"
pid="${1:?the process to wait for}"
job="${2:?the job id of the held allocation}"
log() { echo "$(date '+%F %T') $*"; }
while kill -0 "${pid}" 2> /dev/null; do sleep 30; done
left=0
while read -r commit; do
    grep -q "	${commit}	" "${base}/upload/data/runs.tsv" 2> /dev/null || left=$((left + 1))
done < "${base}/commits.txt"
log "process ${pid} done, ${left} commits left"
(( left > 0 )) || exit 0
while true; do
    state=$(squeue -h -j "${job}" -o %T 2> /dev/null)
    [[ "${state}" == RUNNING ]] && break
    if [[ -z "${state}" ]]; then
        log "job ${job} is gone, stopping"
        exit 1
    fi
    sleep 60
done
log "job ${job} running, starting backfill.sh"
exec "$(dirname "$0")/backfill.sh" "${job}" < /dev/null
