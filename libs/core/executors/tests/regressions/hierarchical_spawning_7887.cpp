//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test case demonstrates the issue described in #7887: hierarchical bulk
// execution captured the launch policy of each part by reference, so a task
// running after its loop iteration had ended read a dangling policy.

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <numeric>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// 16 elements make every part larger than the default hierarchical threshold
// of 7 with up to 2 workers, so the parts are run by tasks posted for them.
void test_hierarchical_bulk(int rounds)
{
    namespace ex = hpx::parallel::execution;

    hpx::execution::parallel_executor_spawn_hierarchically exec;

    std::vector<int> shape(16);
    std::iota(shape.begin(), shape.end(), 0);

    auto twice = [](int i) { return 2 * i; };

    std::atomic<std::size_t> count(0);
    auto counted = [&count](int) { ++count; };

    for (int round = 0; round != rounds; ++round)
    {
        std::vector<int> results = ex::bulk_sync_execute(exec, twice, shape);
        HPX_TEST_EQ(results.size(), shape.size());
        for (std::size_t i = 0; i != results.size(); ++i)
        {
            HPX_TEST_EQ(results[i], 2 * shape[i]);
        }

        std::vector<hpx::future<int>> futures =
            ex::bulk_async_execute(exec, twice, shape);
        HPX_TEST_EQ(futures.size(), shape.size());
        for (std::size_t i = 0; i != futures.size(); ++i)
        {
            HPX_TEST_EQ(futures[i].get(), 2 * shape[i]);
        }

        count = 0;
        ex::bulk_sync_execute(exec, counted, shape);
        HPX_TEST_EQ(count.load(), shape.size());

        count = 0;
        ex::bulk_async_execute(exec, counted, shape).get();
        HPX_TEST_EQ(count.load(), shape.size());
    }
}

int hpx_main()
{
    test_hierarchical_bulk(10);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
