//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test verifies that hpx::threads::detail::create_background_thread()
// correctly flags the newly created thread as a background thread (see
// thread_data::is_background()/set_is_background()).

#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread_pools/detail/background_thread.hpp>
#include <hpx/thread_pools/detail/scheduling_callbacks.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

void test_create_background_thread_sets_is_background()
{
    using namespace hpx::threads;

    auto* scheduler = hpx::threads::get_self_id_data()->get_scheduler_base();
    HPX_TEST(scheduler != nullptr);

    std::int64_t const initial_count = scheduler->get_background_thread_count();

    hpx::threads::detail::scheduling_callbacks callbacks(
        []() -> bool { return false; },    // outer
        []() {},                           // inner
        []() -> bool { return true; }      // background
    );

    std::shared_ptr<bool> running;
    std::int64_t idle_loop_count = 0;

    thread_id_ref_type const background_thread =
        hpx::threads::detail::create_background_thread(
            *scheduler, 0, callbacks, running, idle_loop_count);

    HPX_TEST(background_thread);
    HPX_TEST(running);
    HPX_TEST(*running);

    // the newly created thread must be flagged as a background thread
    auto* thrd_data = get_thread_id_data(background_thread);
    HPX_TEST(thrd_data != nullptr);
    HPX_TEST(thrd_data->is_background());

    // the scheduler's background thread count must have been incremented
    HPX_TEST_EQ(scheduler->get_background_thread_count(), initial_count + 1);

    // let the background work item terminate itself as quickly as possible
    // instead of looping indefinitely for the remainder of the test run
    *running = false;

    // restore the background thread count (normally done once the
    // background thread notices it should terminate, via
    // call_background_thread/call_and_create_background_thread)
    scheduler->decrement_background_thread_count();
    HPX_TEST_EQ(scheduler->get_background_thread_count(), initial_count);
}

int hpx_main()
{
    test_create_background_thread_sets_is_background();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::local::init(hpx_main, argc, argv);
    return hpx::util::report_errors();
}