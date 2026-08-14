//  Copyright (c) 2023 Gregor Daiss
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// hpxinspect:noascii

#include <hpx/algorithm.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_cuda.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

// -------------------------------------------------------------------------
// This example is similar to the unit/cuda_future.cpp example (hence it also uses
// the externally defined cuda_trivial_kernel. See unit/cuda_future.cpp for
// more details regarding this.
//
// This example extends unit/cuda_future.cpp by testing the cuda event polling
// on multiple devices (if available)! If the polling is not working correctly,
// the test will time out as some of the futures are never triggered.

template <typename T>
extern void cuda_trivial_kernel(T, cudaStream_t stream);

extern void launch_saxpy_kernel(
    hpx::cuda::experimental::cuda_executor& cudaexec, unsigned int& blocks,
    unsigned int& threads, void** args);

// -------------------------------------------------------------------------
int hpx_main(hpx::program_options::variables_map&)
{
    // install cuda future polling handler
    hpx::cuda::experimental::enable_user_polling poll("default");

    // Print all targets for debug purposes
    hpx::cuda::experimental::print_local_targets();

    int number_devices = 0;
    hpx::cuda::experimental::check_cuda_error(
        cudaGetDeviceCount(&number_devices));
    HPX_ASSERT(number_devices > 0);

    // Check if the futures complete when using executors on all devices
    std::vector<hpx::shared_future<void>> futs(number_devices);
    for (auto device_id = 0; device_id < number_devices; device_id++)
    {
        hpx::cuda::experimental::cuda_executor exec(
            device_id, hpx::cuda::experimental::event_mode{});
        auto fut = hpx::async(exec, cuda_trivial_kernel<float>,
            static_cast<float>(device_id) + 1);
        futs[device_id] = fut.then([device_id](hpx::future<void>&&) {
            std::cout << "Continuation for kernel future triggered on device "
                         "executor "
                      << device_id << std::endl;
        });
    }
    auto final_fut = hpx::when_all(futs);
    std::cout << "All executor test kernels launched! " << std::endl;
    final_fut.get();
    std::cout << "All executor test kernels finished! " << std::endl;

    // Test to see if HPX correctly picks up the current device in case
    // get_future_with_event is not given a device_id
    for (auto device_id = 0; device_id < number_devices; device_id++)
    {
        hpx::cuda::experimental::check_cuda_error(cudaSetDevice(device_id));
        cudaStream_t device_stream;
        hpx::cuda::experimental::check_cuda_error(
            cudaStreamCreate(&device_stream));
        cuda_trivial_kernel<float>(
            number_devices + device_id + 1, device_stream);
        auto fut = hpx::cuda::experimental::detail::get_future_with_event(
            device_stream);
        fut.get();
        std::cout << "get_future_with_event default ID test finished on device "
                  << device_id << std::endl;
        hpx::cuda::experimental::check_cuda_error(
            cudaStreamDestroy(device_stream));
    }

    return hpx::local::finalize();
}

// -------------------------------------------------------------------------
int main(int argc, char** argv)
{
    std::cout << "[HPX Cuda multi device polling] - Starting...\n" << std::endl;

    hpx::local::init_params init_args;

    auto result = hpx::local::init(hpx_main, argc, argv, init_args);
    return result || hpx::util::report_errors();
}
