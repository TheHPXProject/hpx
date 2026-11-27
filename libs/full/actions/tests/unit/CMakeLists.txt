# Copyright (c) 2011 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests action_task_size set_thread_state thread_affinity thread_stacksize)

if(HPX_WITH_NETWORKING)
  set(tests ${tests} serialize_buffer zero_copy_serialization)
  set(serialize_buffer_PARAMETERS LOCALITIES 2 THREADS_PER_LOCALITY 2)
endif()

set(set_thread_state_PARAMETERS THREADS_PER_LOCALITY 4)
set(thread_stacksize_PARAMETERS LOCALITIES 2)
set(thread_affinity_PARAMETERS THREADS_PER_LOCALITY 4 RUN_SERIAL)
set(thread_affinity_LIBRARIES DEPENDENCIES PRIVATE Hwloc::hwloc)

if(HPX_WITH_STACKOVERFLOW_DETECTION)
  set(tests ${tests} thread_stacksize_overflow thread_stacksize_overflow_v2)
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS} ${${test}_LIBRARIES}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Full/Actions"
  )

  add_hpx_unit_test("modules.actions" ${test} ${${test}_PARAMETERS})
endforeach()

# two different annotations on one function have to be rejected, which needs a
# compiler that supports annotations
if(HPX_WITH_COMPILE_ONLY_TESTS
   AND HPX_WITH_FAIL_COMPILE_TESTS
   AND HPX_WITH_CXX26_REFLECTION
   AND HPX_WITH_CXX26_REFLECTION_ANNOTATIONS
)
  set(sources fail_action_task_size_annotations.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_unit_compile_test(
    "modules.actions" fail_action_task_size_annotations
    SOURCES ${sources}
    FAILURE_EXPECTED
    FOLDER "Tests/Unit/Modules/Full/Actions/CompileOnly"
  )
endif()

if(HPX_WITH_STACKOVERFLOW_DETECTION)
  # both tests crash on purpose, so they are judged by their output: the
  # overflow has to be reported as one, the null pointer dereference must only
  # be reported as a fatal error
  set_tests_properties(
    tests.unit.modules.actions.thread_stacksize_overflow
    PROPERTIES PASS_REGULAR_EXPRESSION
               "Stack overflow in coroutine at address 0x[0-9a-fA-F]*"
  )
  set_tests_properties(
    tests.unit.modules.actions.thread_stacksize_overflow_v2
    PROPERTIES PASS_REGULAR_EXPRESSION "\\{what\\}: " FAIL_REGULAR_EXPRESSION
               "Stack overflow in coroutine"
  )
endif()
