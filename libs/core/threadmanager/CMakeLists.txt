# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(threadmanager_headers hpx/threadmanager/threadmanager.hpp
                          hpx/threadmanager/threadmanager_fwd.hpp
)

# cmake-format: off
set(threadmanager_compat_headers
    hpx/threadmanager.hpp => hpx/modules/threadmanager.hpp
    hpx/runtime/threads/threadmanager.hpp => hpx/include/threadmanager.hpp
)
# cmake-format: on

set(threadmanager_sources threadmanager.cpp)

include(HPX_AddModule)
add_hpx_module(
  core threadmanager
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${threadmanager_sources}
  HEADERS ${threadmanager_headers}
  COMPAT_HEADERS ${threadmanager_compat_headers}
  MODULE_DEPENDENCIES
    hpx_async_combinators
    hpx_concurrency
    hpx_errors
    hpx_execution
    hpx_execution_base
    hpx_futures
    hpx_io_service
    hpx_program_options
    hpx_logging
    hpx_resource_partitioner
    hpx_runtime_configuration
    hpx_schedulers
    hpx_threading_base
    hpx_thread_pools
    hpx_thread_pool_util
    hpx_timing
    hpx_topology
    hpx_type_support
    hpx_util
  CMAKE_SUBDIRS examples tests
)
