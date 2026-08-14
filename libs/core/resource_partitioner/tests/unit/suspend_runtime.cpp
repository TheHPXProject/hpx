//  Copyright (c) 2017 Mikael Simberg
//  Copyright (c) 2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Simple test verifying basic resource_partitioner functionality.

#include <hpx/chrono.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/runtime.hpp>
#include <hpx/thread.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

void test_scheduler(
    int argc, char* argv[], hpx::resource::scheduling_policy scheduler)
{
    hpx::local::init_params init_args;

    init_args.cfg = {"hpx.os_threads=" +
        std::to_string(((std::min) (std::size_t(4),
            std::size_t(hpx::threads::hardware_concurrency()))))};
    init_args.rp_callback = [scheduler](auto& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default", scheduler);
    };

    hpx::local::start(nullptr, argc, argv, init_args);
    hpx::local::suspend();

    hpx::chrono::high_resolution_timer t;

    while (t.elapsed() < 2)
    {
        hpx::local::resume();

        hpx::post([]() {
            for (std::size_t i = 0; i < 10000; ++i)
            {
                hpx::post([]() {});
            }
        });

        hpx::local::suspend();
    }

    hpx::local::resume();
    hpx::post([]() { hpx::local::finalize(); });
    HPX_TEST_EQ(hpx::local::stop(), 0);
}

int main(int argc, char* argv[])
{
    std::vector<hpx::resource::scheduling_policy> schedulers = {
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
