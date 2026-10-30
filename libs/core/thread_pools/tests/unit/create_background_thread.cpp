//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test verifies that hpx::threads::detail::create_background_thread()
// correctly flags the newly created thread as a background thread (see
// thread_data::is_background()/set_is_background()).
//
// It runs under every scheduler and also creates the background thread for a
// worker other than the one running the test. The background thread is
// created suspended, and the shared priority scheduler used to send threads
// meant for another worker down the staged path, which only accepts pending
// threads (see #6190).

#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pools.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

    void test_create_background_thread_sets_is_background(
        std::size_t const num_thread)
    {
        using namespace hpx::threads;

        auto* scheduler =
            hpx::threads::get_self_id_data()->get_scheduler_base();
        HPX_TEST(scheduler != nullptr);

        std::int64_t const initial_count =
            scheduler->get_background_thread_count();

        hpx::threads::detail::scheduling_callbacks callbacks(
            []() -> bool { return false; },    // outer
            []() {},                           // inner
            []() -> bool { return true; }      // background
        );

        std::shared_ptr<bool> running;
        std::int64_t idle_loop_count = 0;

        thread_id_ref_type background_thread;
        hpx::threads::detail::create_background_thread(background_thread,
            *scheduler, num_thread, callbacks, running, idle_loop_count);

        HPX_TEST(background_thread);
        HPX_TEST(running);
        HPX_TEST(*running);

        // the newly created thread must be flagged as a background thread
        auto* thrd_data = get_thread_id_data(background_thread);
        HPX_TEST(thrd_data != nullptr);
        HPX_TEST(thrd_data->is_background());

        // the scheduler's background thread count must have been incremented
        HPX_TEST_EQ(
            scheduler->get_background_thread_count(), initial_count + 1);

        // let the background work item terminate itself as quickly as possible
        // instead of looping indefinitely for the remainder of the test run
        *running = false;

        // forcefully reset the background thread
        hpx::threads::thread_init_data data;
        data.scheduler_base = thrd_data->get_scheduler_base();
        thrd_data->rebind(data);
    }
}    // namespace

int hpx_main()
{
    std::size_t const num_threads =
        hpx::this_thread::get_pool()->get_os_thread_count();
    std::size_t const this_thread = hpx::get_local_worker_thread_num();

    // on the worker running this test
    test_create_background_thread_sets_is_background(this_thread);

    // and on a different one, so that the scheduler has to place it on a
    // queue that does not belong to the calling worker
    if (num_threads > 1)
    {
        test_create_background_thread_sets_is_background(
            (this_thread + 1) % num_threads);
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // clang-format off
    std::vector<std::string> const schedulers = {
        "local",
        "local-priority-fifo",
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        "local-priority-lifo",
#endif
        "static",
        "static-priority",
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        "abp-priority-fifo",
        "abp-priority-lifo",
#endif
        "shared-priority",

#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
        "local-workrequesting-fifo",
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
        "local-workrequesting-lifo",
#endif
        "local-workrequesting-mc",
#endif
    };
    // clang-format on

    for (auto const& scheduler : schedulers)
    {
        std::cout << scheduler << std::endl;

        hpx::local::init_params iparams;
        iparams.cfg = {"--hpx:queuing=!" + scheduler};
        // a scheduler that does not come up would otherwise leave this
        // test passing without having run anything
        HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, iparams), 0);
    }

    return hpx::util::report_errors();
}
