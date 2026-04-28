# Copyright (c) 2020-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/executors/include
set(executors_headers
    hpx/executors/annotating_executor.hpp
    hpx/executors/current_executor.hpp
    hpx/executors/guided_pool_executor.hpp
    hpx/executors/async.hpp
    hpx/executors/dataflow.hpp
    hpx/executors/detail/bulk_invoke_helper.hpp
    hpx/executors/detail/hierarchical_spawning.hpp
    hpx/executors/detail/index_queue_spawning.hpp
    hpx/executors/detail/index_queue_spawning_result.hpp
    hpx/executors/detail/index_queue_spawning_void.hpp
    hpx/executors/execute_on.hpp
    hpx/executors/exception_list.hpp
    hpx/executors/execution_policy_annotation.hpp
    hpx/executors/execution_policy_fwd.hpp
    hpx/executors/execution_policy_mappings.hpp
    hpx/executors/execution_policy_parameters.hpp
    hpx/executors/execution_policy_scheduling_property.hpp
    hpx/executors/execution_policy.hpp
    hpx/executors/explicit_scheduler_executor.hpp
    hpx/executors/fork_join_executor.hpp
    hpx/executors/limiting_executor.hpp
    hpx/executors/macros.hpp
    hpx/executors/parallel_executor_aggregated.hpp
    hpx/executors/parallel_executor.hpp
    hpx/executors/parallel_scheduler.hpp
    hpx/executors/parallel_scheduler_backend.hpp
    hpx/executors/post.hpp
    hpx/executors/restricted_thread_pool_executor.hpp
    hpx/executors/scheduler_executor.hpp
    hpx/executors/sequenced_executor.hpp
    hpx/executors/service_executors.hpp
    hpx/executors/std_execution_policy.hpp
    hpx/executors/sync.hpp
    hpx/executors/thread_pool_executor.hpp
    hpx/executors/thread_pool_scheduler.hpp
    hpx/executors/thread_pool_scheduler_bulk.hpp
)

set(executors_macro_headers hpx/executors/macros.hpp)

if(HPX_WITH_DATAPAR)
  set(executors_headers
      ${executors_headers} hpx/executors/datapar/execution_policy_fwd.hpp
      hpx/executors/datapar/execution_policy_mappings.hpp
      hpx/executors/datapar/execution_policy.hpp
  )
endif()

if(HPX_LIKWID_WITH_LIKWID)
  set(executors_headers ${executors_headers} hpx/executors/likwid_executor.hpp)
  set(additional_dependencies hpx_likwid)
endif()

# Default location is $HPX_ROOT/libs/executors/include_compatibility
# cmake-format: off
set(executors_compat_headers
    hpx/execution/apply.hpp => hpx/include/parallel_executors.hpp
    hpx/execution/executors.hpp => hpx/include/parallel_executors.hpp
    hpx/execution/execution_policy_fwd.hpp => hpx/execution.hpp
    hpx/execution/execution_policy.hpp => hpx/execution.hpp
    hpx/execution/executors/current_executor.hpp => hpx/execution.hpp
    hpx/execution/executors/guided_pool_executor.hpp => hpx/include/parallel_executors.hpp
    hpx/execution/executors/limiting_executor.hpp => hpx/include/parallel_executors.hpp
    hpx/execution/executors/parallel_executor.hpp => hpx/execution.hpp
    hpx/execution/executors/parallel_executor_aggregated.hpp => hpx/include/parallel_executors.hpp
    hpx/execution/executors/restricted_thread_pool_executor.hpp => hpx/include/parallel_executors.hpp
    hpx/execution/executors/sequenced_executor.hpp => hpx/include/parallel_executors.hpp
    hpx/parallel/executors.hpp => hpx/include/parallel_executors.hpp
    hpx/parallel/exception_list.hpp => hpx/execution.hpp
    hpx/parallel/execution_policy_fwd.hpp => hpx/execution.hpp
    hpx/parallel/execution_policy.hpp => hpx/execution.hpp
    hpx/parallel/executors/parallel_executor.hpp => hpx/execution.hpp
    hpx/parallel/executors/parallel_executor_aggregated.hpp => hpx/include/parallel_executors.hpp
    hpx/parallel/executors/sequenced_executor.hpp => hpx/execution.hpp
    hpx/parallel/executors/thread_pool_executor.hpp => hpx/execution.hpp
)

if(HPX_WITH_DATAPAR)
  set(executors_compat_headers ${executors_compat_headers}
      hpx/execution/datapar/execution_policy_fwd.hpp => hpx/include/parallel_executors.hpp
      hpx/execution/datapar/execution_policy.hpp => hpx/include/parallel_executors.hpp
      hpx/parallel/datapar/execution_policy_fwd.hpp => hpx/include/parallel_executors.hpp
      hpx/parallel/datapar/execution_policy.hpp => hpx/include/parallel_executors.hpp
  )
endif()
# cmake-format: on

set(executors_sources current_executor.cpp exception_list_callbacks.cpp
                      fork_join_executor.cpp service_executors.cpp
)

include(HPX_AddModule)
add_hpx_module(
  core executors
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${executors_sources}
  HEADERS ${executors_headers}
  MACRO_HEADERS ${executors_macro_headers}
  COMPAT_HEADERS ${executors_compat_headers}
  MODULE_DEPENDENCIES
    hpx_allocator_support
    hpx_async_base
    hpx_async_combinators
    hpx_concepts
    hpx_concurrency
    hpx_config
    hpx_coroutines
    hpx_datastructures
    hpx_debugging
    hpx_errors
    hpx_execution
    hpx_execution_base
    hpx_format
    hpx_functional
    hpx_futures
    hpx_hardware
    hpx_io_service
    hpx_iterator_support
    hpx_itt_notify
    hpx_memory
    hpx_pack_traversal
    hpx_properties
    hpx_resource_partitioner
    hpx_serialization
    hpx_synchronization
    hpx_tag_invoke
    hpx_timing
    hpx_threading
    hpx_threading_base
    hpx_tracing
    hpx_topology
    hpx_type_support
    ${additional_dependencies}
  CMAKE_SUBDIRS examples tests
)
