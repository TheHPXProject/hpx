#!/bin/bash -l

# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set -eux

# Computes the status of the job and store the artifacts
status_computation_and_artifacts_storage() {
    ctest_exit_code=$?
    ctest_status=$(( ctest_exit_code + configure_build_errors + test_errors + plot_errors ))

    # Copy the testing directory for saving as an artifact
    cp -r ${build_dir}/Testing ${src_dir}/${configuration_name}-Testing
    cp -r ${build_dir}/reports ${src_dir}/${configuration_name}-reports

    echo "${ctest_status}" > "jenkins-hpx-${configuration_name}-ctest-status.txt"
    exit $ctest_status
}

trap "status_computation_and_artifacts_storage" EXIT

src_dir="$(pwd)"
build_dir="${src_dir}/build/${configuration_name}"

mkdir -p "${build_dir}/tools"
cp -r "${src_dir}/tools/perftests_ci" "${build_dir}/tools"

# Variables
perftests_dir=${build_dir}/tools/perftests_ci
envfile=${src_dir}/.jenkins/lsu-perftests/env-${configuration_name}.sh
mkdir -p ${build_dir}/reports
logfile=${build_dir}/reports/jenkins-hpx-${configuration_name}.log

# Load python packages
source /home/pansysk75/virtual_envs/perftests_env/bin/activate

# Things went alright by default
configure_build_errors=0
test_errors=0
plot_errors=0

# Synchronize after the asynchronous copy from the source dir
wait

# Build and Run the perftests
source "${src_dir}/.jenkins/lsu-perftests/launch_perftests.sh"

# Dummy ctest to upload the html report of the perftest
set +e
ctest \
    --verbose \
    -S "${src_dir}/.jenkins/lsu-perftests/ctest.cmake" \
    -DCTEST_BUILD_CONFIGURATION_NAME="${configuration_name}" \
    -DCTEST_SOURCE_DIRECTORY="${src_dir}" \
    -DCTEST_BINARY_DIRECTORY="${build_dir}"
set -e
