#!/bin/bash -l

# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2022 Hartmut Kaiser
# Copyright (c) 2023 Panos Syskakis
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Make undefined variables errors, print each command
set -eux

source .jenkins/common/slurm.sh

source .jenkins/lsu-test-coverage/slurm-constraint-${configuration_name}.sh

if [[ -z "${CHANGE_ID:-}" ]]; then
    # Set name of branch if not building a pull request
    export git_local_branch=$(echo ${GIT_BRANCH} | cut -f2 -d'/')
    job_name="jenkins-hpx-${git_local_branch}-${configuration_name}"
else
    job_name="jenkins-hpx-${CHANGE_ID}-${configuration_name}"

    # Cancel currently running builds on the same branch, but only for pull
    # requests
    hpx_slurm_cancel_previous "${job_name}"
fi

# delay things for a random amount of time
sleep $[(RANDOM % 10) + 1].$[(RANDOM % 10)]s

# Fetch grcov
wget https://github.com/mozilla/grcov/releases/download/v0.8.2/grcov-linux-x86_64.tar.bz2 -O grcov.tar.bz2 \
&& echo "32e40a984cb7ed3a60760e26071618370f10fdce2186916e7321f1dd01a6d0fd grcov.tar.bz2" | sha256sum --check --status \
&& tar -jxf grcov.tar.bz2 \
&& rm grcov.tar.bz2

if [ ! -e "grcov" ]; then
    echo "Error: Failed to fetch grcov."
    exit 1
fi

# Start the actual build
set +e
hpx_slurm_run "${HPX_SLURM_TIMEOUT:-6h}" \
    --verbose --verbose --verbose --verbose \
    --exclusive \
    --job-name="${job_name}" \
    --nodes="1" \
    --partition="${configuration_slurm_partition}" \
    --time="05:00:00" \
    --output="jenkins-hpx-${configuration_name}.out" \
    --error="jenkins-hpx-${configuration_name}.err" \
    .jenkins/lsu-test-coverage/batch.sh
slurm_status=$?


# Print slurm logs
echo "= stdout =================================================="
cat jenkins-hpx-${configuration_name}.out

echo "= stderr =================================================="
cat jenkins-hpx-${configuration_name}.err

# Get build status
status_file="jenkins-hpx-${configuration_name}-ctest-status.txt"

set -e
if [[ "${slurm_status}" -ne 0 ]]; then
    exit "${slurm_status}"
fi
exit $(cat ${status_file})
