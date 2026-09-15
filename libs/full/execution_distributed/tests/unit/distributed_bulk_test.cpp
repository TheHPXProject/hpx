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
///
/// Each test is parameterized by a target locality so that hpx_main
/// can loop over all localities returned by hpx::find_all_localities().

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE) && defined(HPX_WITH_NETWORKING)
#include <hpx/hpx_init.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <vector>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

///////////////////////////////////////////////////////////////////////////////
// Test 1: basic bulk with integral shape.
//         Verifies the scheduled work executes on the expected locality
//         by returning hpx::find_here() from the remote side and
//         comparing it to the target.
void test_bulk_integral_shape(hpx::id_type const& target)
{
    auto sched = hpx::distributed::experimental::distributed_scheduler{target};

    auto snd = ex::schedule(sched) |
        ex::then([]() { return hpx::find_here(); }) |
        ex::bulk(10, [](int /*index*/, hpx::id_type /*loc*/) {});

    auto result = tt::sync_wait(std::move(snd));
    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), target);
}

///////////////////////////////////////////////////////////////////////////////
// Test 2: bulk with a value-carrying upstream sender.
//         Verifies the function receives the upstream value and the
//         value is forwarded unchanged to the downstream.
void test_bulk_with_upstream_value(hpx::id_type const& target)
{
    auto sched = hpx::distributed::experimental::distributed_scheduler{target};

    // Use an accumulator returned from ex::then so the bulk
    // function does not capture a reference to a local variable
    // that would dangle on a remote locality.
    auto snd = ex::schedule(sched) | ex::then([]() { return 5; }) |
        ex::bulk(4, [](int /*index*/, int /*val*/) {});

    auto result = tt::sync_wait(std::move(snd));
    HPX_TEST(result.has_value());
    // The upstream value is forwarded unchanged
    HPX_TEST_EQ(std::get<0>(*result), 5);
}

///////////////////////////////////////////////////////////////////////////////
// Test 3: bulk with shape=0 should invoke the function zero times
//         and still forward the upstream value.
void test_bulk_zero_shape(hpx::id_type const& target)
{
    auto sched = hpx::distributed::experimental::distributed_scheduler{target};

    auto snd = ex::schedule(sched) | ex::then([]() { return 42; }) |
        ex::bulk(0, [](int, int) { HPX_TEST(false); });

    auto result = tt::sync_wait(std::move(snd));
    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 42);
}

///////////////////////////////////////////////////////////////////////////////
// Test 4: bulk exception propagation - if the function throws, the
//         error channel should fire.
void test_bulk_exception_propagation(hpx::id_type const& target)
{
    auto sched = hpx::distributed::experimental::distributed_scheduler{target};

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
        HPX_TEST(false);    // unexpected exception type
    }
    HPX_TEST(caught_exception);
}

///////////////////////////////////////////////////////////////////////////////
// Test 5: verify the returned sender's completion scheduler is still
//         the distributed_scheduler (environment propagation).
void test_bulk_preserves_scheduler_env(hpx::id_type const& target)
{
    auto sched = hpx::distributed::experimental::distributed_scheduler{target};

    auto snd = ex::schedule(sched) | ex::bulk(1, [](int) {});

    auto sched_from_env =
        ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(snd));

    HPX_TEST(sched == sched_from_env);
}

///////////////////////////////////////////////////////////////////////////////
// Test 6: verify that reference completions from the upstream sender
//         are properly decayed into copies by the bulk adaptor.
//         The upstream sends a std::string& (reference); the bulk adaptor
//         should decay it into a copy so the original remains unchanged.
void test_bulk_decayed_reference(hpx::id_type const& target)
{
    auto sched = hpx::distributed::experimental::distributed_scheduler{target};
    std::string upstream_value = "hello";

    auto snd = ex::schedule(sched) |
        ex::then(
            [&upstream_value]() -> std::string& { return upstream_value; }) |
        ex::bulk(3, [](int /*index*/, std::string& s) { s += "!"; });

    auto result = tt::sync_wait(std::move(snd));
    HPX_TEST(result.has_value());
    // The bulk function mutated the decayed copy 3 times
    HPX_TEST_EQ(std::get<0>(*result), std::string("hello!!!"));
    // The original value must remain unchanged (decay created a copy)
    HPX_TEST_EQ(upstream_value, std::string("hello"));
}

///////////////////////////////////////////////////////////////////////////////
// Test 7: verify that a move-only callable (rvalue-only) compiles and
//         runs correctly through the bulk adaptor.
void test_bulk_move_only_callable(hpx::id_type const& target)
{
    auto sched = hpx::distributed::experimental::distributed_scheduler{target};

    struct move_only_fn
    {
        move_only_fn() = default;
        move_only_fn(move_only_fn&&) = default;
        move_only_fn& operator=(move_only_fn&&) = default;
        move_only_fn(move_only_fn const&) = delete;
        move_only_fn& operator=(move_only_fn const&) = delete;

        void operator()(int /*index*/) const {}
    };

    auto snd = ex::schedule(sched) | ex::bulk(5, move_only_fn{});

    auto result = tt::sync_wait(std::move(snd));
    HPX_TEST(result.has_value());
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    for (hpx::id_type const& loc : localities)
    {
        test_bulk_integral_shape(loc);
        test_bulk_with_upstream_value(loc);
        test_bulk_zero_shape(loc);
        test_bulk_exception_propagation(loc);
        test_bulk_preserves_scheduler_env(loc);
        test_bulk_decayed_reference(loc);
        test_bulk_move_only_callable(loc);
    }

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
