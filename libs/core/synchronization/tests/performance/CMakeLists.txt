# Copyright (c) 2019 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(benchmarks channel_mpmc_throughput channel_mpsc_throughput
               channel_spsc_throughput
)

hpx_option(
  HPX_WITH_ORBIT_ROOT
  BOOL
  "Path to a local checkout of austin-hill/orbit used for optional synchronization comparison benchmarks."
  OFF
  ADVANCED
)

set(channel_mpmc_throughput_PARAMETERS THREADS_PER_LOCALITY 2)
set(channel_mpsc_throughput_PARAMETERS THREADS_PER_LOCALITY 2)
set(channel_spsc_throughputs_PARAMETERS THREADS_PER_LOCALITY 2)

foreach(benchmark ${benchmarks})

  set(sources ${benchmark}.cpp)

  source_group("Source Files" FILES ${sources})

  # add benchmark executable
  add_hpx_executable(
    ${benchmark}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL ${${benchmark}_FLAGS}
    FOLDER "Benchmarks/Modules/Core/Synchronization"
  )

  # add a custom target for this benchmark
  add_hpx_performance_test(
    "modules.synchronization" ${benchmark} ${${benchmark}_PARAMETERS}
  )

endforeach()

if(HPX_WITH_ORBIT_ROOT)
  # Orbit can be found at https://github.com/austin-hill/orbit.
  if(EXISTS "${HPX_WITH_ORBIT_ROOT}/include/orbit/mpmc_queue.h")
    add_hpx_executable(
      channel_mpmc_comparison_test INTERNAL_FLAGS
      SOURCES channel_mpmc_comparison.cpp
      EXCLUDE_FROM_ALL
      FOLDER "Benchmarks/Modules/Core/Synchronization"
    )

    target_include_directories(
      channel_mpmc_comparison_test SYSTEM
      PRIVATE "${HPX_WITH_ORBIT_ROOT}/include"
    )

    add_hpx_performance_test(
      "modules.synchronization" channel_mpmc_comparison THREADS_PER_LOCALITY 1
    )
  else()
    message(
      WARNING
        "HPX_WITH_ORBIT_ROOT is set to '${HPX_WITH_ORBIT_ROOT}', but include/orbit/mpmc_queue.h was not found there. The optional channel_mpmc_comparison benchmark will not be added."
    )
  endif()
endif()
