//  Copyright (c) 2026 STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>

void test_latch_reset_zero_wait_deadlock()
{
    hpx::lcos::local::latch l(1);
    HPX_TEST(!l.try_wait());
    HPX_TEST(!l.is_ready());

    l.count_down(1);
    HPX_TEST(l.try_wait());
    HPX_TEST(l.is_ready());
    l.wait();    // initial wait succeeds

    // Reset the latch with n = 0 (Requires: n >= 0)
    l.reset(0);

    // The counter is 0, so try_wait() and is_ready() return true
    HPX_TEST(l.try_wait());
    HPX_TEST(l.is_ready());

    // According to latch specification:
    // "If counter_ is 0, returns immediately. Otherwise, blocks the calling
    //  thread at the synchronization point until counter_ reaches 0."
    //
    // However, reset(0) sets counter_ = 0 but leaves notified_ = false.
    // In wait():
    //   while (counter_.load(std::memory_order_acquire) > 0 || !notified_)
    //       cond_.data_.wait(l, "hpx::latch::wait");
    //
    // Since !notified_ is true, wait() unconditionally deadlocks on cond_.wait().
    // We launch wait() in an asynchronous task and check for completion.
    hpx::future<void> f = hpx::async([&l] { l.wait(); });

    hpx::future_status const status =
        f.wait_for(std::chrono::milliseconds(500));

    // This assertion MUST FAIL on current master because l.wait() deadlocks!
    HPX_TEST_MSG(status == hpx::future_status::ready,
        "DEADLOCK DETECTED: l.wait() blocked indefinitely after reset(0) "
        "despite counter_ == 0 and is_ready() == true");

    if (status != hpx::future_status::ready)
    {
        // Unblock the deadlocked thread so test runner can complete and report failure
        l.abort_all();
        try
        {
            f.get();
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
        catch (...)
        {
        }
    }
}

void test_latch_reset_zero_arrive_and_wait_deadlock()
{
    hpx::lcos::local::latch l(1);
    l.count_down(1);
    l.wait();

    l.reset(0);
    HPX_TEST(l.is_ready());

    hpx::future<void> f = hpx::async([&l] { l.arrive_and_wait(0); });

    hpx::future_status const status =
        f.wait_for(std::chrono::milliseconds(500));

    HPX_TEST_MSG(status == hpx::future_status::ready,
        "DEADLOCK DETECTED: l.arrive_and_wait(0) blocked indefinitely after "
        "reset(0)");

    if (status != hpx::future_status::ready)
    {
        l.abort_all();
        try
        {
            f.get();
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
        catch (...)
        {
        }
    }
}

void test_latch_reset_concurrent_race()
{
    constexpr std::size_t iterations = 10000;

    for (std::size_t i = 0; i < iterations; ++i)
    {
        hpx::lcos::local::latch l(0);
        HPX_TEST(l.is_ready());

        std::atomic<bool> start{false};

        hpx::future<void> f_reset = hpx::async([&l, &start] {
            while (!start.load(std::memory_order_relaxed))
            {
                hpx::this_thread::yield();
            }
            l.reset(1);
        });

        hpx::future<void> f_count_down = hpx::async([&l, &start] {
            while (!start.load(std::memory_order_relaxed))
            {
                hpx::this_thread::yield();
            }
            while (l.try_wait())
            {
                hpx::this_thread::yield();
            }
            l.count_down(1);
        });

        start.store(true, std::memory_order_release);

        f_reset.get();
        f_count_down.get();

        hpx::future<void> f = hpx::async([&l] { l.wait(); });

        hpx::future_status const status =
            f.wait_for(std::chrono::milliseconds(500));

        HPX_TEST_MSG(status == hpx::future_status::ready,
            "DEADLOCK DETECTED: l.wait() blocked indefinitely after concurrent "
            "reset(1) and count_down(1)");

        if (status != hpx::future_status::ready)
        {
            l.abort_all();
            try
            {
                f.get();
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
            catch (...)
            {
            }
            return;
        }

        f.get();

        HPX_TEST(l.is_ready());
        HPX_TEST(l.try_wait());
    }
}

int hpx_main()
{
    test_latch_reset_zero_wait_deadlock();
    test_latch_reset_zero_arrive_and_wait_deadlock();
    test_latch_reset_concurrent_race();

    HPX_TEST_EQ(hpx::local::finalize(), 0);
    return 0;
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
