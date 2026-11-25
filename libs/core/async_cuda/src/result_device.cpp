//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/async_cuda/target.hpp>
#include <hpx/modules/runtime_local.hpp>

namespace hpx::cuda::experimental::detail {

    // answers hpx::detail::result_device() for CUDA and HIP: results of
    // remote calls are received on the GPU of the default target
    namespace {

        int default_target_device() noexcept
        {
            try
            {
                return get_default_target().native_handle().get_device();
            }
            catch (...)
            {
                // the default target couldn't be created (no usable GPU)
                return -1;
            }
        }

        struct register_result_device
        {
            register_result_device() noexcept
            {
                hpx::detail::set_result_device_function(&default_target_device);
            }
        };

        register_result_device const registration;
    }    // namespace
}    // namespace hpx::cuda::experimental::detail
