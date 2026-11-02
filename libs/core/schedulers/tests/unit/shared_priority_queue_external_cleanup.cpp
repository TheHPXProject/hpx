//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <string_view>
#include <thread>
#include <vector>

int hpx_main()
{
    std::atomic<bool> executed{false};
    hpx::threads::thread_id_ref_type id;

    hpx::threads::thread_init_data data(
        hpx::threads::make_thread_function_nullary(
            [&executed]() { executed.store(true, std::memory_order_release); }),
        "test_task");

    hpx::threads::register_thread(data, id);
    HPX_TEST(id);

    // Wait for the task to finish executing and reach terminated state. The
    // worker that runs the task stores the terminated state before it releases
    // its own reference to the task. This test runs on a single worker (see
    // CMakeLists.txt): hpx_main resumes only after that worker has released
    // its reference, so the reference dropped below is the last one.
    while (!executed.load(std::memory_order_acquire) ||
        hpx::threads::get_thread_state(id.noref()).state() !=
            hpx::threads::thread_schedule_state::terminated)
    {
        hpx::this_thread::yield();
    }

    HPX_TEST_EQ(hpx::threads::get_thread_state(id.noref()).state(),
        hpx::threads::thread_schedule_state::terminated);

    // Spawn a standard OS thread (std::thread) and drop the reference inside it.
    std::thread external_thread(
        [id_to_drop = HPX_MOVE(id)]() mutable { id_to_drop.reset(); });

    external_thread.join();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<char*> args(argv, argv + argc);

    bool has_queuing = false;
    for (int i = 1; i < argc; ++i)
    {
        std::string_view const arg(argv[i]);
        if (arg == "--hpx:queuing" || arg.starts_with("--hpx:queuing="))
        {
            has_queuing = true;
            break;
        }
    }

    if (!has_queuing)
    {
        args.push_back(const_cast<char*>("--hpx:queuing=shared-priority"));
    }

    hpx::local::init_params init_args;

    HPX_TEST_EQ(hpx::local::init(hpx_main, static_cast<int>(args.size()),
                    args.data(), init_args),
        0);

    return hpx::util::report_errors();
}
