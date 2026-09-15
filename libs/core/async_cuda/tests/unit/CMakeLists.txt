# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests cuda_future cuda_multi_device_polling transform_stream)
if(HPX_WITH_GPUBLAS)
  set(benchmarks ${benchmarks} cublas_matmul)
endif()

set(cublas_matmul_PARAMETERS THREADS_PER_LOCALITY 4)
set(cuda_future_PARAMETERS THREADS_PER_LOCALITY 4)
set(cuda_multi_device_polling_PARAMETERS THREADS_PER_LOCALITY 4)
set(transform_stream_PARAMETERS THREADS_PER_LOCALITY 4)

set(cuda_future_CUDA_SOURCE saxpy trivial_demo)
set(cuda_multi_device_polling_CUDA_SOURCE trivial_demo)

set(transform_stream_CUDA_SOURCE transform_stream_kernels)

foreach(test ${tests})
  if(${${test}_CUDA})
    set(sources ${test}.cu)
  else()
    set(sources ${test}.cpp)
  endif()

  if(${test}_CUDA_SOURCE)
    foreach(src ${${test}_CUDA_SOURCE})
      set(sources ${sources} ${src}.cu)
    endforeach()
  endif()

  source_group("Source Files" FILES ${sources})

  # add test executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    DEPENDENCIES ${${test_program}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Core/AsyncCuda"
  )

  add_hpx_unit_test(
    "modules.async_cuda" ${test} ${${test}_PARAMETERS} RUN_SERIAL
  )
endforeach()
