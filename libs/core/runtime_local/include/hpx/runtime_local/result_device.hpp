//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

namespace hpx::detail {

    // The GPU on which device data in the results of remote calls (for
    // instance a thrust::device_vector) is received, -1 if there is none.
    // Modules with GPU support register the function that answers this
    // (async_cuda does for CUDA and HIP: the GPU of the default target, see
    // hpx::cuda::experimental::get_default_target); without one, the answer
    // is -1.
    HPX_CXX_CORE_EXPORT using result_device_function = int (*)() noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_result_device_function(
        result_device_function f) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT int result_device() noexcept;
}    // namespace hpx::detail
