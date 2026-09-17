//  Copyright (c) 2026 The STE||AR Group
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_TRACY)

namespace hpx::threads::detail {

    // Internal setter. The public entry point is
    // hpx::threads::set_tracing_sample_rate().
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_sample_rate(int rate) noexcept;

    /// 1-in-N countdown with Rate as a template parameter for the unit
    /// test to exercise any rate independently of the build setting.
    /// Production goes through should_sample_next() below.
    template <int Rate>
    inline bool sample_next(int& counter) noexcept
    {
        if constexpr (Rate <= 1)
            return true;
        else
        {
            if (--counter > 0)
                return false;
            counter = Rate;
            return true;
        }
    }

    /// Returns true when the next task should be sampled, based on the
    /// runtime sample_rate.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool should_sample_next() noexcept;
}    // namespace hpx::threads::detail

#endif    // HPX_HAVE_TRACY
