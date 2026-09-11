#!/bin/bash -l

# Copyright (c) 2020 ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Never trace the token, including when invoked with bash -x.
set +x
set -eu

github_token=${1}
commit_repo=${2}
commit_sha=${3}
commit_status=${4}
configuration_name=${5}
build_id=${6}
context=${7}

# A moved endpoint is an error: use the canonical repository instead of
# forwarding a credential-bearing POST to an unexpected redirect target.
# Retry transient failures at most twice, with bounded connection/transfer time.
http_status=$(curl --disable --silent --show-error --fail \
    --connect-timeout 10 --max-time 30 \
    --retry 2 --retry-delay 1 --retry-max-time 60 \
    --output /dev/null --write-out '%{http_code}' \
    --request POST \
    --url "https://api.github.com/repos/${commit_repo}/statuses/${commit_sha}" \
    --header 'Content-Type: application/json' \
    --header "authorization: Bearer ${github_token}" \
    --data "{
        \"state\": \"${commit_status}\",
        \"target_url\": \"https://cdash.rostam.cct.lsu.edu/build/${build_id}\",
        \"description\": \"Jenkins\",
        \"context\": \"${context}/${configuration_name}\"
    }")

if [[ ! "${http_status}" =~ ^2[0-9][0-9]$ ]]; then
    echo "GitHub status update failed: HTTP ${http_status}" >&2
    exit 1
fi
