//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// Unit tests for the distributed bulk sender adaptor.
///
/// Verifies that ex::bulk() dispatches through the distributed_scheduler
/// when the upstream sender's completion scheduler is a
/// distributed_scheduler, and that the shape-indexed invocation executes
/// the correct number of times.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE) && defined(HPX_WITH_NETWORKING)
#include <hpx/hpx_init.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

///////////////////////////////////////////////////////////////////////////////
// Test 1: basic bulk with integral shape on local distributed_scheduler.
//         Verifies the function is invoked exactly `shape` times.
void test_bulk_integral_shape()
{
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{hpx::find_here()};

    std::atomic<int> count{0};
    auto snd = ex::schedule(sched) | ex::bulk(10, [&](int) { count++; });

    auto result = tt::sync_wait(std::move(snd));
    HPX_TEST(result.has_value());
    HPX_TEST_EQ(count.load(), 10);
}

///////////////////////////////////////////////////////////////////////////////
// Test 2: bulk with a value-carrying upstream sender.
//         Verifies the function receives the upstream value and the
//         value is forwarded unchanged to the downstream.
void test_bulk_with_upstream_value()
{
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{hpx::find_here()};

    std::atomic<int> sum{0};
    auto snd = ex::schedule(sched) | ex::then([]() { return 5; }) |
        ex::bulk(4, [&](int /*index*/, int val) { sum += val; });

    auto result = tt::sync_wait(std::move(snd));
    HPX_TEST(result.has_value());
    // The function was called 4 times, each time with val=5
    HPX_TEST_EQ(sum.load(), 20);
    // The upstream value is forwarded unchanged
    HPX_TEST_EQ(std::get<0>(*result), 5);
}

///////////////////////////////////////////////////////////////////////////////
// Test 3: bulk with shape=0 should invoke the function zero times
//         and still forward the upstream value.
void test_bulk_zero_shape()
{
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{hpx::find_here()};

    std::atomic<int> count{0};
    auto snd = ex::schedule(sched) | ex::then([]() { return 42; }) |
        ex::bulk(0, [&](int, int) { count++; });

    auto result = tt::sync_wait(std::move(snd));
    HPX_TEST(result.has_value());
    HPX_TEST_EQ(count.load(), 0);
    HPX_TEST_EQ(std::get<0>(*result), 42);
}

///////////////////////////////////////////////////////////////////////////////
// Test 4: bulk exception propagation - if the function throws, the
//         error channel should fire.
void test_bulk_exception_propagation()
{
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{hpx::find_here()};

    bool caught_exception = false;
    try
    {
        auto snd = ex::schedule(sched) | ex::bulk(5, [](int i) {
            if (i == 3)
            {
                throw std::runtime_error("bulk_error");
            }
        });
        tt::sync_wait(std::move(snd));
    }
    catch (std::runtime_error const& e)
    {
        caught_exception = true;
        HPX_TEST_EQ(std::string(e.what()), std::string("bulk_error"));
    }
    catch (...)
    {
        caught_exception = true;
    }
    HPX_TEST(caught_exception);
}

///////////////////////////////////////////////////////////////////////////////
// Test 5: verify the returned sender's completion scheduler is still
//         the distributed_scheduler (environment propagation).
void test_bulk_preserves_scheduler_env()
{
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{hpx::find_here()};

    auto snd = ex::schedule(sched) | ex::bulk(1, [](int) {});

    auto sched_from_env =
        ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(snd));

    HPX_TEST(sched == sched_from_env);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_bulk_integral_shape();
    test_bulk_with_upstream_value();
    test_bulk_zero_shape();
    test_bulk_exception_propagation();
    test_bulk_preserves_scheduler_env();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
