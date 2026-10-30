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

if(HPX_WITH_DATAPAR)
  list(APPEND benchmarks benchmark_search benchmark_search_n)
endif()

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

# foreach_report, future_overhead_report and stream_report were the only
# benchmarks of the earlier performance report, they keep its 50 executions so
# that their results stay comparable
# cmake-format: off
set(foreach_report_perftest_PARAMETERS
    THREADS_PER_LOCALITY 4
    N_EXECUTIONS 50
    --hpx:ini=hpx.thread_queue.init_threads_count=100
    --hpx:queuing=local-priority-fifo-double
    --vector_size=104857
    --work_delay=1
    --chunk_size=0
    --test_count=200
)
# cmake-format: on

set(transform_reduce_scaling_perftest_PARAMETERS THREADS_PER_LOCALITY 4
                                                 --test_count=100
)

set(benchmark_is_heap_until_perftest_PARAMETERS THREADS_PER_LOCALITY 4
                                                --test_count=100
)

# The default of 2^28 elements takes about a minute per execution, 2^24 elements
# a few seconds
set(benchmark_merge_perftest_PARAMETERS THREADS_PER_LOCALITY 4
                                        --vector_size=16777216 --test_count=10
)

set(benchmark_inplace_merge_perftest_PARAMETERS THREADS_PER_LOCALITY 4
                                                --test_count=100
)

set(benchmark_is_heap_perftest_PARAMETERS THREADS_PER_LOCALITY 4
                                          --test_count=100
)

set(benchmark_remove_perftest_PARAMETERS THREADS_PER_LOCALITY 4
                                         --test_count=100
)

set(benchmark_remove_if_perftest_PARAMETERS THREADS_PER_LOCALITY 4
                                            --test_count=100
)

set(benchmark_partial_sort_perftest_PARAMETERS THREADS_PER_LOCALITY 4)

set(benchmark_partial_sort_parallel_perftest_PARAMETERS THREADS_PER_LOCALITY 4)

set(benchmark_nth_element_perftest_PARAMETERS THREADS_PER_LOCALITY 4)

set(benchmark_nth_element_parallel_perftest_PARAMETERS THREADS_PER_LOCALITY 4)

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
  if(HPX_WITH_TESTS_RUN_BENCHMARKS AND (${benchmark} IN_LIST perftest_reports))
    add_hpx_performance_report_test(
      "modules.algorithms" ${benchmark}_perftest
      ${${benchmark}_perftest_PARAMETERS}
    )
    set_target_properties(
      ${benchmark}_perftest
      PROPERTIES FOLDER "Benchmarks/Modules/Core/Algorithms/CDashResults"
    )
  endif()
endforeach()
