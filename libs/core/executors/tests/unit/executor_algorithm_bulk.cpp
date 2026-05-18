//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace ex = hpx::execution::experimental;

void test_sequential_bulk()
{
    hpx::execution::sequenced_executor exec;
    std::atomic<int> call_count{0};

    auto snd = ex::schedule(exec) | ex::bulk(10, [&](int i) {
        (void) i;
        ++call_count;
        // Should run sequentially, so it's safe
    });

    hpx::this_thread::experimental::sync_wait(snd);


    HPX_TEST_EQ(call_count.load(), 10);
}

void test_parallel_bulk()
{
    hpx::execution::parallel_executor exec;
    std::atomic<int> call_count{0};

    auto snd = ex::schedule(exec) | ex::bulk(1000, [&](int i) {
        (void) i;
        ++call_count;
    });

    hpx::this_thread::experimental::sync_wait(snd);

    HPX_TEST_EQ(call_count.load(), 1000);
}

void test_parallel_bulk_with_value()
{
    hpx::execution::parallel_executor exec;
    std::atomic<int> call_count{0};

    auto snd = ex::schedule(exec) | ex::then([]() { return 42; }) |
        ex::bulk(100, [&](int i, int val) {
            (void) i;
            HPX_TEST_EQ(val, 42);
            ++call_count;
        });

    hpx::this_thread::experimental::sync_wait(snd);

    HPX_TEST_EQ(call_count.load(), 100);
}

int hpx_main()
{
    test_sequential_bulk();
    test_parallel_bulk();
    test_parallel_bulk_with_value();
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
