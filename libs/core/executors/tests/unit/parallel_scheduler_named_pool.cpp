// Copyright (c) 2026 the-ivii
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/assert.hpp>
#include <hpx/executors/async.hpp>
#include <hpx/executors/parallel_scheduler.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;

std::size_t const max_threads = (std::min) (std::size_t(4),
    std::size_t(hpx::threads::hardware_concurrency()));

int hpx_main()
{
    auto& custom = hpx::resource::get_thread_pool("custom");

    // Unknown pool names fail at lookup with hpx::exception, not UB.
    HPX_TEST(!hpx::resource::pool_exists("no-such-pool"));
    HPX_TEST_THROW(
        hpx::resource::get_thread_pool("no-such-pool"), hpx::exception);

    // Lifetime of `custom` is caller responsibility: get_parallel_scheduler
    // stores a non-owning pointer and does not keep the pool alive. Debug
    // builds assert that the pool currently has OS threads; a dangling pool
    // after this call is not diagnosed.
    auto sched = ex::get_parallel_scheduler(custom);

    std::string seen;
    auto snd = ex::schedule(sched) | ex::then([&seen] {
        auto* pool = hpx::this_thread::get_pool();
        HPX_TEST(pool != nullptr);
        seen = pool->get_pool_name();
    });
    ex::sync_wait(std::move(snd));
    HPX_TEST_EQ(seen, std::string("custom"));

    // Concurrent construction of pool-bound schedulers and concurrent
    // schedule() on one shared backend must be safe.
    std::atomic<int> hits{0};
    std::vector<hpx::future<void>> tasks;
    tasks.reserve(16);
    for (int i = 0; i != 8; ++i)
    {
        tasks.push_back(hpx::async([&custom, &hits] {
            auto s = ex::get_parallel_scheduler(custom);
            ex::sync_wait(ex::schedule(s) | ex::then([&hits] {
                auto* pool = hpx::this_thread::get_pool();
                HPX_TEST(pool != nullptr);
                HPX_TEST_EQ(pool->get_pool_name(), std::string("custom"));
                hits.fetch_add(1, std::memory_order_relaxed);
            }));
        }));
    }
    for (int i = 0; i != 8; ++i)
    {
        tasks.push_back(hpx::async([&sched, &hits] {
            ex::sync_wait(ex::schedule(sched) | ex::then([&hits] {
                auto* pool = hpx::this_thread::get_pool();
                HPX_TEST(pool != nullptr);
                HPX_TEST_EQ(pool->get_pool_name(), std::string("custom"));
                hits.fetch_add(1, std::memory_order_relaxed);
            }));
        }));
    }
    hpx::wait_all(tasks);
    HPX_TEST_EQ(hits.load(std::memory_order_relaxed), 16);

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

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
