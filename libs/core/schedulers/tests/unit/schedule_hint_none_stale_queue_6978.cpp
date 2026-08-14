//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for:
//   shared_priority_queue_scheduler: possible stale (domain_num, q_index)
//   after select_active_pu() in schedule_thread() `none` path
//   https://github.com/TheHPXProject/hpx/issues/6978

#include <hpx/chrono.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pool_util.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <cstddef>
#include <iostream>
#include <vector>

int hpx_main()
{
    std::size_t const num_threads = hpx::resource::get_num_threads("default");
    if (num_threads < 2)
    {
        std::cerr << "[SKIP] test requires >= 2 threads\n";
        return hpx::local::finalize();
    }

    hpx::threads::thread_pool_base& tp =
        hpx::resource::get_thread_pool("default");

    std::atomic<std::size_t> tasks_completed{0};
    std::vector<hpx::future<void>> futures;

    // Cycle PUs through suspend/resume while flooding hint_mode::none tasks.
    // This races task submission against the sleeping-state window where
    // select_active_pu() redirects thread_num. With the bug, domain_num and
    // q_index are stale after the redirect, so tasks may be pushed to the
    // wrong queue. With the fix they are recomputed and all tasks complete.
    std::size_t thread_num = 0;
    bool going_up = true;
    hpx::chrono::high_resolution_timer const t;

    while (t.elapsed() < 2.0)
    {
        for (std::size_t i = 0; i < num_threads * 10; ++i)
        {
            futures.push_back(
                hpx::async([&tasks_completed]() { ++tasks_completed; }));
        }

        if (going_up)
        {
            if (thread_num < num_threads - 1)
                hpx::threads::suspend_processing_unit(tp, thread_num).get();
            ++thread_num;
            if (thread_num == num_threads)
            {
                going_up = false;
                --thread_num;
            }
        }
        else
        {
            hpx::threads::resume_processing_unit(tp, thread_num).get();
            if (thread_num > 0)
                --thread_num;
            else
                going_up = true;
        }
    }

    hpx::wait_all(futures);

    // Resume any still-suspended PUs before shutdown
    for (std::size_t i = 0; i < thread_num; ++i)
        hpx::threads::resume_processing_unit(tp, i).get();

    HPX_TEST_EQ(tasks_completed.load(), futures.size());

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    if (hpx::threads::hardware_concurrency() < 2)
    {
        std::cerr << "[SKIP] Test requires >= 2 hardware threads\n";
        return hpx::util::report_errors();
    }

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=4"};
    init_args.rp_callback = [](auto& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default",
            hpx::resource::scheduling_policy::shared_priority,
            hpx::threads::policies::scheduler_mode::default_ |
                hpx::threads::policies::scheduler_mode::enable_elasticity);
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);

    return hpx::util::report_errors();
}
