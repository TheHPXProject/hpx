//  Copyright (c) 2017 Mikael Simberg
//  Copyright (c) 2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Simple test verifying basic resource_partitioner functionality.

#include <hpx/assert.hpp>
#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pool_util.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/semaphore.hpp>
#include <hpx/thread.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

std::size_t const max_threads = (std::min) (static_cast<std::size_t>(4),
    static_cast<std::size_t>(hpx::threads::hardware_concurrency()));

int hpx_main()
{
    bool exception_thrown = false;

    try
    {
        // Use .get() to throw exception
        hpx::threads::suspend_pool(*hpx::this_thread::get_pool()).get();
        HPX_TEST_MSG(false, "Suspending should not be allowed on own pool");
    }
    catch (hpx::exception const&)
    {
        exception_thrown = true;
    }

    HPX_TEST(exception_thrown);

    hpx::threads::thread_pool_base& worker_pool =
        hpx::resource::get_thread_pool("worker");
    hpx::execution::parallel_executor exec(
        &hpx::resource::get_thread_pool("worker"));
    std::size_t const worker_pool_threads =
        hpx::resource::get_num_threads("worker");

    hpx::threads::thread_schedule_hint hint;
    hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);

    auto worker_exec = hpx::execution::experimental::with_hint(exec, hint);

    {
        // Suspend and resume pool with future
        hpx::chrono::high_resolution_timer const t;

        while (t.elapsed() < 1)
        {
            std::vector<hpx::future<void>> fs;

            for (std::size_t i = 0; i < worker_pool_threads * 10000; ++i)
            {
                fs.push_back(hpx::async(worker_exec, []() {}));
            }

            hpx::threads::suspend_pool(worker_pool).get();

            // All work should be done when pool has been suspended
            HPX_TEST(hpx::when_all(std::move(fs)).is_ready());

            hpx::threads::resume_pool(worker_pool).get();
        }
    }

    {
        // Suspend and resume pool with callback
        hpx::counting_semaphore_var<> sem;
        hpx::chrono::high_resolution_timer const t;

        while (t.elapsed() < 1)
        {
            std::vector<hpx::future<void>> fs;

            for (std::size_t i = 0; i < worker_pool_threads * 10000; ++i)
            {
                fs.push_back(hpx::async(worker_exec, []() {}));
            }

            hpx::threads::suspend_pool_cb(
                worker_pool, [&sem]() { sem.signal(); });

            sem.wait(1);

            // All work should be done when pool has been suspended
            HPX_TEST(hpx::when_all(std::move(fs)).is_ready());

            hpx::threads::resume_pool_cb(
                worker_pool, [&sem]() { sem.signal(); });

            sem.wait(1);
        }
    }

    {
        // Suspend pool with some threads already suspended
        hpx::chrono::high_resolution_timer const t;

        while (t.elapsed() < 1)
        {
            for (std::size_t thread_num = 0;
                thread_num < worker_pool_threads - 1; ++thread_num)
            {
                hpx::threads::suspend_processing_unit(worker_pool, thread_num);
            }

            std::vector<hpx::future<void>> fs;

            for (std::size_t i = 0;
                i < hpx::resource::get_num_threads("default") * 10000; ++i)
            {
                fs.push_back(hpx::async(worker_exec, []() {}));
            }

            hpx::threads::suspend_pool(worker_pool).get();

            // All work should be done when pool has been suspended
            HPX_TEST(hpx::when_all(std::move(fs)).is_ready());

            hpx::threads::resume_pool(worker_pool).get();
        }
    }

    return hpx::local::finalize();
}

void test_scheduler(
    int argc, char* argv[], hpx::resource::scheduling_policy scheduler)
{
    hpx::local::init_params init_args;

    init_args.cfg = {"hpx.os_threads=" + std::to_string(max_threads)};
    init_args.rp_callback = [scheduler](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("worker", scheduler,
            hpx::threads::policies::scheduler_mode::default_ |
                hpx::threads::policies::scheduler_mode::enable_elasticity);

        std::size_t const worker_pool_threads =
            rp.get_number_requested_threads() - 1;
        HPX_ASSERT(worker_pool_threads >= 1);
        std::size_t worker_pool_threads_added = 0;

        for (hpx::resource::numa_domain const& d : rp.numa_domains())
        {
            for (hpx::resource::core const& c : d.cores())
            {
                for (hpx::resource::pu const& p : c.pus())
                {
                    if (worker_pool_threads_added < worker_pool_threads)
                    {
                        rp.add_resource(p, "worker");
                        ++worker_pool_threads_added;
                    }
                }
            }
        }
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
}

int main(int argc, char* argv[])
{
    HPX_ASSERT(max_threads >= 2);

    std::vector<hpx::resource::scheduling_policy> const schedulers = {
        hpx::resource::scheduling_policy::local,
        hpx::resource::scheduling_policy::local_priority_fifo,
        hpx::resource::scheduling_policy::local_priority_fifo_double,
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        hpx::resource::scheduling_policy::local_priority_lifo,
#endif
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        hpx::resource::scheduling_policy::abp_priority_fifo,
        hpx::resource::scheduling_policy::abp_priority_lifo,
#endif
        hpx::resource::scheduling_policy::static_,
        hpx::resource::scheduling_policy::static_priority,
        hpx::resource::scheduling_policy::shared_priority,

#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
        hpx::resource::scheduling_policy::local_workrequesting_fifo,
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        hpx::resource::scheduling_policy::local_workrequesting_lifo,
#endif
        hpx::resource::scheduling_policy::local_workrequesting_mc,
#endif
    };

    for (auto const scheduler : schedulers)
    {
        test_scheduler(argc, argv, scheduler);
    }

    return hpx::util::report_errors();
}
