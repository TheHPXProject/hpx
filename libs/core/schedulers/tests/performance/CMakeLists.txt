# Copyright (c) 2019-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(benchmarks scheduler_latency_report scheduler_throughput_report)

# As a test, the latency report runs only one timed step of each case (the
# rounds of a step wait for each other, which takes long in debug builds); the
# perftest below runs the default number of steps.
set(scheduler_latency_report_PARAMETERS THREADS_PER_LOCALITY 4 ARGS
                                        --test_count=1
)
set(scheduler_throughput_report_PARAMETERS THREADS_PER_LOCALITY 4)

# The reports run on 20 cores, where the placement of the scheduler's per-worker
# data and the waking of idle workers matter most. Each execution is a new
# process with a new heap layout.
# cmake-format: off
set(scheduler_latency_report_perftest_PARAMETERS
    THREADS_PER_LOCALITY 20
    N_EXECUTIONS 25
)
set(scheduler_throughput_report_perftest_PARAMETERS
    THREADS_PER_LOCALITY 20
    N_EXECUTIONS 25
)
# cmake-format: on

foreach(benchmark ${benchmarks})
  set(sources ${benchmark}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Benchmarks/Modules/Core/Schedulers")

  # add example executable
  add_hpx_executable(
    ${benchmark}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${benchmark}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER ${folder_name}
  )

  add_hpx_performance_test(
    "modules.schedulers" ${benchmark} ${${benchmark}_PARAMETERS}
  )

  if(HPX_WITH_TESTS_RUN_BENCHMARKS)
    add_hpx_performance_report_test(
      "modules.schedulers" ${benchmark}_perftest
      ${${benchmark}_perftest_PARAMETERS}
    )
    set_target_properties(
      ${benchmark}_perftest PROPERTIES FOLDER ${folder_name}/CDashResults
    )
  endif()
endforeach()
