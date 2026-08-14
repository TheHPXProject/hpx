# Copyright (c) 2017-2023 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    background_scheduler
    cross_pool_injection
    named_pool_executor
    resource_partitioner_info
    scheduler_binding_check
    scheduler_priority_check
    scheduling_policy_wiring
    shutdown_suspended_pus
    suspend_disabled
    suspend_pool
    suspend_pool_external
    suspend_runtime
    suspend_thread
    suspend_thread_external
    # suspend_thread_timed
    used_pus
)

set(background_scheduler_PARAMETERS THREADS_PER_LOCALITY 2)

set(cross_pool_injection_PARAMETERS THREADS_PER_LOCALITY -1 TIMEOUT 300)
set(scheduler_binding_check_PARAMETERS THREADS_PER_LOCALITY -1)

set(named_pool_executor_PARAMETERS THREADS_PER_LOCALITY 4)
set(resource_partitioner_info_PARAMETERS THREADS_PER_LOCALITY 4)
set(used_pus_PARAMETERS THREADS_PER_LOCALITY 4 RUN_SERIAL)

set(scheduler_priority_check_PARAMETERS THREADS_PER_LOCALITY -1)
set(shutdown_suspended_pus_PARAMETERS THREADS_PER_LOCALITY 4)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(additional_parameters "--hpx:ini=hpx.stacks.use_guard_pages=0")
endif()

set(suspend_disabled_PARAMETERS THREADS_PER_LOCALITY 4 ${additional_parameters})
set(suspend_pool_PARAMETERS THREADS_PER_LOCALITY 4 ${additional_parameters})
set(suspend_pool_external_PARAMETERS THREADS_PER_LOCALITY 4
                                     ${additional_parameters}
)
set(suspend_runtime_PARAMETERS THREADS_PER_LOCALITY 4 ${additional_parameters})
set(suspend_thread_PARAMETERS THREADS_PER_LOCALITY 4 ${additional_parameters})
set(suspend_thread_external_PARAMETERS THREADS_PER_LOCALITY 4
                                       ${additional_parameters}
)
set(suspend_thread_timed_PARAMETERS THREADS_PER_LOCALITY 4
                                    ${additional_parameters}
)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/ResourcePartitioner/"
  )

  add_hpx_unit_test(
    "modules.resource_partitioner" ${test} ${${test}_PARAMETERS}
  )

endforeach()
