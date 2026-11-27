//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A high priority thread waiting with termination_detection(stop_token,
// timeout) on a single worker has to let the normal priority threads it waits
// for run. This test is run with one worker thread.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <chrono>

constexpr int num_tasks = 20;

std::atomic<int> counter{0};
std::atomic<bool> completed{false};

void waiter()
{
    for (int i = 0; i < num_tasks; ++i)
    {
        hpx::post([] { ++counter; });
    }

    hpx::stop_source stop_src;
    completed = hpx::local::termination_detection(
        stop_src.get_token(), std::chrono::seconds(10));

    hpx::local::finalize();
}

int hpx_main()
{
    // The waiter has to be the only other thread while it waits: a thread
    // that waits for the waiter or a shutdown in progress counts as work.
    // The waiter shuts the runtime down when it is done.
    hpx::execution::parallel_executor exec(hpx::threads::thread_priority::high);
    hpx::post(exec, &waiter);
    return 0;
}

int main(int argc, char* argv[])
{
    HPX_TEST(hpx::local::start(hpx_main, argc, argv));
    HPX_TEST_EQ(hpx::local::stop(), 0);

    HPX_TEST(completed.load());
    HPX_TEST_EQ(counter.load(), num_tasks);

    return hpx::util::report_errors();
}
