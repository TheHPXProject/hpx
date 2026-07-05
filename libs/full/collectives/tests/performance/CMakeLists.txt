# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(subdirs lcos osu)

if(NOT HPX_WITH_NETWORKING)
  return()
endif()

foreach(subdir ${subdirs})
  add_hpx_pseudo_target(tests.performance.modules.collectives.${subdir}_perf)
  add_subdirectory(${subdir})
  add_hpx_pseudo_dependencies(
    tests.performance.modules.collectives
    tests.performance.modules.collectives.${subdir}_perf
  )
endforeach()

set(benchmarks benchmark_collectives)

foreach(benchmark ${benchmarks})
  set(sources ${benchmark}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${benchmark}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL ${${benchmark}_FLAGS}
    FOLDER "Benchmarks/Modules/Full/Collectives"
  )

  add_hpx_performance_test(
    "modules.collectives" ${benchmark} ${${benchmark}_PARAMETERS}
  )
endforeach()
