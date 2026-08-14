//  Copyright (c) 2017 Thomas Heller
//  Copyright (c) 2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Simple test verifying basic resource_partitioner functionality.

#include <hpx/assert.hpp>
#include <hpx/chrono.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pool_util.hpp>
#include <hpx/thread.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

std::size_t const max_threads = (std::min) (std::size_t(4),
    std::size_t(hpx::threads::hardware_concurrency()));

int hpx_main()
{
    std::size_t const num_os_threads = hpx::get_os_thread_count();
    std::size_t const num_threads = hpx::resource::get_num_threads("worker");

    HPX_TEST_EQ(std::size_t(num_os_threads - 1), num_threads);

    hpx::threads::thread_pool_base& tp =
        hpx::resource::get_thread_pool("worker");

    {
        // Check number of used resources
        for (std::size_t thread_num = 0; thread_num < num_threads - 1;
            ++thread_num)
        {
            hpx::threads::suspend_processing_unit(tp, thread_num).get();
            HPX_TEST_EQ(std::size_t(num_threads - thread_num - 1),
                tp.get_active_os_thread_count());
        }

        for (std::size_t thread_num = 0; thread_num < num_threads - 1;
            ++thread_num)
        {
            hpx::threads::resume_processing_unit(tp, thread_num).get();
            HPX_TEST_EQ(
                std::size_t(thread_num + 2), tp.get_active_os_thread_count());
        }
    }

    {
        // Check suspending and resuming the same thread without waiting for
        // each to finish.
        for (std::size_t thread_num = 0;
            thread_num < hpx::resource::get_num_threads("worker"); ++thread_num)
        {
            std::vector<hpx::future<void>> fs;

            fs.push_back(hpx::threads::suspend_processing_unit(tp, thread_num));
            fs.push_back(hpx::threads::resume_processing_unit(tp, thread_num));

            hpx::wait_all(fs);

            // Suspend is not guaranteed to run before resume, so make sure
            // processing unit is running
            hpx::threads::resume_processing_unit(tp, thread_num).get();

            fs.clear();

            // Launching the same number of tasks as worker threads may deadlock
            // as no thread is available to steal from the current thread.
            for (std::size_t i = 0; i < num_os_threads - 1; ++i)
            {
                fs.push_back(
                    hpx::threads::suspend_processing_unit(tp, thread_num));
            }

            hpx::wait_all(fs);

            fs.clear();

            // Launching the same number of tasks as worker threads may deadlock
            // as no thread is available to steal from the current thread.
            for (std::size_t i = 0; i < num_os_threads - 1; ++i)
            {
                fs.push_back(
                    hpx::threads::resume_processing_unit(tp, thread_num));
            }

            hpx::wait_all(fs);
        }
    }

    {
        // Check random scheduling with reducing resources.
        std::size_t thread_num = 0;
        bool up = true;
        std::vector<hpx::future<void>> fs;
        hpx::chrono::high_resolution_timer t;
        while (t.elapsed() < 2)
        {
            for (std::size_t i = 0;
                i < hpx::resource::get_num_threads("worker") * 10; ++i)
            {
                fs.push_back(hpx::async([]() {}));
            }

            if (up)
            {
                if (thread_num < hpx::resource::get_num_threads("worker"))
                {
                    hpx::threads::suspend_processing_unit(tp, thread_num).get();
                }

                ++thread_num;

                if (thread_num == hpx::resource::get_num_threads("worker"))
                {
                    up = false;
                    --thread_num;
                }
            }
            else
            {
                hpx::threads::resume_processing_unit(tp, thread_num).get();

                if (thread_num > 0)
                {
                    --thread_num;
                }
                else
                {
                    up = true;
                }
            }
        }

        hpx::when_all(std::move(fs)).get();

        // Don't exit with suspended pus
        for (std::size_t thread_num_resume = 0; thread_num_resume < thread_num;
            ++thread_num_resume)
        {
            hpx::threads::resume_processing_unit(tp, thread_num_resume).get();
        }
    }

    return hpx::local::finalize();
}

void test_scheduler(
    int argc, char* argv[], hpx::resource::scheduling_policy scheduler)
{
    hpx::local::init_params init_args;

    init_args.cfg = {"hpx.os_threads=" + std::to_string(max_threads)};
    init_args.rp_callback = [scheduler](auto& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("worker", scheduler,
            hpx::threads::policies::scheduler_mode::default_ |
                hpx::threads::policies::scheduler_mode::enable_elasticity);

        std::size_t const worker_pool_threads =
            rp.get_number_requested_threads() - 1;
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

    std::vector<hpx::resource::scheduling_policy> schedulers = {
        hpx::resource::scheduling_policy::local,
        hpx::resource::scheduling_policy::local_priority_fifo,
        hpx::resource::scheduling_policy::local_priority_fifo_double,
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        hpx::resource::scheduling_policy::local_priority_lifo,
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
