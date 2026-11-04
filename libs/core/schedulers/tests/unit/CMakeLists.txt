# Copyright (c) 2011 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    lockfree_fifo_double_backend
    min_tasks_to_steal_staged
    numa_stealing_domains
    queue_length_all_threads
    schedule_last
    schedule_hint_none_stale_queue_6978
    scheduler_mode_overrides
    shared_priority_queue_external_cleanup
    steal_pending_early
    steal_staged_early
    thread_queue_get_next_threads
    thread_queue_parameters
)

if(HPX_WITH_WORK_REQUESTING_SCHEDULERS)
  list(
    APPEND
    tests
    workrequesting_random_victim
    workrequesting_run_as_child
    workrequesting_steal
    workrequesting_scheduler_mode
    workrequesting_suspend_resume
  )
endif()

set(min_tasks_to_steal_staged_PARAMETERS THREADS_PER_LOCALITY 4)
set(queue_length_all_threads_PARAMETERS THREADS_PER_LOCALITY 1)

# with a single worker the task's last reference is dropped by the external
# thread, see the test
set(shared_priority_queue_external_cleanup_PARAMETERS THREADS_PER_LOCALITY 1)

set(steal_staged_early_PARAMETERS THREADS_PER_LOCALITY 4)
set(steal_pending_early_PARAMETERS THREADS_PER_LOCALITY 4)

# ##############################################################################
foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS} ${${test}_LIBRARIES}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/Schedulers"
  )

  add_hpx_unit_test("modules.schedulers" ${test} ${${test}_PARAMETERS})
endforeach()

# with a single worker nobody else could convert the staged tasks
add_hpx_unit_test(
  "modules.schedulers" min_tasks_to_steal_staged_single_worker
  EXECUTABLE min_tasks_to_steal_staged
  PSEUDO_DEPS_NAME min_tasks_to_steal_staged
  THREADS_PER_LOCALITY 1
)

# the work-requesting scheduler sends tasks to the workers requesting them, it
# has to honor the minimum as well (--scheduler is used instead of --hpx:queuing
# to avoid conflicts with HPX_WITH_TESTS_COMMAND_LINE)
if(HPX_WITH_WORK_REQUESTING_SCHEDULERS)
  add_hpx_unit_test(
    "modules.schedulers" min_tasks_to_steal_staged_workrequesting
    EXECUTABLE min_tasks_to_steal_staged
    PSEUDO_DEPS_NAME min_tasks_to_steal_staged
    THREADS_PER_LOCALITY 4
    ARGS --scheduler=workrequesting
  )
endif()

# the same tests in fast idle mode
add_hpx_unit_test(
  "modules.schedulers" steal_staged_early_fast_idle
  EXECUTABLE steal_staged_early
  PSEUDO_DEPS_NAME steal_staged_early
  THREADS_PER_LOCALITY 4
  ARGS --mode=fast-idle
)
add_hpx_unit_test(
  "modules.schedulers" steal_pending_early_fast_idle
  EXECUTABLE steal_pending_early
  PSEUDO_DEPS_NAME steal_pending_early
  THREADS_PER_LOCALITY 4
  ARGS --mode=fast-idle
)
