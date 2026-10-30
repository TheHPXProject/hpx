//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test verifies that a thread running with thread_priority::bound keeps
// running on its worker thread while it yields. Bound threads may hold locks
// that are tied to the OS thread (e.g., a std::mutex) across suspensions.
// execution_agent::yield_k suspends the calling thread with pending_boost for
// 16 <= k < 32 (4 <= k < 32 if HPX_SMT_PAUSE is not defined) and for odd
// k >= 32, smaller values of k only spin. The scheduling loop used to
// reschedule such a thread with boosted priority, i.e., in a queue other
// worker threads steal from.

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>

void test_bound_thread_stays_on_worker_thread_while_yielding()
{
    std::size_t const num_threads = hpx::get_os_thread_count();
    HPX_TEST_LTE(std::size_t(2), num_threads);

    // all other worker threads are idle and try to steal work while the bound
    // thread yields
    std::int16_t const target = static_cast<std::int16_t>(num_threads - 1);

    hpx::async([target]() {
        hpx::this_thread::set_affinity(
            target, hpx::threads::thread_priority::bound);

        std::thread::id const os_thread = std::this_thread::get_id();
        HPX_TEST_EQ(
            hpx::get_worker_thread_num(), static_cast<std::size_t>(target));

        // keeps the target worker thread busy while the bound thread is
        // queued, giving the other worker threads a chance to steal it
        hpx::execution::parallel_executor const busy_exec(
            hpx::threads::thread_priority::high,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(target));

        std::size_t migrations = 0;
        for (std::size_t i = 0; i != 2000; ++i)
        {
            hpx::post(busy_exec, []() {
                auto const until = std::chrono::steady_clock::now() +
                    std::chrono::microseconds(50);
                while (std::chrono::steady_clock::now() < until)
                {
                }
            });

            // yield_k(17) suspends this thread with pending_boost
            hpx::execution_base::this_thread::yield_k(17);

            if (std::this_thread::get_id() != os_thread)
            {
                ++migrations;
            }
        }
        HPX_TEST_EQ(migrations, std::size_t(0));
        HPX_TEST_EQ(
            hpx::get_worker_thread_num(), static_cast<std::size_t>(target));
    }).get();
}

int hpx_main()
{
    // the local scheduler has no bound queues, it may move bound threads
    if (hpx::get_config_entry("hpx.scheduler", "") != "local")
    {
        test_bound_thread_stays_on_worker_thread_while_yielding();
    }
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
