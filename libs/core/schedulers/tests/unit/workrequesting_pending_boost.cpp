//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A thread repeatedly yielding with pending_boost must not starve normal work
// on the same worker. The bounded yield count prevents the test from hanging.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

int hpx_main()
{
    std::atomic<bool> normal_task_ran{false};
    auto normal_task = hpx::async([&normal_task_ran]() {
        normal_task_ran.store(true, std::memory_order_relaxed);
    });

    for (std::size_t i = 0;
        i != 64 && !normal_task_ran.load(std::memory_order_relaxed); ++i)
    {
        hpx::execution_base::this_thread::yield_k(17);
    }

    bool const completed_while_boosted =
        normal_task_ran.load(std::memory_order_relaxed);
    normal_task.wait();
    HPX_TEST(completed_while_boosted);

    return hpx::local::finalize();
}

int main(int, char* argv[])
{
    for (char const* scheduler :
        {"local-workrequesting-fifo", "local-workrequesting-mc"})
    {
        std::string const queuing = std::string("--hpx:queuing=") + scheduler;
        std::vector<char*> av = {argv[0], const_cast<char*>(queuing.c_str())};

        hpx::local::init_params init_args;
        init_args.cfg = {"hpx.os_threads=1"};

        HPX_TEST_EQ(hpx::local::init(hpx_main, static_cast<int>(av.size()),
                        av.data(), init_args),
            0);
    }

    return hpx::util::report_errors();
}
