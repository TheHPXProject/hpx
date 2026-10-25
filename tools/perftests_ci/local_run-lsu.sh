#!/bin/bash
# Copyright (c)      2021 ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# To execute from the build directory

src_dir=${1:-~/projects/hpx}
build_dir=$PWD

# Clean old artifacts if any
rm -rf ${build_dir}/reports/reference-comparison

# Setup the environment (libs + python)
envfile=${src_dir}/.jenkins/lsu-perftests/env-perftests.sh
source ${envfile}
source /home/pansysk75/virtual_envs/perftests_env/bin/activate

# Variables
mkdir -p ${build_dir}/tools
cp -r ${src_dir}/tools/perftests_ci ${build_dir}/tools
perftests_dir=${build_dir}/tools/perftests_ci
mkdir -p ${build_dir}/reports
logfile=log_perftest_plot.tmp

configure_build_errors=0
configuration_name="local"
test_errors=0
plot_errors=0

source ${src_dir}/.jenkins/lsu-perftests/launch_perftests.sh
