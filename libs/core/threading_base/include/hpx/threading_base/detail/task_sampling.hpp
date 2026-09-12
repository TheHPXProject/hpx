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

    // Per-worker countdown; thread_local, so no atomic. Counter starts
    // at 0, so each worker's first task is always sampled.
    inline thread_local int tl_sample_countdown = 0;

    /// 1-in-N countdown, factored out with Rate as a template parameter
    /// so the unit test can exercise any rate independently of the build
    /// setting. Production goes through should_sample_next() below.
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

    /// Returns true when the next task should be sampled. Called from
    /// thread_data's ctor and rebind_base, exactly once per task creation.
    /// Rate is baked in at build time via HPX_TRACING_SAMPLE_RATE; the
    /// default (1) collapses the body to `return true` at compile time.
    inline bool should_sample_next() noexcept
    {
        return sample_next<HPX_TRACING_SAMPLE_RATE>(tl_sample_countdown);
    }
}    // namespace hpx::threads::detail

#endif    // HPX_HAVE_TRACY
