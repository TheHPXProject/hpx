# Copyright (c) 2007-2015 Hartmut Kaiser
# Copyright (c) 2013,2016 Maciej Brodowicz
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests discover_counters_1787 late_counter_registration_4627
          registry_thread_safety_7563 statistics_2666 uptime_1737
)

set(registry_thread_safety_7563_PARAMETERS THREADS_PER_LOCALITY 4)

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  set(tests ${tests} dynamic_counters_loaded_1508)
  set(dynamic_counters_loaded_1508_FLAGS DEPENDENCIES memory_counters_component)
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Full/PerformanceCounters"
  )

  add_hpx_regression_test(
    "modules.performance_counters" ${test} ${${test}_PARAMETERS}
  )

endforeach()
