//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test case demonstrates the issue described in #7724: post() could take
// a new reference to a run_as_child thread that another worker had already
// finished and released. It is a race, so without the fix it fails only in
// some runs.

#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <vector>

int hpx_main()
{
    // all children are hinted to worker 0 and the parent waits on every 4th
    // one right away, so some children finish on other workers while post()
    // is still running, and some run inline
    hpx::threads::thread_schedule_hint hint(0);
    hint.runs_as_child_mode(hpx::threads::thread_execution_hint::run_as_child);

    auto exec = hpx::execution::experimental::with_hint(
        hpx::execution::parallel_executor(), hint);

    std::atomic<int> count(0);
    for (int round = 0; round != 2; ++round)
    {
        std::vector<hpx::future<void>> futures;
        futures.reserve(100000);
        for (int i = 0; i != 100000; ++i)
        {
            // the race did not show up with empty children
            futures.push_back(hpx::async(exec, [&] {
                auto const start = hpx::chrono::high_resolution_clock::now();
                while (
                    (hpx::chrono::high_resolution_clock::now() - start) < 1000)
                {
                }
                ++count;
            }));

            if (i % 4 == 0)
            {
                futures.back().wait();
            }
        }
        hpx::wait_all(futures);
    }
    HPX_TEST_EQ(count.load(), 200000);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
