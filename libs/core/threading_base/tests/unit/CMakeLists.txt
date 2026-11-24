# Copyright (c) 2011 Bryce Adelstein-Lelbach
# Copyright (c) 2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    execution_agent
    scheduling_policy_idle_step
    set_thread_affinity
    set_thread_state_timed_self
    sized_function
    stack_sized_function
    suspension_timeout
    task_size_annotations
    task_size_hint
    thread_description_address
    thread_exit_callbacks
    thread_data_is_background
)

if(HPX_WITH_TRACY)
  set(tests ${tests} task_sampling)
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/ThreadingBase"
  )

  add_hpx_unit_test("modules.threading_base" ${test} ${${test}_PARAMETERS})
endforeach()

# two different annotations on one type have to be rejected, which needs a
# compiler that supports annotations
if(HPX_WITH_COMPILE_ONLY_TESTS
   AND HPX_WITH_FAIL_COMPILE_TESTS
   AND HPX_WITH_CXX26_REFLECTION
   AND HPX_WITH_CXX26_REFLECTION_ANNOTATIONS
)
  set(sources fail_task_size_annotations.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_unit_compile_test(
    "modules.threading_base" fail_task_size_annotations
    SOURCES ${sources}
    FAILURE_EXPECTED
    FOLDER "Tests/Unit/Modules/Core/ThreadingBase/CompileOnly"
  )
endif()
