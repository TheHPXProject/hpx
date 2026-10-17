# Copyright (c) 2011 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests lockfree_fifo_double_backend min_tasks_to_steal_staged schedule_last
          schedule_hint_none_stale_queue_6978 thread_queue_get_next_threads
)

if(HPX_WITH_WORK_REQUESTING_SCHEDULERS)
  list(APPEND tests workrequesting_random_victim workrequesting_steal
       workrequesting_scheduler_mode workrequesting_suspend_resume
  )
endif()

set(min_tasks_to_steal_staged_PARAMETERS THREADS_PER_LOCALITY 4)

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
