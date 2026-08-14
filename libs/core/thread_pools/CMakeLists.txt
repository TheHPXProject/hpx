# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(thread_pools_headers
    hpx/thread_pools/detail/background_thread.hpp
    hpx/thread_pools/detail/scheduling_callbacks.hpp
    hpx/thread_pools/detail/scheduling_counters.hpp
    hpx/thread_pools/detail/scheduling_log.hpp
    hpx/thread_pools/detail/scoped_background_timer.hpp
    hpx/thread_pools/scheduled_thread_pool.hpp
    hpx/thread_pools/scheduled_thread_pool_impl.hpp
    hpx/thread_pools/scheduling_loop.hpp
)

# cmake-format: off
set(thread_pools_compat_headers
    hpx/runtime/threads/thread_pools.hpp => hpx/modules/thread_pools.hpp
    hpx/runtime/threads/scoped_background_timer.hpp => hpx/modules/thread_pools.hpp
)
# cmake-format: on

set(thread_pools_sources detail/background_thread.cpp detail/scheduling_log.cpp
                         scheduled_thread_pool.cpp
)

include(HPX_AddModule)
add_hpx_module(
  core thread_pools
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${thread_pools_sources}
  HEADERS ${thread_pools_headers}
  COMPAT_HEADERS ${thread_pools_compat_headers}
  ADD_TO_GLOBAL_HEADER "hpx/thread_pools/detail/scheduling_callbacks.hpp"
                       "hpx/thread_pools/detail/background_thread.hpp"
  MODULE_DEPENDENCIES
    hpx_affinity
    hpx_assertion
    hpx_concurrency
    hpx_config
    hpx_coroutines
    hpx_debugging
    hpx_errors
    hpx_execution_base
    hpx_functional
    hpx_logging
    hpx_schedulers
    hpx_tag_invoke
    hpx_threading_base
    hpx_tracing
    hpx_topology
  CMAKE_SUBDIRS examples tests
)
