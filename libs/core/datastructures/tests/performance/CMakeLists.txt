# Copyright (c) 2019-2022 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests small_vector_benchmark)

set(small_vector_benchmark_perftest_PARAMETERS THREADS_PER_LOCALITY 4
                                               --test_count=100
)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Benchmarks/Modules/Core/Datastructures")

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER ${folder_name}
  )

  add_hpx_performance_test(
    "modules.datastructures" ${test} ${${test}_PARAMETERS}
  )

  if(HPX_WITH_TESTS_RUN_BENCHMARKS)
    add_hpx_performance_report_test(
      "modules.datastructures" ${test}_perftest ${${test}_perftest_PARAMETERS}
    )
    set_target_properties(
      ${test}_cdash_results PROPERTIES FOLDER ${folder_name}/CDashResults
    )
  endif()
endforeach()
