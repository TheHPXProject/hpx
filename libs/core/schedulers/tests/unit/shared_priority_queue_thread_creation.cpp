//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <cstddef>
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
    hpx::local::init_params init_args;
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool(
            "default", hpx::resource::scheduling_policy::shared_priority);
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
