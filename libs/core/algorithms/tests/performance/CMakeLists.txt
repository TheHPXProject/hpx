# Copyright (c) 2017 Taeguk Kwon
# Copyright (c) 2017 Google
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(benchmarks
    benchmark_uninitialized_relocate
    benchmark_inplace_merge
    benchmark_is_heap
    benchmark_is_heap_until
    benchmark_merge
    benchmark_merge_sweep
    benchmark_nth_element
    benchmark_nth_element_parallel
    benchmark_partial_sort
    benchmark_partial_sort_parallel
    benchmark_partition
    benchmark_partition_copy
    benchmark_reduce_deterministic
    benchmark_remove
    benchmark_remove_if
    benchmark_reverse
    benchmark_scan_algorithms
    benchmark_unique
    benchmark_unique_copy
    foreach_report
    foreach_scaling
    transform_reduce_scaling
)

set(perftest_reports
    foreach_report
    transform_reduce_scaling
    benchmark_is_heap_until
    benchmark_merge
    benchmark_inplace_merge
    benchmark_is_heap
    benchmark_remove
    benchmark_remove_if
    benchmark_partial_sort
    benchmark_partial_sort_parallel
    benchmark_nth_element
    benchmark_nth_element_parallel
)

set(foreach_report_perftest_PARAMETERS
    THREADS_PER_LOCALITY
    4
    --hpx:ini=hpx.thread_queue.init_threads_count=100
    --vector_size=104857
    --work_delay=1
    --chunk_size=0
    --test_count=200
)

set(transform_reduce_scaling_perftest_PARAMETERS THREADS_PER_LOCALITY 4)

set(benchmark_is_heap_until_perftest_PARAMETERS THREADS_PER_LOCALITY 4)

set(benchmark_merge_perftest_PARAMETERS THREADS_PER_LOCALITY 4)

set(benchmark_inplace_merge_perftest_PARAMETERS THREADS_PER_LOCALITY 4)

set(benchmark_is_heap_perftest_PARAMETERS THREADS_PER_LOCALITY 4)

set(benchmark_remove_perftest_PARAMETERS THREADS_PER_LOCALITY 4)

set(benchmark_remove_if_perftest_PARAMETERS THREADS_PER_LOCALITY 4)

foreach(benchmark ${benchmarks})
  set(sources ${benchmark}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${benchmark}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL ${${benchmark}_FLAGS}
    FOLDER "Benchmarks/Modules/Core/Algorithms"
  )

  add_hpx_performance_test(
    "modules.algorithms" ${benchmark} ${${benchmark}_PARAMETERS}
  )

  if(${benchmark} IN_LIST perftest_reports)
    add_hpx_performance_report_test(
      "modules.algorithms" ${benchmark}_perftest
      ${${benchmark}_perftest_PARAMETERS}
    )
  endif()
endforeach()
