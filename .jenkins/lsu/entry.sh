#!/bin/bash -l

# Copyright (c) 2020 ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Make undefined variables errors, print each command
set -eux

# Clean up old artifacts
rm -f ./jenkins-hpx* ./*-Testing

export configuration_name_with_build_type="${configuration_name}-${build_type,,}"

source .jenkins/lsu/slurm-configuration-${configuration_name}.sh

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
    scancel --verbose --verbose --verbose --verbose --jobname="${job_name}"

    # Wait for the job to be cancelled before launching a new job with the
    # same name
    while squeue --name="${job_name}" --noheader | grep -q .; do
        sleep 1                  # adjust the interval as needed
    done

    export install_hpx=0
fi

# delay things for a random amount of time
sleep $[(RANDOM % 20) + 1].$[(RANDOM % 20)]s

# Start the actual build
set +e
sbatch \
    --verbose --verbose --verbose --verbose \
    --exclusive \
    --job-name="${job_name}" \
    --nodes="${configuration_slurm_num_nodes}" \
    --partition="${configuration_slurm_partition}" \
    --exclude="bahram" \
    --time="06:00:00" \
    --output="jenkins-hpx-${configuration_name_with_build_type}.out" \
    --error="jenkins-hpx-${configuration_name_with_build_type}.err" \
    --wait .jenkins/lsu/batch.sh

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
if [[ -f "${status_file}" && "$(cat ${status_file})" -eq "0" ]]; then
    github_commit_status="success"
else
    github_commit_status="failure"
fi

# Get the CDash dashboard build id
cdash_build_id="$(cat jenkins-hpx-${configuration_name_with_build_type}-cdash-build-id.txt)"

if [[ -z "${ghprbPullId:-}" ]]; then
    .jenkins/common/set_github_status.sh \
        "${GITHUB_TOKEN}" \
        "TheHPXProject/hpx" \
        "${GIT_COMMIT}" \
        "${github_commit_status}" \
        "${configuration_name_with_build_type}" \
        "jenkins/lsu" \
        "${cdash_build_id}"
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
        "jenkins/lsu" \
        "${cdash_build_id}"
fi

set -e
exit $(cat ${status_file})
