#!/bin/bash

# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Prints the master commits (the first parents, oldest first) after <since>
# that are not in runs.tsv of the clone of backfill.sh, as commits.txt wants
# them. <since> defaults to the earliest commit of master's first parents
# that has a run in runs.tsv: the runs are not added in commit order, and a
# commit whose run failed can be older than the last run, so starting there
# would miss it (nor are the commit dates in first-parent order).
#
# Usage: missing_commits.sh [<since> [<master, default origin/master>]]
#
# With PERFTEST_BACKFILL_DIR as for backfill.sh. See README.md.
set -euo pipefail
base="${PERFTEST_BACKFILL_DIR:?the work directory}"
runs=${base}/upload/data/runs.tsv
hpx="$(cd "$(dirname "$0")/../../.." && pwd)"
master="${2:-origin/master}"
since="${1:-}"
if [[ -z "${since}" && -s "${runs}" ]]; then
    since=$(awk -F'\t' 'NR == FNR { if (FNR > 1) run[$2] = 1; next }
        $1 in run { print; exit }' "${runs}" \
        <(git -C "${hpx}" log --first-parent --reverse --format=%H "${master}"))
fi
if [[ -z "${since}" ]]; then
    echo "no <since> commit given and no run of ${master} in ${runs}" >&2
    exit 1
fi
git -C "${hpx}" log --first-parent --reverse --format=%H "${since}..${master}" |
    while read -r commit; do
        grep -q "	${commit}	" "${runs}" || echo "${commit}"
    done
