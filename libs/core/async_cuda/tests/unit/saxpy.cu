//  Copyright (c) 2018 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Only the device kernel lives in this file. The device compiler cannot digest
// the HPX execution headers (stdexec does not support nvcc), so the hpx::post
// launching this kernel lives in cuda_future.cpp and receives the kernel
// address through get_saxpy_kernel_address. Taking the address has to happen
// in this translation unit: the host-side symbol is a device stub since
// Clang 11.

#include <hpx/async_cuda/custom_gpu_api.hpp>

__global__ void saxpy(int n, float a, float* x, float* y)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n)
        y[i] = a * x[i] + y[i];
}

void* get_saxpy_kernel_address()
{
    return reinterpret_cast<void*>(&saxpy);
}
