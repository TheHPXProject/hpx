//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
void test_executor_scheduler_schedule()
{
    using namespace hpx::execution::experimental;

    hpx::execution::sequenced_executor exec;

    // Retrieve a P2300-compliant scheduler from the legacy executor
    auto sched = get_scheduler(exec);

    // Verify the scheduler satisfies the is_scheduler trait
    static_assert(is_scheduler_v<decltype(sched)>,
        "executor_scheduler must satisfy is_scheduler");

    // Create a sender pipeline: schedule | then
    auto s = then(schedule(sched), []() { return 42; });

    // Execute synchronously and verify the result
    auto result = hpx::this_thread::experimental::sync_wait(std::move(s));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(hpx::get<0>(*result), 42);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_executor_scheduler_schedule();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
