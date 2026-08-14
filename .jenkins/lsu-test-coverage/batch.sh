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

# Run tests
ctest --test-dir "${build_dir}" --output-on-failure
ctest_status=$?


# Tests are finished; Collect coverage data
./grcov . -s ${src_dir} -o lcov.info -t lcov --log "grcov-log.txt" --ignore-not-existing --ignore "/*"

# Upload to Codacy
bash <(curl -Ls https://coverage.codacy.com/get.sh) report -r lcov.info --language CPP -t ${CODACY_TOKEN} --commit-uuid ${GIT_COMMIT}

echo "${ctest_status}" > "jenkins-hpx-${configuration_name}-ctest-status.txt"
exit $ctest_status
