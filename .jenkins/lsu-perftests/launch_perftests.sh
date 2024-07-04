#!/bin/bash -l

# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set -eux

src_dir="$(pwd)"
build_dir="${src_dir}/build"

# rm -rf "${build_dir}"
# mkdir -p "${build_dir}"

ctest -VV\
    --output-on-failure \
    -S ${src_dir}/.jenkins/lsu-perftests/ctest.cmake \
    -DCTEST_SOURCE_DIRECTORY="${src_dir}" \
    -DCTEST_BINARY_DIRECTORY="${build_dir}"
