# Copyright (c) 2026 Ryan Thomas
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

module purge
module load cmake
module load boost/1.88.0-release
module load hwloc

export CXX_STD="20"

configure_extra_options+=" -DHPX_WITH_CXX_STANDARD=${CXX_STD}"
configure_extra_options+=" -DHPX_WITH_MALLOC=system"
configure_extra_options+=" -DHPX_WITH_FETCH_ASIO=ON"
configure_extra_options+=" -DHPX_WITH_COMPILER_WARNINGS=ON"
configure_extra_options+=" -DHPX_WITH_COMPILER_WARNINGS_AS_ERRORS=OFF"
configure_extra_options+=" -DHPX_WITH_CUDA=ON"
configure_extra_options+=" -DHPX_WITH_NETWORKING=OFF"
configure_extra_options+=" -DHPX_WITH_DISTRIBUTED_RUNTIME=OFF"
configure_extra_options+=" -DHPX_WITH_ASYNC_MPI=OFF"
configure_extra_options+=" -DCMAKE_CUDA_ARCHITECTURES='121'"
configure_extra_options+=" -DHPX_WITH_MAX_CPU_COUNT=20"
configure_extra_options+=" -DCMAKE_CXX_COMPILER=/usr/bin/g++-13"
