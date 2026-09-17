//  Copyright (c) 2026 The STE||AR Group
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_TRACY)

#include <hpx/threading_base/detail/task_sampling.hpp>
#include <hpx/threading_base/tracing_sample_rate.hpp>

#include <atomic>

namespace hpx::threads::detail {

    namespace {

        // 1-in-N task-sampling rate. Initialized from HPX_TRACING_SAMPLE_RATE
        // (CMake compile-time default); the runtime override applied at
        // startup via hpx::threads::set_tracing_sample_rate() replaces it.
        std::atomic<int> sample_rate{HPX_TRACING_SAMPLE_RATE};

        // Per-worker sampling state. last_rate is the rate value that seeded
        // the current countdown; when it disagrees with the runtime rate, the
        // cycle is abandoned and a fresh one starts at the new rate.
        struct sample_state
        {
            int countdown = 0;
            int last_rate = 0;
        };

        // HPX_NOINLINE so the thread_local address is looked up fresh on each
        // call - a cached address would be wrong once a task migrates workers.
        template <typename State = sample_state>
        HPX_NOINLINE State& sample_state_tls()
        {
            thread_local State state{};
            return state;
        }
    }    // namespace

    void set_sample_rate(int rate) noexcept
    {
        if (rate < 1)
            rate = 1;
        sample_rate.store(rate, std::memory_order_relaxed);
    }

    bool should_sample_next() noexcept
    {
        int const rate = sample_rate.load(std::memory_order_relaxed);
        if (rate <= 1)
            return true;
        sample_state& s = sample_state_tls();
        if (s.last_rate != rate)
        {
            s.last_rate = rate;
            s.countdown = rate;
            return true;
        }
        if (--s.countdown > 0)
            return false;
        s.countdown = rate;
        return true;
    }
}    // namespace hpx::threads::detail

namespace hpx::threads {

    void set_tracing_sample_rate(int rate) noexcept
    {
        detail::set_sample_rate(rate);
    }
}    // namespace hpx::threads

#endif    // HPX_HAVE_TRACY
