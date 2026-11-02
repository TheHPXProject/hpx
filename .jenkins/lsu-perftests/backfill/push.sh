#!/bin/bash

# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Pushes the runs backfill.sh committed in upload/data to the data repository,
# all of them at once; never forced.
#
# Usage: push.sh
#
#   PERFTEST_BACKFILL_DIR       the work directory of backfill.sh
#   PERFTEST_BACKFILL_PUSH_URL  where to push (default the SSH URL of
#                               hpx-performance)
#
# See README.md.
set -eu
base="${PERFTEST_BACKFILL_DIR:?the work directory}"
url="${PERFTEST_BACKFILL_PUSH_URL:-git@github.com:TheHPXProject/hpx-performance.git}"
data=${base}/upload/data
git -C "${data}" fetch --quiet --depth 1 --filter=blob:none "${url}" main
echo "Runs to push:"
git -C "${data}" log --format='  %h %s' FETCH_HEAD..HEAD
git -C "${data}" push "${url}" HEAD:main
