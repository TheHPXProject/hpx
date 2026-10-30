//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Work launched with launch::fork from a thread that can't yield to the new
// task (a plain OS thread, or an HPX thread of a different pool) used to be
// created but never scheduled.

#include <hpx/assert.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <thread>

namespace ex = hpx::execution::experimental;

std::size_t const max_threads = (std::min) (std::size_t(4),
    std::size_t(hpx::threads::hardware_concurrency()));

// launch through every fork entry point: sender, post, and async
void fork_all(hpx::threads::thread_pool_base* pool, std::atomic<int>& count)
{
    auto increment = [&count] { ++count; };

    ex::start_detached(ex::then(
        ex::schedule(
            ex::thread_pool_policy_scheduler<hpx::launch::fork_policy>(pool)),
        increment));

    hpx::execution::parallel_policy_executor<hpx::launch::fork_policy> exec(
        pool, hpx::launch::fork);
    hpx::post(exec, increment);
    (void) hpx::async(exec, increment);
}

void wait_for(std::atomic<int> const& count, int expected)
{
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (count != expected && std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::yield();
    }
    HPX_TEST_EQ(count.load(), expected);
}

int hpx_main()
{
    {
        std::atomic<int> count{0};
        std::thread([&] {
            fork_all(&hpx::resource::get_thread_pool("default"), count);
        }).join();
        wait_for(count, 3);
    }

    {
        std::atomic<int> count{0};
        fork_all(&hpx::resource::get_thread_pool("custom"), count);
        wait_for(count, 3);
    }

    return hpx::local::finalize();
}

void init_resource_partitioner_handler(
    hpx::resource::partitioner& rp, hpx::program_options::variables_map const&)
{
    rp.create_thread_pool(
        "custom", hpx::resource::scheduling_policy::local_priority_fifo);

    // give the last PU to the custom pool
    hpx::resource::pu const* last = nullptr;
    for (hpx::resource::numa_domain const& d : rp.numa_domains())
    {
        for (hpx::resource::core const& c : d.cores())
        {
            for (hpx::resource::pu const& p : c.pus())
            {
                last = &p;
            }
        }
    }
    HPX_ASSERT(last != nullptr);
    rp.add_resource(*last, "custom");
}

int main(int argc, char* argv[])
{
    // the test needs at least two PUs: one for "default", one for "custom"
    if (max_threads < 2)
    {
        return hpx::util::report_errors();
    }

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=" + std::to_string(max_threads)};
    init_args.rp_callback = &init_resource_partitioner_handler;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
