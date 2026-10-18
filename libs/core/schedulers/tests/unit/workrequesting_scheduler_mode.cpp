//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/affinity.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/topology.hpp>

#include <cstddef>

int main()
{
    namespace policies = hpx::threads::policies;

    constexpr std::size_t num_workers = 4;
    policies::detail::affinity_data affinity;
    policies::local_workrequesting_scheduler<> scheduler(
        {num_workers, affinity});

    hpx::threads::mask_type workers;
    hpx::threads::resize(workers, num_workers);
    for (std::size_t worker = 0; worker != num_workers; ++worker)
    {
        hpx::threads::set(workers, worker);
    }

    // The work-requesting scheduler retains required modes without restoring
    // NUMA stealing after the thread manager has disabled it.
    scheduler.set_scheduler_mode(
        policies::scheduler_mode::nothing_special, workers);

    for (std::size_t worker = 0; worker != num_workers; ++worker)
    {
        HPX_TEST(scheduler.has_scheduler_mode(
            policies::scheduler_mode::fast_idle_mode, worker));
        HPX_TEST(!scheduler.has_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, worker));
    }

    scheduler.set_scheduler_mode(policies::scheduler_mode::default_, workers);

    for (std::size_t worker = 0; worker != num_workers; ++worker)
    {
        HPX_TEST(scheduler.has_scheduler_mode(
            policies::scheduler_mode::enable_stealing_numa, worker));
    }

    return hpx::util::report_errors();
}
