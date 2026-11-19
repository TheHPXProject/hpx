# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Where the ROCm packages HPX uses (and their dependencies) are found: also in
# the directories the environment names (needed on rostam).
macro(hpx_add_hip_prefix_path)
  list(APPEND CMAKE_PREFIX_PATH $ENV{HIP_PATH}/lib/cmake/hip)
  list(APPEND CMAKE_PREFIX_PATH $ENV{DEVICE_LIB_PATH}/cmake/AMDDeviceLibs)
  list(APPEND CMAKE_PREFIX_PATH $ENV{DEVICE_LIB_PATH}/cmake/amd_comgr)
  list(APPEND CMAKE_PREFIX_PATH $ENV{DEVICE_LIB_PATH}/cmake/hsa-runtime64)
  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
endmacro()
