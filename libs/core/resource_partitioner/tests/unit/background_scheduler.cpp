//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

inline constexpr char const* background_pool_name = "background-thread-pool";

std::atomic<bool> background_work_called(false);

bool background_work(std::size_t)
{
    auto* pool = hpx::this_thread::get_pool();
    std::string name = pool->get_pool_name();

    background_work_called = true;
    HPX_TEST_EQ(name, std::string(background_pool_name));

    return true;
}

int hpx_main()
{
    // regression test for #7742: background_scheduler::set_scheduler_mode
    // must add only the background-work flags, not every bit
    {
        using hpx::threads::policies::scheduler_mode;
        hpx::threads::thread_pool_base& pool =
            hpx::resource::get_thread_pool(background_pool_name);
        auto const mode = pool.get_scheduler()->get_scheduler_mode(0);
        HPX_TEST((mode & scheduler_mode::do_background_work) != 0);
        HPX_TEST((mode & scheduler_mode::do_background_work_only) != 0);
        HPX_TEST((mode & scheduler_mode::enable_stealing) == 0);
        HPX_TEST((mode & scheduler_mode::enable_stealing_numa) == 0);
        HPX_TEST((mode & scheduler_mode::fast_idle_mode) == 0);
        HPX_TEST((mode & scheduler_mode::enable_elasticity) == 0);
    }

    hpx::this_thread::suspend(std::chrono::seconds(1));
    return hpx::local::finalize();
}

void init_resource_partitioner_handler(hpx::resource::partitioner& rp,
    hpx::program_options::variables_map const& /*vm*/)
{
    rp.create_thread_pool(background_pool_name,
        hpx::resource::scheduling_policy::static_,
        hpx::threads::policies::scheduler_mode::do_background_work_only,
        background_work);

    hpx::resource::numa_domain const& d = rp.numa_domains()[0];
    hpx::resource::core const& c = d.cores()[0];
    hpx::resource::pu const& p = c.pus()[0];

    rp.add_resource(p, background_pool_name);
}

int main(int argc, char* argv[])
{
    std::vector<std::string> cfg = {"hpx.force_min_os_threads!=2"};

    // Set the callback to init the thread_pools
    hpx::local::init_params init_args;
    init_args.cfg = std::move(cfg);
    init_args.rp_callback = &init_resource_partitioner_handler;

    hpx::local::init(hpx_main, argc, argv, init_args);

    HPX_TEST(background_work_called.load());
    return hpx::util::report_errors();
}
