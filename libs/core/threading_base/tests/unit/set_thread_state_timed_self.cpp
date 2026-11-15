//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A timer a thread sets on itself through the public set_thread_state resumes
// the thread when it expires, also if the thread changes its state (here by
// yielding) between setting the timer and suspending.

#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>

#include <chrono>

using hpx::threads::thread_restart_state;
using hpx::threads::thread_schedule_state;

namespace {

    hpx::chrono::steady_time_point in_10ms()
    {
        return hpx::chrono::steady_time_point(
            std::chrono::steady_clock::now() + std::chrono::milliseconds(10));
    }

    // the thread suspends right after setting the timer
    void timer_then_suspend()
    {
        hpx::threads::set_thread_state(hpx::threads::get_self_id(), in_10ms());

        thread_restart_state const statex =
            hpx::this_thread::suspend(thread_schedule_state::suspended);
        HPX_TEST(statex == thread_restart_state::timeout);
    }

    // the thread yields between setting the timer and suspending
    void timer_yield_then_suspend()
    {
        auto const deadline = in_10ms();
        hpx::threads::set_thread_state(hpx::threads::get_self_id(), deadline);

        // yield: active -> pending -> active, each change increments the tag
        // of the thread's state
        hpx::this_thread::suspend(thread_schedule_state::pending);

        // A timer that fires while its thread is pending doesn't change the
        // thread's state (it is about to run anyway), so the suspension below
        // would never be resumed. That can only have happened if the
        // deadline has passed by now; then this round is skipped, waiting
        // for the timer if it fires while this thread is still active
        // (which is retried until the thread is suspended).
        if (std::chrono::steady_clock::now() >= deadline.value())
        {
            hpx::this_thread::suspend(hpx::chrono::steady_time_point(
                deadline.value() + std::chrono::milliseconds(10)));
            return;
        }

        thread_restart_state const statex =
            hpx::this_thread::suspend(thread_schedule_state::suspended);
        HPX_TEST(statex == thread_restart_state::timeout);
    }

    // suspend with a timeout
    void suspend_until()
    {
        thread_restart_state const statex =
            hpx::this_thread::suspend(in_10ms());
        HPX_TEST(statex == thread_restart_state::timeout);
    }
}    // namespace

int hpx_main()
{
    for (int i = 0; i != 10; ++i)
    {
        timer_then_suspend();
        timer_yield_then_suspend();
        suspend_until();
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
