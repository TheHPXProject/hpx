// Copyright (c) 2026 the-ivii
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The purpose of this example is to show the P2079 parallel_scheduler entry
// points: schedule work with get_parallel_scheduler() (default pool) and with
// get_parallel_scheduler(pool) (a named resource-partitioner pool).

#include <hpx/assert.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <utility>

std::size_t const max_threads = (std::min) (std::size_t(4),
    std::size_t(hpx::threads::hardware_concurrency()));

int hpx_main()
{
    namespace ex = hpx::execution::experimental;

    //[get_parallel_scheduler_default
    auto snd = ex::schedule(ex::get_parallel_scheduler()) |
        ex::then([] { return 42; });
    auto [val] = ex::sync_wait(std::move(snd)).value();
    //get_parallel_scheduler_default]

    std::cout << "parallel_scheduler ran, got " << val << "\n";

    //[get_parallel_scheduler_named_pool
    auto named = ex::schedule(ex::get_parallel_scheduler(
                     hpx::resource::get_thread_pool("custom"))) |
        ex::then([] {
            // runs on the "custom" pool
        });
    ex::sync_wait(std::move(named));
    //get_parallel_scheduler_named_pool]

    std::cout << "named-pool parallel_scheduler ran on "
              << hpx::resource::get_thread_pool("custom").get_pool_name()
              << "\n";

    return hpx::local::finalize();
}

void init_resource_partitioner_handler(
    hpx::resource::partitioner& rp, hpx::program_options::variables_map const&)
{
    rp.create_thread_pool(
        "custom", hpx::resource::scheduling_policy::local_priority_fifo);

    bool skipped_first = false;
    for (hpx::resource::numa_domain const& d : rp.numa_domains())
    {
        for (hpx::resource::core const& c : d.cores())
        {
            for (hpx::resource::pu const& p : c.pus())
            {
                if (!skipped_first)
                {
                    skipped_first = true;
                    continue;
                }

                rp.add_resource(p, "custom");
                return;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    HPX_ASSERT(max_threads >= 2);

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=" + std::to_string(max_threads)};
    init_args.rp_callback = &init_resource_partitioner_handler;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
