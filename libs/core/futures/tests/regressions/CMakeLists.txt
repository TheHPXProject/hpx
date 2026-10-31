# Copyright (c) 2007-2017 Hartmut Kaiser
# Copyright (c) 2011-2012 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    exception_from_continuation_1613
    future_2667
    future_790
    future_uncompleted_7490
    future_unwrap_878
    future_unwrap_1182
    run_as_child_hint_7724
    runs_child_race_7724
    set_hpx_limit_798
    shared_future_continuation_order
    shared_future_then_2166
    stackful_direct_execution_7713
    stackless_direct_execution_7708
    wait_for_1751
    wait_for_7915
)

if(HPX_WITH_THREAD_CUMULATIVE_COUNTS)
  list(APPEND tests count_direct_children_7830)
endif()

set(exception_from_continuation_1613_PARAMETERS THREADS_PER_LOCALITY 2)
set(wait_for_1751_PARAMETERS THREADS_PER_LOCALITY 4)
set(run_as_child_hint_7724_PARAMETERS THREADS_PER_LOCALITY 4)
set(runs_child_race_7724_PARAMETERS THREADS_PER_LOCALITY 8)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Core/Futures"
  )

  add_hpx_regression_test("modules.futures" ${test} ${${test}_PARAMETERS})
endforeach()

# with more workers a leftover queue entry can also be dequeued while the parent
# is still running the child
add_hpx_regression_test(
  "modules.futures" stackless_direct_execution_7708_4
  EXECUTABLE stackless_direct_execution_7708
  PSEUDO_DEPS_NAME stackless_direct_execution_7708
  THREADS_PER_LOCALITY 4
)

add_hpx_regression_test(
  "modules.futures" stackful_direct_execution_7713_4
  EXECUTABLE stackful_direct_execution_7713
  PSEUDO_DEPS_NAME stackful_direct_execution_7713
  THREADS_PER_LOCALITY 4
)

# every child has to be counted once also when workers can run some of them
if(HPX_WITH_THREAD_CUMULATIVE_COUNTS)
  add_hpx_regression_test(
    "modules.futures" count_direct_children_7830_4
    EXECUTABLE count_direct_children_7830
    PSEUDO_DEPS_NAME count_direct_children_7830
    THREADS_PER_LOCALITY 4
  )
endif()

# compile only tests
if(HPX_WITH_COMPILE_ONLY_TESTS)
  set(compile_tests future_range_ambiguity_2032)

  if(HPX_WITH_FAIL_COMPILE_TESTS)
    set(fail_compile_tests fail_future_2667)
    foreach(fail_compile_test ${fail_compile_tests})
      set(${fail_compile_test}_FLAGS FAILURE_EXPECTED)
    endforeach()

    set(compile_tests ${compile_tests} ${fail_compile_tests})
  endif()

  foreach(compile_test ${compile_tests})
    set(sources ${compile_test}.cpp)

    source_group("Source Files" FILES ${sources})

    add_hpx_regression_compile_test(
      "modules.futures" ${compile_test}
      SOURCES ${sources} ${${compile_test}_FLAGS}
      FOLDER "Tests/Regressions/Modules/Core/Futures/CompileOnly"
    )
  endforeach()
endif()
