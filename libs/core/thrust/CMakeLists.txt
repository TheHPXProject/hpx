# Copyright (c) 2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT (HPX_WITH_CUDA AND HPX_WITH_THRUST))
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(thrust_headers
    hpx/thrust/policy.hpp hpx/thrust/thrust_headers.hpp
    hpx/thrust/algorithms.hpp hpx/thrust/detail/algorithm_map.hpp
)

# cmake-format: off
set(thrust_compat_headers)
# cmake-format: on

set(thrust_sources)

include(HPX_AddModule)
add_hpx_module(
  core thrust
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${thrust_sources}
  HEADERS ${thrust_headers}
  EXCLUDE_FROM_GLOBAL_HEADER "hpx/thrust/thrust_headers.hpp"
  COMPAT_HEADERS ${thrust_compat_headers}
  MODULE_DEPENDENCIES
    hpx_algorithms
    hpx_async_cuda
    hpx_concepts
    hpx_execution
    hpx_execution_base
    hpx_executors
    hpx_functional
    hpx_futures
    hpx_threading_base
  CMAKE_SUBDIRS examples tests
)
