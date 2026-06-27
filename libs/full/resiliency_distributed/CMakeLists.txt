# Copyright (c) 2019-2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(resiliency_distributed_headers
    hpx/resiliency_distributed/async_replay_distributed.hpp
    hpx/resiliency_distributed/async_replicate_distributed.hpp
    hpx/resiliency_distributed/resiliency_distributed.hpp
)

include(HPX_AddModule)
add_hpx_module(
  full resiliency_distributed
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${resiliency_distributed_sources}
  HEADERS ${resiliency_distributed_headers}
  EXCLUDE_FROM_GLOBAL_HEADER
    "hpx/resiliency_distributed/resiliency_distributed.hpp"
  DEPENDENCIES hpx_core
  MODULE_DEPENDENCIES hpx_actions_base hpx_async_distributed hpx_naming
  CMAKE_SUBDIRS examples tests
)
