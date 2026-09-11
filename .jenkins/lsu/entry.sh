#!/bin/bash -l

# Copyright (c) 2020 ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Make undefined variables errors, print each command
set -eux

export configuration_name_with_build_type="${configuration_name}-${build_type,,}"

# Matrix lanes share this directory. Remove only this lane's old artifacts.
rm -f \
    "jenkins-hpx-${configuration_name_with_build_type}.out" \
    "jenkins-hpx-${configuration_name_with_build_type}.err" \
    "jenkins-hpx-${configuration_name_with_build_type}-ctest-status.txt" \
    "jenkins-hpx-${configuration_name_with_build_type}-cdash-build-id.txt" \
    "jenkins-hpx-${configuration_name_with_build_type}-cdash-submission.txt"

source .jenkins/lsu/slurm-configuration-${configuration_name}.sh
source .jenkins/common/slurm.sh

if [[ -z "${ghprbPullId:-}" ]]; then
    # Set name of branch if not building a pull request
    export git_local_branch=$(echo ${GIT_BRANCH} | cut -f2 -d'/')
    job_name="jenkins-hpx-${git_local_branch}-${configuration_name_with_build_type}"

    if [[ "${git_local_branch}" == "master" ]]; then
        export install_hpx=1
    else
        export install_hpx=0
    fi
else
    job_name="jenkins-hpx-${ghprbPullId}-${configuration_name_with_build_type}"

    # Cancel currently running builds on the same branch, but only for pull
    # requests
    hpx_slurm_cancel_previous "${job_name}"

    export install_hpx=0
fi

# delay things for a random amount of time
sleep $[(RANDOM % 20) + 1].$[(RANDOM % 20)]s

# Start the actual build
set +e
hpx_slurm_run "${HPX_SLURM_TIMEOUT:-7h}" \
    --verbose --verbose --verbose --verbose \
    --exclusive \
    --job-name="${job_name}" \
    --nodes="${configuration_slurm_num_nodes}" \
    --partition="${configuration_slurm_partition}" \
    --exclude="bahram" \
    --time="06:00:00" \
    --output="jenkins-hpx-${configuration_name_with_build_type}.out" \
    --error="jenkins-hpx-${configuration_name_with_build_type}.err" \
    .jenkins/lsu/batch.sh
slurm_status=$?

# Print slurm logs
echo "= stdout =================================================="
cat jenkins-hpx-${configuration_name_with_build_type}.out

echo "= stderr =================================================="
cat jenkins-hpx-${configuration_name_with_build_type}.err

# Echo submission status
echo "= submission =============================================="
cat jenkins-hpx-${configuration_name_with_build_type}-cdash-submission.txt

# Get build status
status_file="jenkins-hpx-${configuration_name_with_build_type}-ctest-status.txt"
if [[ "${slurm_status}" -eq 0 && -f "${status_file}" &&
    "$(cat ${status_file})" -eq "0" ]]; then
    github_commit_status="success"
else
    github_commit_status="failure"
fi

# Get the CDash dashboard build id
cdash_build_id="$(cat jenkins-hpx-${configuration_name_with_build_type}-cdash-build-id.txt)"

if [[ -z "${ghprbPullId:-}" ]]; then
    .jenkins/common/set_github_status.sh \
        "${GITHUB_TOKEN}" \
        "STEllAR-GROUP/hpx" \
        "${GIT_COMMIT}" \
        "${github_commit_status}" \
        "${configuration_name_with_build_type}" \
        "${cdash_build_id}" \
        "jenkins/lsu"
else
    # Extract just the organization and repo names "org/repo" from the full URL
    github_commit_repo="$(echo $ghprbPullLink | sed -n 's/https:\/\/github.com\/\(.*\)\/pull\/[0-9]*/\1/p')"

    # Set GitHub status with CDash url
    .jenkins/common/set_github_status.sh \
        "${GITHUB_TOKEN}" \
        "${github_commit_repo}" \
        "${ghprbActualCommit}" \
        "${github_commit_status}" \
        "${configuration_name_with_build_type}" \
        "${cdash_build_id}" \
        "jenkins/lsu"
fi

set -e
if [[ "${slurm_status}" -ne 0 ]]; then
    exit "${slurm_status}"
fi
exit $(cat ${status_file})
