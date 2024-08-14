# Copyright (c) 2016 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(benchmarks minmax_element_performance)

foreach(benchmark ${benchmarks})
  set(sources ${benchmark}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${benchmark}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${benchmark}_FLAGS}
    DEPENDENCIES iostreams_component partitioned_vector_component
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Benchmarks/Modules/Full/SegmentedAlgorithms"
  )

  add_hpx_performance_test(
    "modules.segmented_algorithms" ${benchmark} ${${benchmark}_PARAMETERS}
  )

  add_hpx_performance_report_test(
    "modules.segmented_algorithms" ${benchmark}_perftest
    ${${benchmark}_perftest_PARAMETERS}
  )
endforeach()
