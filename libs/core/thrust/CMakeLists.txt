# Copyright (c) 2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT ((HPX_WITH_CUDA OR HPX_WITH_HIP) AND HPX_WITH_THRUST))
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# with HIP, Thrust is rocThrust (found in HPX_SetupHIP)
set(thrust_dependencies)
if(HPX_WITH_HIP)
  set(thrust_dependencies roc::rocthrust)
endif()

set(thrust_headers
    hpx/thrust/policy.hpp
    hpx/thrust/thrust_headers.hpp
    hpx/thrust/algorithms.hpp
    hpx/thrust/detail/algorithm_map.hpp
    hpx/thrust/serialization/detail/device_staging.hpp
    hpx/thrust/serialization/detail/host_accessible.hpp
    hpx/thrust/serialization/device_vector.hpp
    hpx/thrust/serialization/host_vector.hpp
    hpx/thrust/serialization/universal_vector.hpp
)

# cmake-format: off
set(thrust_compat_headers)
# cmake-format: on

# moves thrust::device_vector data through the archives (pinned staging buffers,
# asynchronous copies)
set(thrust_sources detail/device_staging.cpp)

include(HPX_AddModule)
add_hpx_module(
  core thrust
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${thrust_sources}
  HEADERS ${thrust_headers}
  EXCLUDE_FROM_GLOBAL_HEADER "hpx/thrust/thrust_headers.hpp"
  COMPAT_HEADERS ${thrust_compat_headers}
  DEPENDENCIES ${thrust_dependencies}
  MODULE_DEPENDENCIES
    hpx_algorithms
    hpx_async_cuda
    hpx_concepts
    hpx_errors
    hpx_execution
    hpx_execution_base
    hpx_executors
    hpx_functional
    hpx_futures
    hpx_lcos_local
    hpx_runtime_local
    hpx_serialization
    hpx_synchronization
    hpx_threading_base
  CMAKE_SUBDIRS examples tests
)
