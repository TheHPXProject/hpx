# Copyright (c) 2020-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    annotating_executor
    annotation_property
    created_executor
    execution_policy_mappings
    explicit_scheduler_executor
    fork_join_executor
    fork_join_executor_from
    limiting_executor
    parallel_executor
    parallel_executor_parameters
    parallel_fork_executor
    parallel_policy_executor
    parallel_scheduler
    polymorphic_executor
    scheduler_executor
    sequenced_executor
    service_executors
    shared_parallel_executor
    standalone_thread_pool_executor
    thread_pool_scheduler
)

if(HPX_WITH_CXX17_STD_EXECUTION_POLICES)
  set(tests ${tests} std_execution_policies)
endif()

if(HPX_LIKWID_WITH_LIKWID)
  set(tests ${tests} likwid_executor)
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  set(${test}_PARAMETERS THREADS_PER_LOCALITY 4)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Core/Executors")

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER ${folder_name}
  )

  add_hpx_unit_test("modules.executors" ${test} ${${test}_PARAMETERS})

endforeach()
