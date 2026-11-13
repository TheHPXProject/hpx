//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/thread.hpp>

#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <vector>

void test_numa_worker_selection()
{
    std::vector<std::size_t> const domain_workers{2, 3};

    // Repeated hints rotate evenly over the workers in the requested domain.
    for (std::size_t i = 0; i != 8; ++i)
    {
        HPX_TEST_EQ(hpx::threads::policies::detail::select_numa_worker(
                        domain_workers, i),
            domain_workers[i % domain_workers.size()]);
    }

    // An empty domain falls back to the scheduler's global round-robin path.
    HPX_TEST_EQ(hpx::threads::policies::detail::select_numa_worker(
                    std::vector<std::size_t>{}, 0),
        static_cast<std::size_t>(-1));

    // High-priority lookup uses a prefiltered list containing only workers
    // with a high-priority queue.
    std::vector<std::size_t> const high_priority_workers{0, 1};
    HPX_TEST_EQ(hpx::threads::policies::detail::select_numa_worker(
                    high_priority_workers, 1),
        std::size_t(1));

    // Skip an inactive worker if another eligible worker belongs to the
    // requested domain.
    HPX_TEST_EQ(
        hpx::threads::policies::detail::select_numa_worker(domain_workers, 0,
            [](std::size_t worker) noexcept { return worker != 2; }),
        std::size_t(3));
    HPX_TEST_EQ(
        hpx::threads::policies::detail::select_numa_worker(
            domain_workers, 0, [](std::size_t) noexcept { return false; }),
        static_cast<std::size_t>(-1));
}

int hpx_main()
{
    auto& rp = hpx::resource::get_partitioner();
    auto const& topo = hpx::threads::create_topology();
    std::size_t const num_workers = hpx::get_num_worker_threads();

    // A thread hint of -1 means that no specific worker was requested and
    // must retain the scheduler's round-robin fallback.
    hpx::execution::parallel_executor fallback_exec(
        hpx::threads::thread_priority::bound,
        hpx::threads::thread_stacksize::default_,
        hpx::threads::thread_schedule_hint(
            hpx::threads::thread_schedule_hint_mode::thread, std::int16_t(-1)));
    HPX_TEST(hpx::async(fallback_exec, [] { return true; }).get());

    std::set<std::size_t> domains;
    for (std::size_t worker = 0; worker != num_workers; ++worker)
    {
        domains.insert(topo.get_numa_node_number(rp.get_pu_num(worker)));
    }

    for (std::size_t domain : domains)
    {
        hpx::execution::parallel_executor exec(
            hpx::threads::thread_priority::bound,
            hpx::threads::thread_stacksize::default_,
            hpx::threads::thread_schedule_hint(
                hpx::threads::thread_schedule_hint_mode::numa,
                static_cast<std::int16_t>(domain)));

        std::size_t const actual_domain = hpx::async(exec, [&rp, &topo] {
            std::size_t const worker = hpx::get_worker_thread_num();
            return topo.get_numa_node_number(rp.get_pu_num(worker));
        }).get();
        HPX_TEST_EQ(actual_domain, domain);
    }

    return hpx::local::finalize();
}

void test_scheduler(int argc, char* argv[], char const* scheduler)
{
    std::string queuing = std::string("--hpx:queuing=") + scheduler;
    std::vector<char*> av;
    av.reserve(static_cast<std::size_t>(argc) + 1);
    av.push_back(argv[0]);

    for (int i = 1; i != argc; ++i)
    {
        std::string_view const arg(argv[i]);
        if (arg == "--hpx:queuing")
        {
            if (i + 1 != argc)
            {
                ++i;
            }
            continue;
        }

        if (arg.starts_with("--hpx:queuing="))
        {
            continue;
        }

        av.push_back(argv[i]);
    }

    av.push_back(queuing.data());

    HPX_TEST_EQ(
        hpx::local::init(hpx_main, static_cast<int>(av.size()), av.data()), 0);
}

int main(int argc, char* argv[])
{
    test_numa_worker_selection();

    test_scheduler(argc, argv, "local-workrequesting-fifo");
    test_scheduler(argc, argv, "local-workrequesting-mc");

    return hpx::util::report_errors();
}
