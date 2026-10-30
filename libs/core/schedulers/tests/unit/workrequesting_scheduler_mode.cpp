//  Copyright (c) 2026 Pratyksh Gupta
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

    // The thread manager replaces the constructor's mode with the pool mode.
    scheduler.set_scheduler_mode(policies::scheduler_mode::default_, workers);

    for (std::size_t worker = 0; worker != num_workers; ++worker)
    {
        HPX_TEST(scheduler.has_scheduler_mode(
            policies::scheduler_mode::fast_idle_mode, worker));
    }

    return hpx::util::report_errors();
}
