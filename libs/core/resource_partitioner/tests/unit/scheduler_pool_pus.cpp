//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The affinity data is indexed by the global thread number, while schedulers
// know their workers by the number inside of their pool. This test verifies
// that the schedulers look up the PUs of their own workers in any pool but the
// first one.

#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <mutex>
#include <vector>

namespace policies = hpx::threads::policies;

using local_scheduler = policies::local_queue_scheduler<>;
using priority_scheduler = policies::local_priority_queue_scheduler<std::mutex,
    policies::lockfree_fifo>;

// These types are never instantiated, they give access to protected members.
struct local_scheduler_access : local_scheduler
{
    static auto steals_in_numa_domain() noexcept
    {
        return &local_scheduler_access::steals_in_numa_domain_;
    }
};

struct priority_scheduler_access : priority_scheduler
{
    static auto thread_numa_domain() noexcept
    {
        return &priority_scheduler_access::thread_numa_domain_;
    }
};

bool has_priority_pool = false;

// Every worker of the local scheduler marks its PU during startup, and the
// workers of a pool start concurrently.
void test_local_scheduler()
{
    auto const* sched = dynamic_cast<local_scheduler const*>(
        hpx::resource::get_thread_pool("local_pool").get_scheduler());
    HPX_TEST(sched != nullptr);

    auto const pool_mask =
        hpx::resource::get_partitioner().get_pool_pus_mask("local_pool");
    auto const& steals =
        sched->*local_scheduler_access::steals_in_numa_domain();

    auto const& topo = hpx::threads::create_topology();
    for (std::size_t pu = 0; pu != hpx::threads::mask_size(pool_mask); ++pu)
    {
        // the mark is set only if the topology knows about the PU
        if (hpx::threads::test(pool_mask, pu) &&
            hpx::threads::any(topo.get_thread_affinity_mask(pu)) &&
            hpx::threads::any(topo.get_numa_node_affinity_mask(pu)))
        {
            HPX_TEST(hpx::threads::test(steals, pu));
        }
    }
}

// This shows a wrong lookup only if the PUs of the pool are not in the same
// NUMA domains as the PUs at the start of the affinity data.
void test_priority_scheduler()
{
    auto const& pool = hpx::resource::get_thread_pool("priority_pool");
    auto const* sched =
        dynamic_cast<priority_scheduler const*>(pool.get_scheduler());
    HPX_TEST(sched != nullptr);

    auto const pool_mask =
        hpx::resource::get_partitioner().get_pool_pus_mask("priority_pool");

    auto const& topo = hpx::threads::create_topology();
    std::map<std::size_t, std::size_t> expected;
    for (std::size_t pu = 0; pu != hpx::threads::mask_size(pool_mask); ++pu)
    {
        if (hpx::threads::test(pool_mask, pu))
        {
            ++expected[topo.get_numa_node_number(pu)];
        }
    }

    std::map<std::size_t, std::size_t> actual;
    std::vector<std::size_t> const& domains =
        sched->*priority_scheduler_access::thread_numa_domain();
    for (std::size_t const domain : domains)
    {
        ++actual[domain];
    }

    HPX_TEST(expected == actual);
}

int hpx_main()
{
    test_local_scheduler();
    if (has_priority_pool)
    {
        test_priority_scheduler();
    }
    return hpx::local::finalize();
}

void init_resource_partitioner(
    hpx::resource::partitioner& rp, hpx::program_options::variables_map const&)
{
    // the partitioner counts the use of a PU in the object it handed out, so
    // these must not be copies
    std::vector<hpx::resource::pu const*> pus;
    for (auto const& domain : rp.numa_domains())
    {
        for (auto const& core : domain.cores())
        {
            for (auto const& pu : core.pus())
            {
                pus.push_back(&pu);
            }
        }
    }

    HPX_TEST(pus.size() >= 2);

    // the first PU stays in the default pool, the others are split evenly
    std::size_t const num_local_pus =
        (std::max) (pus.size() / 2, std::size_t(1));

    rp.create_thread_pool(
        "local_pool", hpx::resource::scheduling_policy::local);
    for (std::size_t i = 1; i != num_local_pus + 1; ++i)
    {
        rp.add_resource(*pus[i], "local_pool");
    }

    has_priority_pool = pus.size() > num_local_pus + 1;
    if (has_priority_pool)
    {
        rp.create_thread_pool("priority_pool",
            hpx::resource::scheduling_policy::local_priority_fifo);
        for (std::size_t i = num_local_pus + 1; i != pus.size(); ++i)
        {
            rp.add_resource(*pus[i], "priority_pool");
        }
    }
}

int main(int argc, char* argv[])
{
    hpx::local::init_params init_args;
    init_args.rp_callback = &init_resource_partitioner;

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
