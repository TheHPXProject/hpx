//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The first use of a device by the HIP runtime builds its internal kernels on
// the calling thread, which overflowed the small stack of an HPX task. The
// runtime warms the device up at startup (hpx.hip.warmup), so a task may use
// the device first. Harmless with CUDA.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_cuda.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>

int hpx_main()
{
    // a task (small stack) is the first to use the device
    hpx::async([]() {
        void* p = nullptr;
        hpx::cuda::experimental::check_cuda_error(cudaMalloc(&p, 64));
        hpx::cuda::experimental::check_cuda_error(
            cudaMemsetAsync(p, 0, 64, nullptr));
        hpx::cuda::experimental::check_cuda_error(
            cudaStreamSynchronize(nullptr));
        hpx::cuda::experimental::check_cuda_error(cudaFree(p));
    }).get();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
