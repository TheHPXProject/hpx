//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Device kernels for the transform_stream test. The test logic lives in
// transform_stream.cpp and is compiled by the host compiler; the device
// compiler cannot digest the HPX execution headers (stdexec does not support
// nvcc), so only the kernels and their launch wrappers live here.

#include <hpx/async_cuda/custom_gpu_api.hpp>

__global__ void dummy_kernel() {}

__global__ void increment_kernel(int* p)
{
    ++(*p);
}

void launch_dummy_kernel(cudaStream_t stream)
{
    dummy_kernel<<<1, 1, 0, stream>>>();
}

void launch_increment_kernel(int* p, cudaStream_t stream)
{
    increment_kernel<<<1, 1, 0, stream>>>(p);
}
