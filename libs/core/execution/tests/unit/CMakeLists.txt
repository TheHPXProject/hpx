# Copyright (c) 2014-2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    algorithm_as_sender
    algorithm_bulk
    algorithm_ensure_started
    algorithm_execute
    algorithm_just
    algorithm_just_error
    algorithm_just_stopped
    algorithm_let_error
    algorithm_let_stopped
    algorithm_let_value
    algorithm_run_loop
    algorithm_split
    algorithm_start_detached
    algorithm_sync_wait
    algorithm_sync_wait_with_variant
    algorithm_then
    algorithm_transfer
    algorithm_transfer_just
    algorithm_transfer_when_all
    algorithm_when_all
    algorithm_when_all_vector
    bulk_async
    environment_queries
    executor_parameters
    executor_parameters_dispatching
    executor_parameters_timer_hooks
    executor_parameters_partition_hooks
    forwarding_env_query
    forwarding_scheduler_query
    forwarding_sender_query
    forward_progress_guarantee
    future_then_executor
    minimal_async_executor
    minimal_sync_executor
    persistent_executor_parameters
    rebind_executor_parameters
)

set(future_then_executor_PARAMETERS THREADS_PER_LOCALITY 4)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  foreach(test ${tests})
    set(sources ${test}.cpp)

    set(${test}_PARAMETERS THREADS_PER_LOCALITY 4)

    source_group("Source Files" FILES ${sources})

    set(folder_name "Tests/Unit/Modules/Core/Execution")

    # add example executable
    add_hpx_executable(
      ${test}_test INTERNAL_FLAGS
      SOURCES ${sources} ${${test}_FLAGS}
      EXCLUDE_FROM_ALL
      HPX_PREFIX ${HPX_BUILD_PREFIX}
      FOLDER ${folder_name}
    )

    target_link_libraries(${test}_test PRIVATE hpx_execution_test_utilities)

    add_hpx_unit_test("modules.execution" ${test} ${${test}_PARAMETERS})

  endforeach()
endif()
