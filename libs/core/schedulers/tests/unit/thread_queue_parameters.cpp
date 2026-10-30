//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/runtime.hpp>
#include <hpx/thread.hpp>

#include <cstdint>
#include <string>

void test_add_count_bounds()
{
    hpx::threads::policies::thread_queue_init_parameters const parameters(
        1000, 1, 1, 7, 3);

    HPX_TEST_EQ(parameters.min_add_new_count_, std::int64_t(3));
    HPX_TEST_EQ(parameters.max_add_new_count_, std::int64_t(3));
}

int test_defaults()
{
    HPX_TEST_EQ(hpx::get_config_entry("hpx.thread_queue.max_delete_count", ""),
        std::to_string(HPX_THREAD_QUEUE_MAX_DELETE_COUNT));

    test_add_count_bounds();
    return hpx::local::finalize();
}

int test_terminated_cleanup()
{
    hpx::latch started(2);
    hpx::latch release(1);
    auto task = [&] {
        started.count_down(1);
        release.wait();
    };

    hpx::future<void> first = hpx::async(task);
    hpx::future<void> second = hpx::async(task);

    started.wait();
    release.count_down(1);
    hpx::wait_all(first, second);

    auto* scheduler = hpx::threads::get_self_id_data()->get_scheduler_base();
    HPX_TEST_EQ(scheduler->get_thread_count(
                    hpx::threads::thread_schedule_state::terminated),
        std::int64_t(0));

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(test_defaults, argc, argv), 0);

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=1", "hpx.thread_queue.min_delete_count=1",
        "hpx.thread_queue.max_delete_count=1",
        "hpx.thread_queue.max_terminated_threads=1"};
    init_args.rp_callback = [](auto& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool(
            "default", hpx::resource::scheduling_policy::shared_priority);
    };

    HPX_TEST_EQ(
        hpx::local::init(test_terminated_cleanup, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
