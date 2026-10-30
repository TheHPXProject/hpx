# Copyright (c) 2007-2015 Hartmut Kaiser
# Copyright (c) 2013,2016 Maciej Brodowicz
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests papi_counters_active_interface papi_counters_basic_functions
          papi_counters_segfault_1890
)

# The papi_counters_basic_functions test depends on floating point counters
# being available. They are not always available so we only add it as an
# executable, not a test.
set(papi_counters_basic_functions_NOTEST TRUE)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources}
    COMPONENT_DEPENDENCIES papi_counters ${${test}_FLAGS}
    EXCLUDE_FROM_ALL ${HPX_WITH_CXX_MODULES_OPTION}
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Components/Counters/Papi"
  )

  if(NOT "${${test}_NOTEST}")
    add_hpx_regression_test(
      "components.papi_counters" ${test} ${${test}_PARAMETERS}
    )
  endif()
endforeach()

target_link_libraries(papi_counters_active_interface_test PRIVATE Papi::papi)
set_tests_properties(
  tests.regressions.components.papi_counters.papi_counters_active_interface
  PROPERTIES SKIP_REGULAR_EXPRESSION
             "SKIP: PAPI_SR_INS is unavailable on this machine"
)
