////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)

#include <cstdint>

#if defined(HPX_HAVE_CUDA) && defined(HPX_COMPUTE_CODE)
#include <hpx/hardware/timestamp/cuda.hpp>
#endif

#include <windows.h>

namespace hpx::util::hardware {

    [[nodiscard]] HPX_HOST_DEVICE HPX_FORCEINLINE std::uint64_t
    timestamp() noexcept
    {
#if defined(HPX_HAVE_CUDA) && defined(HPX_COMPUTE_DEVICE_CODE)
        return timestamp_cuda();
#else
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return static_cast<std::uint64_t>(now.QuadPart);
#endif
    }
}    // namespace hpx::util::hardware

#endif
