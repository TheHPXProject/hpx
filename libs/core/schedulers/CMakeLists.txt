# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(schedulers_headers
    hpx/schedulers/background_scheduler.hpp
    hpx/schedulers/deadlock_detection.hpp
    hpx/schedulers/local_priority_queue_scheduler.hpp
    hpx/schedulers/local_queue_scheduler.hpp
    hpx/schedulers/lockfree_queue_backends.hpp
    hpx/schedulers/macros.hpp
    hpx/schedulers/maintain_queue_wait_times.hpp
    hpx/schedulers/queue_helpers.hpp
    hpx/schedulers/queue_holder_numa.hpp
    hpx/schedulers/queue_holder_thread.hpp
    hpx/schedulers/shared_priority_queue_scheduler.hpp
    hpx/schedulers/static_priority_queue_scheduler.hpp
    hpx/schedulers/static_queue_scheduler.hpp
    hpx/schedulers/thread_queue.hpp
    hpx/schedulers/thread_queue_mc.hpp
)

if(HPX_WITH_WORK_REQUESTING_SCHEDULERS)
  set(schedulers_headers ${schedulers_headers}
                         hpx/schedulers/local_workrequesting_scheduler.hpp
  )
endif()

set(schedulers_macro_headers hpx/schedulers/macros.hpp)

# cmake-format: off
set(schedulers_compat_headers
    hpx/runtime/threads/policies/deadlock_detection.hpp => hpx/modules/schedulers.hpp
    hpx/runtime/threads/policies/local_priority_queue_scheduler.hpp => hpx/modules/schedulers.hpp
    hpx/runtime/threads/policies/local_queue_scheduler.hpp => hpx/modules/schedulers.hpp
    hpx/runtime/threads/policies/lockfree_queue_backends.hpp => hpx/modules/schedulers.hpp
    hpx/runtime/threads/policies/maintain_queue_wait_times.hpp => hpx/modules/schedulers.hpp
    hpx/runtime/threads/policies/queue_helpers.hpp => hpx/modules/schedulers.hpp
    hpx/runtime/threads/policies/shared_priority_queue_scheduler.hpp => hpx/modules/schedulers.hpp
    hpx/runtime/threads/policies/static_priority_queue_scheduler.hpp => hpx/modules/schedulers.hpp
    hpx/runtime/threads/policies/static_queue_scheduler.hpp => hpx/modules/schedulers.hpp
    hpx/runtime/threads/policies/thread_queue.hpp => hpx/modules/schedulers.hpp
    hpx/runtime/threads/policies/thread_queue_mc.hpp => hpx/modules/schedulers.hpp
    hpx/runtime/threads/policies/schedulers.hpp => hpx/modules/schedulers.hpp
)
# cmake-format: on

set(schedulers_sources deadlock_detection.cpp maintain_queue_wait_times.cpp)

include(HPX_AddModule)
add_hpx_module(
  core schedulers
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${schedulers_sources}
  HEADERS ${schedulers_headers}
  MACRO_HEADERS ${schedulers_macro_headers}
  COMPAT_HEADERS ${schedulers_compat_headers}
  SOURCES ${schedulers_sources}
  MODULE_DEPENDENCIES
    hpx_affinity
    hpx_allocator_support
    hpx_assertion
    hpx_concurrency
    hpx_config
    hpx_datastructures
    hpx_debugging
    hpx_errors
    hpx_format
    hpx_functional
    hpx_logging
    hpx_synchronization
    hpx_threading_base
    hpx_thread_support
    hpx_timing
    hpx_topology
    hpx_tracing
    hpx_type_support
    hpx_util
  CMAKE_SUBDIRS examples tests
)
