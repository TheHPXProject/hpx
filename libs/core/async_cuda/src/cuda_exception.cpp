//  Copyright (c) 2020 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>

// CUDA runtime
#include <hpx/async_cuda/cuda_exception.hpp>
#include <hpx/async_cuda/custom_gpu_api.hpp>

#include <string>

namespace hpx::cuda::experimental {

    // -------------------------------------------------------------------------
    // Error message handler for cuda calls
    void check_cuda_error(cudaError_t const err)
    {
        if (err != cudaSuccess)
        {
            char const* msg = cudaGetErrorString(err);
            auto temp = std::string("cuda function returned error code :") +
                (msg != nullptr ? msg : std::to_string(static_cast<int>(err)));
            throw cuda_exception(temp, err);
        }
    }
}    // namespace hpx::cuda::experimental
