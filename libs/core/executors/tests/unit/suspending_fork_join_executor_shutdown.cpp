//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The worker threads of a suspending_fork_join_executor suspend while they
// wait for work. A runtime can't stop while it has suspended threads, so the
// worker threads have to exit when the runtime starts to stop, even if the
// executor still exists (here: a global executor).

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <vector>

using hpx::execution::experimental::suspending_fork_join_executor;

constinit suspending_fork_join_executor global_exec{};

std::atomic<std::size_t> count{0};

int hpx_main()
{
    // creates the worker threads, which then suspend
    std::vector<int> v(107);
    hpx::parallel::execution::bulk_sync_execute(
        global_exec, [](int) { ++count; }, v);
    HPX_TEST_EQ(count.load(), v.size());

    // global_exec still exists, with suspended worker threads
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    // the worker threads have exited, the executor can't be used anymore
    bool caught = false;
    try
    {
        hpx::parallel::execution::bulk_sync_execute(
            global_exec, [](int) {}, std::vector<int>(4));
    }
    catch (hpx::exception const& e)
    {
        caught = e.get_error() == hpx::error::invalid_status;
    }
    HPX_TEST(caught);

    return hpx::util::report_errors();
}
