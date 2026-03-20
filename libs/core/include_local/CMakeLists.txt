# Copyright (c) 2019-2023 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(include_local_headers
    hpx/any.hpp
    hpx/bit.hpp
    hpx/chrono.hpp
    hpx/condition_variable.hpp
    hpx/datapar.hpp
    hpx/exception.hpp
    hpx/execution.hpp
    hpx/format.hpp
    hpx/functional.hpp
    hpx/generator.hpp
    hpx/local.hpp
    hpx/memory.hpp
    hpx/mutex.hpp
    hpx/numeric.hpp
    hpx/optional.hpp
    hpx/semaphore.hpp
    hpx/shared_mutex.hpp
    hpx/source_location.hpp
    hpx/stop_token.hpp
    hpx/system_error.hpp
    hpx/task_block.hpp
    hpx/thread.hpp
    hpx/tuple.hpp
    hpx/type_traits.hpp
    hpx/unwrap.hpp
    hpx/include_local/iostream.hpp
    hpx/experimental/run_on_all.hpp
    hpx/experimental/scope.hpp
    hpx/experimental/task_group.hpp
)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  set(generated_include_local_headers
      hpx/algorithm.hpp
      hpx/barrier.hpp
      hpx/channel.hpp
      hpx/compute.hpp
      hpx/future.hpp
      hpx/init.hpp
      hpx/iostream.hpp
      hpx/latch.hpp
      hpx/runtime.hpp
  )
endif()

# The headers in hpx/local/ were deprecated in HPX V1.9.1
# cmake-format: off
set(include_local_compat_headers
    hpx/local/algorithm.hpp => hpx/algorithm.hpp
    hpx/local/any.hpp => hpx/any.hpp
    hpx/local/barrier.hpp => hpx/barrier.hpp
    hpx/local/channel.hpp => hpx/channel.hpp
    hpx/local/chrono.hpp => hpx/chrono.hpp
    hpx/local/compute.hpp => hpx/compute.hpp
    hpx/local/condition_variable.hpp => hpx/condition_variable.hpp
    hpx/local/datapar.hpp => hpx/datapar.hpp
    hpx/local/exception.hpp => hpx/exception.hpp
    hpx/local/execution.hpp => hpx/execution.hpp
    hpx/local/functional.hpp => hpx/functional.hpp
    hpx/local/future.hpp => hpx/future.hpp
    hpx/local/generator.hpp => hpx/generator.hpp
    hpx/local/latch.hpp => hpx/latch.hpp
    hpx/local/memory.hpp => hpx/memory.hpp
    hpx/local/mutex.hpp => hpx/mutex.hpp
    hpx/local/numeric.hpp => hpx/numeric.hpp
    hpx/local/optional.hpp => hpx/optional.hpp
    hpx/local/runtime.hpp => hpx/runtime.hpp
    hpx/local/semaphore.hpp => hpx/semaphore.hpp
    hpx/local/shared_mutex.hpp => hpx/shared_mutex.hpp
    hpx/local/source_location.hpp => hpx/source_location.hpp
    hpx/local/stop_token.hpp => hpx/stop_token.hpp
    hpx/local/system_error.hpp => hpx/system_error.hpp
    hpx/local/task_block.hpp => hpx/task_block.hpp
    hpx/local/thread.hpp => hpx/thread.hpp
    hpx/local/tuple.hpp => hpx/tuple.hpp
    hpx/local/type_traits.hpp => hpx/type_traits.hpp
    hpx/local/unwrap.hpp => hpx/unwrap.hpp
)
# cmake-format: on

include(HPX_AddModule)
add_hpx_module(
  core include_local
  GLOBAL_HEADER_GEN OFF
  HEADERS ${include_local_headers}
  COMPAT_HEADERS ${include_local_compat_headers}
  GENERATED_HEADERS ${generated_include_local_headers}
  MODULE_DEPENDENCIES
    hpx_algorithms
    hpx_assertion
    hpx_async_base
    hpx_async_combinators
    hpx_async_local
    hpx_compute_local
    hpx_datastructures
    hpx_errors
    hpx_execution
    hpx_execution_base
    hpx_executors
    hpx_format
    hpx_functional
    hpx_futures
    hpx_init_runtime_local
    hpx_lcos_local
    hpx_memory
    hpx_pack_traversal
    hpx_runtime_local
    hpx_string_util
    hpx_synchronization
    hpx_tag_invoke
    hpx_threading
    hpx_threading_base
    hpx_thread_support
    hpx_timed_execution
    hpx_timing
    hpx_type_support
    hpx_util
  CMAKE_SUBDIRS examples tests
)
