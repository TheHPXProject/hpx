//  Copyright (c) 2023 Gregor Daiss
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// hpxinspect:noascii

#include <hpx/assert.hpp>
#include <hpx/async_cuda/cuda_event.hpp>
#include <hpx/async_cuda/custom_gpu_api.hpp>

namespace hpx::cuda::experimental {

    cuda_event_pool& cuda_event_pool::get_event_pool()
    {
        static cuda_event_pool event_pool;
        return event_pool;
    }
}    // namespace hpx::cuda::experimental
