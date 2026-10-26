//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/exception.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <vector>

void test_condition_variable_abort_stress()
{
    hpx::spinlock mtx;
    hpx::lcos::local::detail::condition_variable cv;
    std::atomic<bool> stop{false};

    constexpr std::size_t num_threads = 8;
    std::vector<hpx::thread> threads;
    threads.reserve(num_threads);

    for (std::size_t i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([&]() {
            while (!stop.load(std::memory_order_relaxed))
            {
                try
                {
                    std::unique_lock<hpx::spinlock> lk(mtx);
                    if (stop.load(std::memory_order_relaxed))
                    {
                        break;
                    }
                    hpx::chrono::steady_time_point const timeout =
                        hpx::chrono::steady_time_point(
                            hpx::chrono::steady_duration(
                                std::chrono::microseconds(100))
                                .from_now());
                    cv.wait_until(lk, timeout);
                }
                // NOLINTNEXTLINE(bugprone-empty-catch)
                catch (hpx::thread_interrupted const&)
                {
                    // Expected interruption while aborting waiters
                }
                catch (hpx::exception const& e)
                {
                    if (e.get_error() == hpx::error::yield_aborted)
                    {
                        return;
                    }
                    throw;
                }
                hpx::this_thread::yield();
            }
        });
    }

    // Repeatedly call abort_all while ensuring waiters are queued
    for (int i = 0; i < 500; ++i)
    {
        std::unique_lock<hpx::spinlock> lk(mtx);
        while (cv.size(lk) == 0)
        {
            lk.unlock();
            hpx::this_thread::yield();
            lk.lock();
        }
        cv.abort_all(HPX_MOVE(lk));
        hpx::this_thread::yield();
    }

    stop.store(true, std::memory_order_relaxed);

    // Abort once more to wake up any threads still waiting
    {
        std::unique_lock<hpx::spinlock> lk(mtx);
        cv.abort_all(HPX_MOVE(lk));
    }

    for (auto& t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

int hpx_main()
{
    test_condition_variable_abort_stress();
    HPX_TEST_EQ(hpx::local::finalize(), 0);
    return 0;
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
