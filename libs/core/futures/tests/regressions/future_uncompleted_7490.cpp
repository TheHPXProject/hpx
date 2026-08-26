//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <string>

void test_future_uncompleted_error()
{
    hpx::promise<void> p;
    hpx::future<void> f = p.get_future();

    hpx::threads::thread_id_type waiter_id;
    std::atomic<bool> waiter_started{false};

    // Spawn a thread that blocks on future::get() without the promise ever
    // being satisfied.
    hpx::thread waiter([&]() {
        waiter_id = hpx::threads::get_self_id();
        waiter_started.store(true, std::memory_order_release);

        bool caught = false;
        try
        {
            f.get();
        }
        catch (hpx::exception const& e)
        {
            caught = true;
            HPX_TEST_EQ(e.get_error(), hpx::error::future_uncompleted);
            HPX_TEST(
                std::string(e.what()).find(
                    "this future was resumed while its state was 'empty'") !=
                std::string::npos);

            // Must be distinct from no_state.
            HPX_TEST_NEQ(e.get_error(), hpx::error::no_state);
        }
        HPX_TEST(caught);
    });

    // Wait until the waiter thread has actually registered itself and is about
    // to suspend inside future_data_base::wait().
    while (!waiter_started.load(std::memory_order_acquire))
    {
        hpx::this_thread::yield();
    }

    // Wait until the waiter thread has actually suspended inside wait() before
    // forcing a resume; a fixed sleep is a race under load.
    while (hpx::threads::get_thread_state(waiter_id).state() !=
        hpx::threads::thread_schedule_state::suspended)
    {
        hpx::this_thread::yield();
    }

    // Force-resume the waiting HPX thread without ever calling p.set_value().
    // This mimics an external resume that leaves the future's state as 'empty',
    // which is exactly the scenario get_result_void() guards against.
    hpx::threads::set_thread_state(
        waiter_id, hpx::threads::thread_schedule_state::pending);

    waiter.join();

    // Clean up: satisfy the promise so its destructor doesn't assert/throw.
    p.set_value();
}

int hpx_main()
{
    test_future_uncompleted_error();
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST(hpx::init(argc, argv) == 0);
    return hpx::util::report_errors();
}
