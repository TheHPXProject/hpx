//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/testing.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

void test_as_sender_with_scheduler_basic()
{
    // Test: future + scheduler -> sender with env
    auto f = hpx::async([] { return 42; });
    ex::thread_pool_scheduler sched{};
    auto sender = ex::as_sender(std::move(f), sched);
    // Verify compilation and basic properties
    (void) sender;
}

void test_as_sender_with_scheduler_get_env()
{
    // Test: env exposes the scheduler
    auto f = hpx::async([] { return 42; });
    ex::thread_pool_scheduler sched{};
    auto sender = ex::as_sender(std::move(f), sched);
    auto env = ex::get_env(sender);
    // Verify env can be queried for scheduler
    auto sched_from_env = ex::get_completion_scheduler<ex::set_value_t>(env);
    HPX_TEST(sched == sched_from_env);
}

void test_as_sender_with_scheduler_in_pipeline()
{
    // Full end-to-end test
    auto f = hpx::async([] { return 42; });
    ex::thread_pool_scheduler sched{};

    auto [result] =
        tt::sync_wait(ex::as_sender(std::move(f), sched) | ex::then([](int x) {
            return x * 2;
        })).value();

    HPX_TEST_EQ(result, 84);
}

void test_as_sender_with_scheduler_shared_future()
{
    // Test shared_future variant (copyable)
    auto f = hpx::make_shared_future(hpx::async([] { return 42; }));
    ex::thread_pool_scheduler sched{};
    auto sender = ex::as_sender(f, sched);    // f is lvalue, not moved

    auto [result] =
        tt::sync_wait(sender | ex::then([](int x) { return x * 2; })).value();

    HPX_TEST_EQ(result, 84);
}

void test_as_sender_with_scheduler_void()
{
    // Test void-returning futures
    bool executed = false;
    auto f = hpx::async([&] { executed = true; });
    ex::thread_pool_scheduler sched{};
    auto sender = ex::as_sender(std::move(f), sched);
    tt::sync_wait(std::move(sender));
    HPX_TEST(executed);
}

void test_as_sender_with_scheduler_error()
{
    // Test exception propagation
    auto f = hpx::async([]() -> int {
        throw std::runtime_error("test error");
        return 42;
    });
    ex::thread_pool_scheduler sched{};
    auto sender = ex::as_sender(std::move(f), sched);

    bool caught = false;
    try
    {
        tt::sync_wait(std::move(sender));
        HPX_TEST(false);    // Should have thrown
    }
    catch (std::runtime_error const& e)
    {
        caught = true;
        HPX_TEST_EQ(std::string(e.what()), std::string("test error"));
    }
    HPX_TEST(caught);
}

int hpx_main()
{
    test_as_sender_with_scheduler_basic();
    test_as_sender_with_scheduler_get_env();
    test_as_sender_with_scheduler_in_pipeline();
    test_as_sender_with_scheduler_shared_future();
    test_as_sender_with_scheduler_void();
    test_as_sender_with_scheduler_error();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
