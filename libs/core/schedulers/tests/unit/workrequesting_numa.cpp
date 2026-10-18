//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/affinity.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <vector>

int main()
{
    namespace policies = hpx::threads::policies;

    std::vector<std::size_t> const worker_domains = {0, 0, 1, 1, 2, 2};
    constexpr std::size_t num_workers = 6;

    auto nearest = policies::detail::make_workrequesting_victim_masks(
        0, worker_domains, {1});

    // Same-domain workers are the only candidates in the first phase.
    for (std::size_t worker = 0; worker != num_workers; ++worker)
    {
        HPX_TEST_EQ(hpx::threads::test(nearest.local_, worker), worker != 1);
    }

    // The remote phase contains only workers from the selected nearest domain.
    for (std::size_t worker = 0; worker != num_workers; ++worker)
    {
        bool const expected_excluded = worker < 2 || worker >= 4;
        HPX_TEST_EQ(
            hpx::threads::test(nearest.remote_, worker), expected_excluded);
    }

    auto strict = policies::detail::make_workrequesting_victim_masks(
        0, worker_domains, {});
    HPX_TEST_EQ(hpx::threads::count(strict.remote_), num_workers);

    auto unrestricted = policies::detail::make_workrequesting_victim_masks(
        0, worker_domains, {1, 2});
    HPX_TEST_EQ(hpx::threads::count(unrestricted.remote_), std::size_t(2));

    // Victim selection is constrained by each phase's precomputed mask.
    policies::detail::affinity_data affinity;
    policies::local_workrequesting_scheduler<> scheduler(
        {num_workers, affinity}, false);

    policies::detail::workrequesting_steal_request local_request(
        0, nullptr, nearest.local_, true, true);
    for (int i = 0; i != 100; ++i)
    {
        HPX_TEST_EQ(scheduler.random_victim(0, local_request), std::size_t(1));
    }

    policies::detail::workrequesting_steal_request remote_request(
        0, nullptr, nearest.remote_, true, true, true);
    for (int i = 0; i != 100; ++i)
    {
        std::size_t const victim = scheduler.random_victim(0, remote_request);
        HPX_TEST(victim == 2 || victim == 3);
    }

    return hpx::util::report_errors();
}
