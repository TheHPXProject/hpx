//  Copyright (c) 2020 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_HIP)

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-copy"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

#include <hip/hip_runtime.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#ifdef _WIN32
#define CUDART_CB __stdcall
#else
#define CUDART_CB
#endif

#define cudaDeviceProp hipDeviceProp_t
#define cudaDeviceSynchronize hipDeviceSynchronize
#define cudaError_t hipError_t
#define cudaErrorMemoryAllocation hipErrorOutOfMemory
#define cudaErrorNotReady hipErrorNotReady
#define cudaEvent_t hipEvent_t
#define cudaEventCreateWithFlags hipEventCreateWithFlags
#define cudaEventDestroy hipEventDestroy
#define cudaEventDisableTiming hipEventDisableTiming
#define cudaEventQuery hipEventQuery
#define cudaEventRecord hipEventRecord
#define cudaFree hipFree
#define cudaFreeHost hipHostFree
#define cudaGetDevice hipGetDevice
#define cudaGetDeviceCount hipGetDeviceCount
#define cudaGetDeviceProperties hipGetDeviceProperties
#define cudaGetErrorString hipGetErrorString
#define cudaGetLastError hipGetLastError
#define cudaGetParameterBuffer hipGetParameterBuffer
#define cudaLaunchDevice hipLaunchDevice
#define cudaLaunchKernel hipLaunchKernel
#define cudaMalloc hipMalloc
#define cudaMallocHost hipHostMalloc
#define cudaMemcpy hipMemcpy
#define cudaMemcpyAsync hipMemcpyAsync
#define cudaMemcpyDeviceToDevice hipMemcpyDeviceToDevice
#define cudaMemcpyDeviceToHost hipMemcpyDeviceToHost
#define cudaMemcpyHostToDevice hipMemcpyHostToDevice
#define cudaMemGetInfo hipMemGetInfo
#define cudaMemoryTypeHost hipMemoryTypeHost
#define cudaMemsetAsync hipMemsetAsync
#define cudaPointerAttributes hipPointerAttribute_t
#define cudaPointerGetAttributes hipPointerGetAttributes
#define cudaSetDevice hipSetDevice
#define cudaStream_t hipStream_t
#define cudaStreamAddCallback hipStreamAddCallback
#define cudaStreamCreate hipStreamCreate
#define cudaStreamCreateWithFlags hipStreamCreateWithFlags
#define cudaStreamDestroy hipStreamDestroy
#define cudaStreamNonBlocking hipStreamNonBlocking
#define cudaStreamPerThread hipStreamPerThread
#define cudaStreamQuery hipStreamQuery
#define cudaStreamSynchronize hipStreamSynchronize
#define cudaSuccess hipSuccess

#elif defined(HPX_HAVE_CUDA)

#include <cuda_runtime.h>

#endif
