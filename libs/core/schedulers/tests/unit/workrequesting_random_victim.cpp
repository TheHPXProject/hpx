//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/affinity.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/topology.hpp>

#include <barrier>
#include <cstddef>
#include <future>
#include <vector>

int main()
{
    namespace policies = hpx::threads::policies;
    constexpr std::size_t num_workers = 8;
    policies::detail::affinity_data affinity;
    // initialize eagerly so every core's generator is seeded
    policies::local_workrequesting_scheduler<> scheduler(
        {num_workers, affinity}, false);
    std::barrier start(static_cast<std::ptrdiff_t>(num_workers));
    std::vector<std::future<void>> workers;

    // Exercise the scheduler's victim selection concurrently without HPX
    // context switches, so ThreadSanitizer can check its shared state.
    for (std::size_t worker = 0; worker != num_workers; ++worker)
    {
        workers.push_back(std::async(std::launch::async, [&, worker] {
            hpx::threads::mask_type victims;
            hpx::threads::resize(victims, num_workers);
            hpx::threads::reset(victims);
            hpx::threads::set(victims, worker);
            policies::detail::workrequesting_steal_request request(
                worker, nullptr, victims, true, true);

            start.arrive_and_wait();
            for (std::size_t i = 0; i != 10000; ++i)
            {
                auto const victim = scheduler.random_victim(worker, request);
                HPX_TEST_LT(victim, num_workers);
                HPX_TEST_NEQ(victim, worker);
            }
        }));
    }
    for (auto& worker : workers)
    {
        worker.get();
    }
    return hpx::util::report_errors();
}
