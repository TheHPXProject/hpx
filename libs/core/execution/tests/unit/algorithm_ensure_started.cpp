//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;

void test_eager_execution()
{
    std::atomic<bool> started{false};

    // Sender that sets started to true when executed
    auto s1 = ex::then(ex::just(), [&]() { started = true; });

    // Eagerly start the sender
    auto s2 = ex::ensure_started(std::move(s1));

    // Test Case 1: Ensure work starts even if the resulting sender isn't connected
    // ensure_started immediately connects and starts the predecessor
    HPX_TEST(started.load());

    // Connect it to sink the result, avoiding memory leaks from the shared state
    std::atomic<bool> set_value_called{false};
    auto s3 = ex::then(std::move(s2), [&]() { set_value_called = true; });
    hpx::this_thread::experimental::sync_wait(std::move(s3));

    HPX_TEST(set_value_called.load());
}

void test_multiple_connects()
{
    std::atomic<int> start_count{0};

    auto s1 = ex::then(ex::just(), [&]() {
        ++start_count;
        return 42;
    });

    auto s2 = ex::ensure_started(std::move(s1));

    HPX_TEST_EQ(start_count.load(), 1);

    // Test Case 2: Ensure multiple connects to the same started operation receive the same value
    int result1 = 0;
    auto s3 = ex::then(s2, [&](int val) { result1 = val; });
    hpx::this_thread::experimental::sync_wait(std::move(s3));
    HPX_TEST_EQ(result1, 42);

    int result2 = 0;
    auto s4 = ex::then(s2, [&](int val) { result2 = val; });
    hpx::this_thread::experimental::sync_wait(std::move(s4));
    HPX_TEST_EQ(result2, 42);

    // The predecessor should only have been started ONCE!
    HPX_TEST_EQ(start_count.load(), 1);
}

void test_error_propagation()
{
    auto s1 = ex::then(ex::just(), []() { throw std::runtime_error("error"); });
    auto s2 = ex::ensure_started(std::move(s1));

    bool caught = false;
    try
    {
        hpx::this_thread::experimental::sync_wait(s2);
    }
    catch (std::runtime_error const&)
    {
        caught = true;
    }

    HPX_TEST(caught);
}

int hpx_main()
{
    test_eager_execution();
    test_multiple_connects();
    test_error_propagation();
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
