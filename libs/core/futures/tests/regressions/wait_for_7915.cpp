//  Copyright (c) 2026 STE||AR Group
//  SPDX-License-Identifier: BSL-1.0

#include <hpx/hpx_main.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <thread>

using namespace std::chrono;

int main()
{
    // set_value arrives while a non-HPX thread is in a timed wait
    {
        hpx::promise<int> p;
        hpx::future<int> f = p.get_future();

        std::thread waiter([&] {
            hpx::future_status const status = f.wait_for(seconds(10));
            HPX_TEST(status == hpx::future_status::ready);
        });

        std::this_thread::sleep_for(milliseconds(5));
        p.set_value(42);
        waiter.join();

        HPX_TEST_EQ(f.get(), 42);
    }

    // untimed wait from a non-HPX thread
    {
        hpx::promise<int> p;
        hpx::future<int> f = p.get_future();

        std::thread waiter([&] { f.wait(); });

        std::this_thread::sleep_for(milliseconds(5));
        p.set_value(42);
        waiter.join();

        HPX_TEST_EQ(f.get(), 42);
    }

    // the waiter times out before set_value arrives
    {
        hpx::promise<int> p;
        hpx::future<int> f = p.get_future();

        std::thread waiter([&] {
            hpx::future_status const status = f.wait_for(milliseconds(1));
            HPX_TEST(status == hpx::future_status::timeout);
        });
        waiter.join();

        p.set_value(42);
    }

    return hpx::util::report_errors();
}
