# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(futures_headers
    hpx/futures/detail/execute_thread.hpp
    hpx/futures/detail/future_data.hpp
    hpx/futures/detail/future_transforms.hpp
    hpx/futures/future.hpp
    hpx/futures/future_fwd.hpp
    hpx/futures/futures_factory.hpp
    hpx/futures/future_or_value.hpp
    hpx/futures/macros.hpp
    hpx/futures/monadic_operations.hpp
    hpx/futures/packaged_continuation.hpp
    hpx/futures/packaged_task.hpp
    hpx/futures/promise.hpp
    hpx/futures/traits/acquire_future.hpp
    hpx/futures/traits/acquire_shared_state.hpp
    hpx/futures/traits/detail/future_await_traits.hpp
    hpx/futures/traits/detail/future_traits.hpp
    hpx/futures/traits/future_access.hpp
    hpx/futures/traits/future_then_result.hpp
    hpx/futures/traits/future_traits.hpp
    hpx/futures/traits/get_remote_result.hpp
    hpx/futures/traits/is_future.hpp
    hpx/futures/traits/is_future_range.hpp
    hpx/futures/traits/is_future_tuple.hpp
    hpx/futures/traits/promise_local_result.hpp
    hpx/futures/traits/promise_remote_result.hpp
)

set(futures_macro_headers hpx/futures/macros.hpp)

# cmake-format: off
set(futures_compat_headers
    hpx/lcos/future.hpp => hpx/future.hpp
    hpx/lcos/local/futures_factory.hpp => hpx/future.hpp
    hpx/lcos/local/packaged_task.hpp => hpx/future.hpp
    hpx/lcos/local/packaged_continuation.hpp => hpx/future.hpp
    hpx/lcos/local/promise.hpp => hpx/future.hpp
    hpx/traits/acquire_future.hpp => hpx/future.hpp
    hpx/traits/acquire_shared_state.hpp => hpx/future.hpp
    hpx/traits/future_access.hpp => hpx/future.hpp
    hpx/traits/future_then_result.hpp => hpx/future.hpp
    hpx/traits/future_traits.hpp => hpx/future.hpp
    hpx/traits/get_remote_result.hpp => hpx/future.hpp
    hpx/traits/is_future.hpp => hpx/future.hpp
    hpx/traits/is_future_range.hpp => hpx/future.hpp
    hpx/traits/is_future_tuple.hpp => hpx/future.hpp
    hpx/traits/promise_local_result.hpp => hpx/future.hpp
    hpx/traits/promise_remote_result.hpp => hpx/future.hpp
)
# cmake-format: on

set(futures_sources detail/execute_thread.cpp future_data.cpp)

include(HPX_AddModule)
add_hpx_module(
  core futures
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${futures_sources}
  HEADERS ${futures_headers}
  MACRO_HEADERS ${futures_macro_headers}
  COMPAT_HEADERS ${futures_compat_headers}
  ADD_TO_GLOBAL_HEADER "hpx/futures/detail/future_transforms.hpp"
                       "hpx/futures/detail/future_data.hpp"
  MODULE_DEPENDENCIES
    hpx_allocator_support
    hpx_assertion
    hpx_async_base
    hpx_concepts
    hpx_concurrency
    hpx_config
    hpx_contracts
    hpx_coroutines
    hpx_datastructures
    hpx_errors
    hpx_execution_base
    hpx_functional
    hpx_iterator_support
    hpx_logging
    hpx_lock_registration
    hpx_memory
    hpx_serialization
    hpx_synchronization
    hpx_tag_invoke
    hpx_timing
    hpx_threading_base
    hpx_thread_support
    hpx_type_support
    hpx_util
  CMAKE_SUBDIRS examples tests
)
