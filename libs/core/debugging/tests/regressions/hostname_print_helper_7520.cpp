//  Copyright (c) 2026 Rohan on Keys
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for issue #7520.
//
// hostname_print_helper::get_hostname() lazily filled a static buffer
// behind a plain bool flag. Two worker threads could both observe the
// flag as false and race on writing the flag and the buffer, so the
// flag could become true before the buffer was fully written.
//
// This calls get_hostname() many times from tasks spread across multiple
// worker threads and checks every task observed the same fully
// initialized string once all tasks have finished.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/debugging.hpp>
#include <hpx/modules/testing.hpp>

#include <string>
#include <vector>

constexpr int num_tasks = 64;
constexpr int calls_per_task = 1000;

int hpx_main()
{
    std::vector<std::string> results(num_tasks);
    std::vector<hpx::future<void>> futures;
    futures.reserve(num_tasks);

    for (int i = 0; i < num_tasks; ++i)
    {
        futures.push_back(hpx::async([&results, i]() {
            hpx::debug::detail::hostname_print_helper helper;
            char const* name = nullptr;
            for (int j = 0; j < calls_per_task; ++j)
            {
                name = helper.get_hostname();
                HPX_TEST(name != nullptr);
            }
            results[i] = name;
        }));
    }

    hpx::wait_all(futures);

    for (int i = 1; i < num_tasks; ++i)
    {
        HPX_TEST_EQ(results[i], results[0]);
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
