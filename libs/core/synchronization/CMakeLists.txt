# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/synchronization/include
set(synchronization_headers
    hpx/synchronization/barrier.hpp
    hpx/synchronization/binary_semaphore.hpp
    hpx/synchronization/channel_mpmc.hpp
    hpx/synchronization/channel_mpsc.hpp
    hpx/synchronization/channel_spsc.hpp
    hpx/synchronization/condition_variable.hpp
    hpx/synchronization/counting_semaphore.hpp
    hpx/synchronization/detail/condition_variable.hpp
    hpx/synchronization/detail/counting_semaphore.hpp
    hpx/synchronization/detail/range_mutex_impl.hpp
    hpx/synchronization/detail/sliding_semaphore.hpp
    hpx/synchronization/event.hpp
    hpx/synchronization/latch.hpp
    hpx/synchronization/lock_types.hpp
    hpx/synchronization/macros.hpp
    hpx/synchronization/mutex.hpp
    hpx/synchronization/no_mutex.hpp
    hpx/synchronization/once.hpp
    hpx/synchronization/range_mutex.hpp
    hpx/synchronization/recursive_mutex.hpp
    hpx/synchronization/shared_mutex.hpp
    hpx/synchronization/sliding_semaphore.hpp
    hpx/synchronization/spinlock.hpp
    hpx/synchronization/spinlock_pool.hpp
    hpx/synchronization/stop_token.hpp
)

set(synchronization_macro_headers hpx/synchronization/macros.hpp)

# Default location is $HPX_ROOT/libs/synchronization/include_compatibility
# cmake-format: off
set(synchronization_compat_headers
    hpx/lcos/local/barrier.hpp => hpx/barrier.hpp
    hpx/lcos/local/condition_variable.hpp => hpx/condition_variable.hpp
    hpx/lcos/local/counting_semaphore.hpp => hpx/counting_semaphore.hpp
    hpx/lcos/local/event.hpp => hpx/modules/synchronization.hpp
    hpx/lcos/local/latch.hpp => hpx/latch.hpp
    hpx/lcos/local/mutex.hpp => hpx/mutex.hpp
    hpx/lcos/local/no_mutex.hpp => hpx/mutex.hpp
    hpx/lcos/local/once.hpp => hpx/mutex.hpp
    hpx/lcos/local/recursive_mutex.hpp => hpx/mutex.hpp
    hpx/lcos/local/shared_mutex.hpp => hpx/shared_mutex.hpp
    hpx/lcos/local/sliding_semaphore.hpp => hpx/modules/synchronization.hpp
    hpx/lcos/local/spinlock.hpp => hpx/mutex.hpp
    hpx/lcos/local/spinlock_no_backoff.hpp => hpx/modules/synchronization.hpp
    hpx/lcos/local/spinlock_pool.hpp => hpx/modules/synchronization.hpp
    hpx/synchronization.hpp => hpx/modules/synchronization.hpp
)
# cmake-format: on

set(synchronization_sources
    detail/condition_variable.cpp detail/counting_semaphore.cpp
    detail/sliding_semaphore.cpp local_barrier.cpp mutex.cpp stop_token.cpp
)

if(HPX_WITH_TRACY)
  set(additional_dependencies hpx_tracy)
endif()

include(HPX_AddModule)
add_hpx_module(
  core synchronization
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${synchronization_sources}
  HEADERS ${synchronization_headers}
  MACRO_HEADERS ${synchronization_macro_headers}
  COMPAT_HEADERS ${synchronization_compat_headers}
  MODULE_DEPENDENCIES
    hpx_config
    hpx_allocator_support
    hpx_assertion
    hpx_datastructures
    hpx_concurrency
    hpx_coroutines
    hpx_errors
    hpx_execution_base
    hpx_functional
    hpx_hashing
    hpx_lock_registration
    hpx_logging
    hpx_memory
    hpx_tag_invoke
    hpx_threading_base
    hpx_thread_support
    hpx_timing
    hpx_topology
    hpx_tracing
    hpx_type_support
    ${additional_dependencies}
  CMAKE_SUBDIRS examples tests
)
