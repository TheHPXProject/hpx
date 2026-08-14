//  Copyright (c) 2023 Gregor Daiss
//  Copyright (c) 2020 John Biddiscombe
//  Copyright (c) 2020 Teodor Nikolov
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// hpxinspect:noascii

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_cuda/cuda_exception.hpp>
#include <hpx/async_cuda/custom_gpu_api.hpp>
#include <hpx/modules/concurrency.hpp>

#include <cstddef>
#include <deque>

namespace hpx::cuda::experimental {

    // a pool of cudaEvent_t objects.
    // Since allocation of a cuda event passes into the cuda runtime
    // it might be an expensive operation, so we pre-allocate a pool
    // of them at startup.
    HPX_CXX_CORE_EXPORT struct cuda_event_pool
    {
        static constexpr std::size_t initial_events_in_pool = 128;

        HPX_CORE_EXPORT static cuda_event_pool& get_event_pool();

        // on destruction, all objects in stack will be freed
        ~cuda_event_pool()
        {
            HPX_ASSERT_MSG(free_lists_.size() ==
                    static_cast<std::size_t>(max_number_devices_),
                "Number of CUDA event pools does not match the number of "
                "devices!");

            for (int device = 0; device < max_number_devices_; device++)
            {
                check_cuda_error(cudaSetDevice(device));
                cudaEvent_t event;
                bool ok = true;
                while (ok)
                {
                    ok = free_lists_[device].pop(event);
                    if (ok)
                        check_cuda_error(cudaEventDestroy(event));
                }
            }
        }

        inline bool pop(cudaEvent_t& event, int const device = 0)
        {
            HPX_ASSERT_MSG(device >= 0 && device < max_number_devices_,
                "Accessing CUDA event pool with invalid device ID!");
            // pop an event off the pool, if that fails, create a new one
            while (!free_lists_[device].pop(event))
            {
                // Save current device
                int original_device = -1;
                check_cuda_error(cudaGetDevice(&original_device));
                add_event_to_pool(device);
                // reset to original device if required
                if (original_device != device)
                    check_cuda_error(cudaSetDevice(original_device));
            }
            return true;
        }

        inline bool push(cudaEvent_t event, int const device = 0)
        {
            HPX_ASSERT_MSG(device >= 0 && device < max_number_devices_,
                "Accessing CUDA event pool with invalid device ID!");
            return free_lists_[device].push(event);
        }

        // delete copy / move constructors
        cuda_event_pool(cuda_event_pool&&) = delete;
        cuda_event_pool& operator=(cuda_event_pool&&) = delete;
        cuda_event_pool(cuda_event_pool const&) = delete;
        cuda_event_pool& operator=(cuda_event_pool const&) = delete;

    private:
        // Private singleton constructor. Creates a initial_events_in_pool of
        // events on initialization
        cuda_event_pool()
          : max_number_devices_(0)
        {
            check_cuda_error(cudaGetDeviceCount(&max_number_devices_));
            HPX_ASSERT_MSG(max_number_devices_ > 0,
                "CUDA polling enabled and called, yet no CUDA device found!");
            int original_device = 0;
            check_cuda_error(cudaGetDevice(&original_device));
            for (int device = 0; device < max_number_devices_; device++)
            {
                check_cuda_error(cudaSetDevice(device));
                free_lists_.emplace_back(initial_events_in_pool);
                for (std::size_t i = 0; i < initial_events_in_pool; ++i)
                {
                    add_event_to_pool(device);
                }
            }
            // Restore original device -- avoids confusion about HPX switching
            // devices for the current thread if only one device is used.
            check_cuda_error(cudaSetDevice(original_device));
        }

        void add_event_to_pool(int const device)
        {
            check_cuda_error(cudaSetDevice(device));
            cudaEvent_t event;
            // Create an cuda_event to query a CUDA/CUBLAS kernel for completion.
            // Timing is disabled for performance. [1]
            //
            // [1]: CUDA Runtime API, section 5.5 cuda_event Management
            check_cuda_error(
                cudaEventCreateWithFlags(&event, cudaEventDisableTiming));
            free_lists_[device].push(event);
        }
        int max_number_devices_;

        // One pool per GPU - each pool is dynamically sized and can grow if needed
        std::deque<hpx::lockfree::stack<cudaEvent_t>> free_lists_;
    };
}    // namespace hpx::cuda::experimental
