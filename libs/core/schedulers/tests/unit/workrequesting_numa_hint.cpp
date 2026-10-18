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
#include <vector>

void test_numa_worker_selection()
{
    std::vector<std::size_t> const domains{0, 0, 1, 1, 2, 2};

    HPX_TEST_EQ(
        hpx::threads::policies::detail::select_numa_worker(domains, 1, 0),
        std::size_t(2));
    HPX_TEST_EQ(
        hpx::threads::policies::detail::select_numa_worker(domains, 1, 3),
        std::size_t(3));
    HPX_TEST_EQ(
        hpx::threads::policies::detail::select_numa_worker(domains, 0, 4),
        std::size_t(0));
    HPX_TEST_EQ(
        hpx::threads::policies::detail::select_numa_worker(domains, 3, 0),
        static_cast<std::size_t>(-1));
}

int hpx_main()
{
    auto& rp = hpx::resource::get_partitioner();
    auto const& topo = hpx::threads::create_topology();
    std::size_t const num_workers = hpx::get_num_worker_threads();

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

void test_scheduler(char* argv0, char const* scheduler)
{
    std::string const queuing = std::string("--hpx:queuing=") + scheduler;
    std::vector<char*> av = {argv0, const_cast<char*>(queuing.c_str())};

    HPX_TEST_EQ(
        hpx::local::init(hpx_main, static_cast<int>(av.size()), av.data()), 0);
}

int main(int, char* argv[])
{
    test_numa_worker_selection();

    test_scheduler(argv[0], "local-workrequesting-fifo");
    test_scheduler(argv[0], "local-workrequesting-mc");

    return hpx::util::report_errors();
}
