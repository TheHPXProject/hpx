//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// All schedulers have to report the length of all of their queues when asked
// for thread number -1, as threadmanager::get_queue_length does (used by the
// /threadqueue/length performance counter). The shared priority scheduler used
// to throw instead.
//
// The test runs on a single worker: while hpx_main runs, the tasks it posts
// stay queued, so the queue length has to grow by their number.

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>

constexpr std::ptrdiff_t num_tasks = 10;

int hpx_main()
{
    auto& pool = hpx::resource::get_thread_pool("default");
    auto const all_threads = static_cast<std::size_t>(-1);

    std::int64_t const before = pool.get_queue_length(all_threads, false);

    hpx::latch done(num_tasks + 1);
    for (std::ptrdiff_t i = 0; i != num_tasks; ++i)
    {
        hpx::post([&done] { done.count_down(1); });
    }

    HPX_TEST_EQ(pool.get_queue_length(all_threads, false), before + num_tasks);
    HPX_TEST_EQ(pool.get_queue_length(all_threads, false),
        pool.get_queue_length(0, false));

    done.arrive_and_wait();
    return hpx::local::finalize();
}

void test_scheduler(
    int argc, char* argv[], hpx::resource::scheduling_policy policy)
{
    hpx::local::init_params init_args;
    init_args.rp_callback = [policy](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default", policy);
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
}

int main(int argc, char* argv[])
{
    using hpx::resource::scheduling_policy;

    test_scheduler(argc, argv, scheduling_policy::local);
    test_scheduler(argc, argv, scheduling_policy::local_priority_fifo);
    test_scheduler(argc, argv, scheduling_policy::static_);
    test_scheduler(argc, argv, scheduling_policy::static_priority);
    test_scheduler(argc, argv, scheduling_policy::shared_priority);
#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
    test_scheduler(argc, argv, scheduling_policy::local_workrequesting_fifo);
#endif

    return hpx::util::report_errors();
}
