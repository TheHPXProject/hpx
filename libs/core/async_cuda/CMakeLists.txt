# Copyright (c) 2019-2025 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(NOT (HPX_WITH_CUDA OR HPX_WITH_HIP))
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/async_cuda/include
set(async_cuda_headers
    hpx/async_cuda/cuda_event.hpp
    hpx/async_cuda/cuda_executor.hpp
    hpx/async_cuda/cuda_exception.hpp
    hpx/async_cuda/cuda_future.hpp
    hpx/async_cuda/cuda_polling_helper.hpp
    hpx/async_cuda/cublas_executor.hpp
    hpx/async_cuda/custom_blas_api.hpp
    hpx/async_cuda/custom_gpu_api.hpp
    hpx/async_cuda/detail/cuda_debug.hpp
    hpx/async_cuda/detail/cuda_event_callback.hpp
    hpx/async_cuda/get_targets.hpp
    hpx/async_cuda/target.hpp
    hpx/async_cuda/transform_stream.hpp
)

set(async_cuda_macros_headers hpx/async_cuda/custom_blas_api.hpp
                              hpx/async_cuda/custom_gpu_api.hpp
)

# Default location is $HPX_ROOT/libs/async_cuda/include_compatibility
# cmake-format: off
set(async_cuda_compat_headers
    hpx/compute/cuda/get_targets.hpp => hpx/async_cuda/get_targets.hpp
    hpx/compute/cuda/target.hpp => hpx/async_cuda/target.hpp
)
# cmake-format: on

set(async_cuda_sources
    cublas_executor.cpp
    cuda_event_callback.cpp
    cuda_future.cpp
    cuda_target.cpp
    cuda_event.cpp
    cuda_exception.cpp
    get_targets.cpp
)

if(HPX_WITH_HIP AND TARGET roc::hipblas)
  set(async_cuda_extra_deps ${async_cuda_extra_deps} roc::hipblas)
elseif(HPX_WITH_CUDA AND TARGET Cuda::cuda)
  set(async_cuda_extra_deps ${async_cuda_extra_deps} Cuda::cuda
                            ${CUDA_CUBLAS_LIBRARIES}
  )
endif()

include(HPX_AddModule)
add_hpx_module(
  core async_cuda
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  SOURCES ${async_cuda_sources}
  HEADERS ${async_cuda_headers}
  MACRO_HEADERS ${async_cuda_macros_headers}
  COMPAT_HEADERS ${async_cuda_compat_headers}
  MODULE_DEPENDENCIES
    hpx_allocator_support
    hpx_assertion
    hpx_concurrency
    hpx_config
    hpx_debugging
    hpx_errors
    hpx_execution_base
    hpx_functional
    hpx_futures
    hpx_memory
    hpx_runtime_local
    hpx_synchronization
    hpx_threading_base
    hpx_type_support
  DEPENDENCIES ${async_cuda_extra_deps}
  CMAKE_SUBDIRS examples tests
)
