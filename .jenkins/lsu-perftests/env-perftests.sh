# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2022 Hartmut Kaiser
# Copyright (c) 2024 Alireza Kheirkhahan

#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
set -eu

module purge
module load cmake
module load llvm/18
module load boost/1.85.0-release
module load hwloc
module load openmpi

export CXX_STD="20"

configure_extra_options+=" -DHPX_WITH_CXX_STANDARD=${CXX_STD}"
configure_extra_options+=" -DHPX_WITH_MALLOC=system"
configure_extra_options+=" -DHPX_WITH_FETCH_ASIO=ON"
