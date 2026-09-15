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
    set_hpx_limit_798
    shared_future_continuation_order
    shared_future_then_2166
    wait_for_1751
)

set(exception_from_continuation_1613_PARAMETERS THREADS_PER_LOCALITY 2)
set(wait_for_1751_PARAMETERS THREADS_PER_LOCALITY 4)

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
