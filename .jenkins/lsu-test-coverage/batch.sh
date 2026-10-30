#!/bin/bash -l

# Copyright (c) 2023 Panos Syskakis
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set -eux

src_dir="$(pwd)"
build_dir="${src_dir}/build/"

rm -rf "${build_dir}"

source "${src_dir}/.jenkins/lsu-test-coverage/env-${configuration_name}.sh"

ulimit -l unlimited

set +e

# Configure
cmake \
    -S "${src_dir}"   \
    -B "${build_dir}" \
     -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DHPX_WITH_CXX_STANDARD=20 \
    -DHPX_WITH_MALLOC=system \
    -DHPX_WITH_FETCH_ASIO=ON \
    -DHPX_WITH_PARCELPORT_MPI=ON \
    -DHPX_WITH_PARCELPORT_LCI=ON \
    -DHPX_WITH_FETCH_LCI=ON \
    -DHPX_WITH_LCI_BOOTSTRAP_MPI=ON \
    -DCMAKE_CXX_FLAGS="-O0 --coverage" \
    -DCMAKE_EXE_LINKER_FLAGS=--coverage 


# Build
cmake --build "${build_dir}" --target tests examples

# Run tests. A serial run of the instrumented suite takes two to three hours,
# which together with the build does not fit into the job's time limit. Use the
# same parallelism and per-test timeout as the build matrix.
ctest --test-dir "${build_dir}" --output-on-failure --parallel 4 --timeout 300
ctest_status=$?


# Tests are finished; Collect coverage data
# grcov writes one function record for every template instantiation in every
# translation unit. For HPX those records make up more than 99% of a report
# of over 20 GB. Codacy reads the line records only, so drop the function
# records while streaming the report to disk.
coverage_status=0
./grcov . -s "${src_dir}" -t lcov --log "grcov-log.txt" \
    --ignore-not-existing --ignore "/*" |
    grep -v -E '^(FN|FNDA|FNF|FNH):' > lcov.info
pipeline_status=("${PIPESTATUS[@]}")
grcov_status="${pipeline_status[0]}"
filter_status="${pipeline_status[1]}"
if [[ "${grcov_status}" -ne 0 ]]; then
    echo "Error: grcov failed to generate coverage data."
    coverage_status=1
elif [[ "${filter_status}" -gt 1 ]]; then
    # grep exits 1 when every record was filtered out, which the next check
    # reports. Anything higher is an error, such as a failed write, and can
    # leave a partial report behind.
    echo "Error: filtering the coverage data failed."
    coverage_status=1
elif [[ ! -s lcov.info ]]; then
    echo "Error: grcov produced no coverage data."
    coverage_status=1
fi

# Upload to Codacy. Download the uploader to a file first: running it through
# bash <(curl ...) hides a failed download, because bash happily runs the
# empty script it gets and reports success.
if [[ "${coverage_status}" -eq 0 ]]; then
    if ! curl --fail --location --silent --show-error \
            https://coverage.codacy.com/get.sh -o codacy-uploader.sh; then
        echo "Error: downloading the Codacy uploader failed."
        coverage_status=1
    elif ! bash codacy-uploader.sh report -r lcov.info --language CPP \
            -t "${CODACY_TOKEN}" --commit-uuid "${GIT_COMMIT}"; then
        echo "Error: uploading coverage to Codacy failed."
        coverage_status=1
    fi
    rm -f codacy-uploader.sh
fi

echo "${ctest_status}" > "jenkins-hpx-${configuration_name}-ctest-status.txt"
if [[ "${ctest_status}" -ne 0 ]]; then
    exit "${ctest_status}"
fi
exit "${coverage_status}"
