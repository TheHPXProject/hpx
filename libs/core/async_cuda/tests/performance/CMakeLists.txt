# Copyright (c) 2020 John Biddiscombe
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(benchmarks synchronize)
if(HPX_WITH_GPUBLAS)
  list(APPEND benchmarks cuda_executor_throughput)
endif()

set(synchronize_CUDA ON)
set(cuda_executor_throughput_PARAMETERS THREADS_PER_LOCALITY 1)

foreach(benchmark ${benchmarks})

  if(${${benchmark}_CUDA})
    set(sources ${benchmark}.cu)
  else()
    set(sources ${benchmark}.cpp)
  endif()

  source_group("Source Files" FILES ${sources})

  # add benchmark executable
  add_hpx_executable(
    ${benchmark}_test INTERNAL_FLAGS
    SOURCES ${sources}
    EXCLUDE_FROM_ALL ${${benchmark}_FLAGS}
    DEPENDENCIES ${${benchmark}_FLAGS}
    FOLDER "Benchmarks/Modules/Core/AsyncCuda"
  )

  # add a custom target for this benchmark
  add_hpx_performance_test(
    "modules.async_cuda" ${benchmark} ${${benchmark}_PARAMETERS}
  )

endforeach()
