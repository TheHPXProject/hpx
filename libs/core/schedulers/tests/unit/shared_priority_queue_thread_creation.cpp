//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pools.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

int hpx_main()
{
    constexpr std::size_t num_creators = 8;
    constexpr std::size_t threads_per_creator = 10000;

    std::atomic<std::size_t> completed{0};
    std::vector<hpx::future<void>> creators;
    creators.reserve(num_creators);

    for (std::size_t i = 0; i != num_creators; ++i)
    {
        creators.push_back(hpx::async([&] {
            for (std::size_t j = 0; j != threads_per_creator; ++j)
            {
                hpx::thread([&] { ++completed; }).join();
            }
        }));
    }

    hpx::wait_all(creators);
    for (auto& creator : creators)
    {
        creator.get();
    }
    HPX_TEST_EQ(completed.load(), num_creators * threads_per_creator);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::program_options::options_description desc("Test options");
    desc.add_options()("shared-normal-queue",
        "share one normal-priority queue between two workers");

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc;
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const& vm) {
        if (vm.count("shared-normal-queue") == 0)
        {
            rp.create_thread_pool(
                "default", hpx::resource::scheduling_policy::shared_priority);
            return;
        }

        using scheduler_type =
            hpx::threads::policies::shared_priority_queue_scheduler<>;
        rp.create_thread_pool("default",
            [](hpx::threads::thread_pool_init_parameters init,
                hpx::threads::policies::thread_queue_init_parameters queue_init)
                -> std::unique_ptr<hpx::threads::thread_pool_base> {
                scheduler_type::init_parameter_type scheduler_init(
                    init.num_threads_, {1, 2, 1}, init.affinity_data_,
                    queue_init);
                auto scheduler =
                    std::make_unique<scheduler_type>(scheduler_init);
                return std::make_unique<hpx::threads::detail::
                        scheduled_thread_pool<scheduler_type>>(
                    std::move(scheduler), init);
            });
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
