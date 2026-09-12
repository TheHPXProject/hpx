#!/bin/bash -l

# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Make undefined variables errors, print each command
set -eux

source .jenkins/common/slurm.sh

source .jenkins/lsu-perftests/slurm-constraint-${configuration_name}.sh

if [[ -z "${ghprbPullId:-}" ]]; then
    # Set name of branch if not building a pull request
    export git_local_branch=$(echo ${GIT_BRANCH} | cut -f2 -d'/')
    job_name="jenkins-hpx-${git_local_branch}-${configuration_name}"
else
    job_name="jenkins-hpx-${ghprbPullId}-${configuration_name}"

    # Cancel currently running builds on the same branch, but only for pull
    # requests
    hpx_slurm_cancel_previous "${job_name}"
fi

# delay things for a random amount of time
sleep $[(RANDOM % 10) + 1].$[(RANDOM % 10)]s

# Start the actual build
set +e
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

# Print slurm logs
echo "= stdout =================================================="
cat jenkins-hpx-${configuration_name}.out

echo "= stderr =================================================="
cat jenkins-hpx-${configuration_name}.err

# Get build status
status_file="jenkins-hpx-${configuration_name}-ctest-status.txt"

# Comment on the PR if any failures
if [[ $(cat ${status_file}) != 0 ]]; then
    ./.jenkins/lsu-perftests/comment_github.sh
fi


set -e
if [[ "${slurm_status}" -ne 0 ]]; then
    exit "${slurm_status}"
fi
exit $(cat ${status_file})
