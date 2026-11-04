//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <thread>

int hpx_main()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST(hpx::local::start(hpx_main, argc, argv));

    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};

    auto do_stop = [&]() {
        try
        {
            int const res = hpx::local::stop();
            if (res == 0)
            {
                ++success_count;
            }
        }
        catch (hpx::exception const& e)
        {
            if (e.get_error() == hpx::error::invalid_status)
            {
                ++error_count;
            }
        }
    };

    std::thread t1(do_stop);
    std::thread t2(do_stop);

    t1.join();
    t2.join();

    HPX_TEST_EQ(success_count.load(), 1);
    HPX_TEST_EQ(error_count.load(), 1);

    return hpx::util::report_errors();
}
