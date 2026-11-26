# Copyright (c) 2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(benchmarks fork_join_executors_benchmark)

# as a test, run only a few short loops
set(fork_join_executors_benchmark_PARAMETERS
    THREADS_PER_LOCALITY 4 ARGS --regions=20 --batches=1 --sizes 0 1000
    --gaps-us 10 --other-work-size=1000
)

foreach(benchmark ${benchmarks})
  set(sources ${benchmark}.cpp)

  source_group("Source Files" FILES ${sources})

  add_hpx_executable(
    ${benchmark}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL ${${benchmark}_FLAGS}
    FOLDER "Benchmarks/Modules/Core/Executors"
  )

  add_hpx_performance_test(
    "modules.executors" ${benchmark} ${${benchmark}_PARAMETERS}
  )
endforeach()
