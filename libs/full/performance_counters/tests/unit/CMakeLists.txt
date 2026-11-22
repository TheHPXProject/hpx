# Copyright (c) 2007-2018 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests all_counters counter_raw_values path_elements reinit_counters
          scheduler_tuning_counters
)

# a pool with the tuned scheduler and one with another scheduler
set(scheduler_tuning_counters_PARAMETERS THREADS_PER_LOCALITY 2)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Full/PerformanceCounters/"
  )

  add_hpx_unit_test(
    "modules.performance_counters" ${test} ${${test}_PARAMETERS}
  )

endforeach()
