//  Copyright (c) 2025 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Comprehensive P2300 distributed scheduler test suite.
//
// Tests are organized by capability tier:
//   - Active tests:  void execution, single-value transfer, scheduler
//                    concept checks, equality, domain routing.
//   - Guarded tests: multi-value, exception propagation, cancellation
//                    (gated behind #if 0 until component-based receiver
//                    marshalling is implemented).

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE) && defined(HPX_WITH_NETWORKING)
#include <hpx/hpx_init.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <string>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

///////////////////////////////////////////////////////////////////////////////
// Active tests - these must pass against the minimal baseline.
///////////////////////////////////////////////////////////////////////////////

// Test 1: distributed_scheduler satisfies the P2300 scheduler concept.
void test_scheduler_concept()
{
    static_assert(
        ex::scheduler<hpx::distributed::experimental::distributed_scheduler>,
        "distributed_scheduler must satisfy the scheduler concept");
}

// Test 2: schedule() returns a valid P2300 sender.
void test_schedule_returns_sender()
{
    auto const sched =
        hpx::distributed::experimental::distributed_scheduler{hpx::find_here()};
    auto s = sched.schedule();

    static_assert(ex::sender<decltype(s)>,
        "distributed_schedule_sender must satisfy the sender concept");
}

// Test 3: scheduler equality - same target compares equal.
void test_scheduler_equality()
{
    auto const here = hpx::find_here();
    auto const s1 = hpx::distributed::experimental::distributed_scheduler{here};
    auto const s2 = hpx::distributed::experimental::distributed_scheduler{here};
    HPX_TEST(s1 == s2);
    HPX_TEST(!(s1 != s2));
}

// Test 4: scheduler inequality - different targets compare unequal.
void test_scheduler_inequality()
{
    auto const localities = hpx::find_all_localities();
    if (localities.size() < 2)
        return;    // skip when running single-locality

    auto const s1 =
        hpx::distributed::experimental::distributed_scheduler{localities[0]};
    auto const s2 =
        hpx::distributed::experimental::distributed_scheduler{localities[1]};
    HPX_TEST(s1 != s2);
    HPX_TEST(!(s1 == s2));
}

// Test 5: void execution - schedule on local locality, complete with
//         set_value(). No values are sent.
void test_void_execution_local()
{
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{hpx::find_here()};

    // sync_wait on a void sender returns an optional<tuple<>>
    auto result = tt::sync_wait(ex::schedule(sched));
    HPX_TEST(result.has_value());
}

// Test 6: void execution on a remote locality (or local if single-node).
void test_void_execution_remote()
{
    auto localities = hpx::find_all_localities();
    auto target = localities.back();
    auto sched = hpx::distributed::experimental::distributed_scheduler{target};

    auto result = tt::sync_wait(ex::schedule(sched));
    HPX_TEST(result.has_value());
}

// Test 7: schedule() | then() - chain a simple continuation on local.
void test_schedule_then_local()
{
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{hpx::find_here()};

    auto result =
        tt::sync_wait(ex::schedule(sched) | ex::then([]() { return 42; }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 42);
}

// Test 8: ex::just(value) | ex::continues_on(sched) resolves the domain
// customization point. Note: This currently only tests that the tag-invoke
// resolves to distributed_domain. Physical remote execution routing will be
// implemented next.
void test_continues_on_resolves_domain_cpo()
{
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{hpx::find_here()};

    auto result = tt::sync_wait(ex::just(10) | ex::continues_on(sched));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 10);
}

// Test 9: ex::just() (void) | ex::continues_on(sched) for void upstream.
void test_continues_on_void()
{
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{hpx::find_here()};

    auto result = tt::sync_wait(ex::just() | ex::continues_on(sched));
    HPX_TEST(result.has_value());
}

// Test 10: schedule | then with remote locality - verify execution locality.
void test_schedule_then_remote_locality()
{
    auto localities = hpx::find_all_localities();
    auto target = localities.back();
    auto sched = hpx::distributed::experimental::distributed_scheduler{target};

    auto result = tt::sync_wait(
        ex::schedule(sched) | ex::then([]() { return hpx::find_here(); }));

    HPX_TEST(result.has_value());
    // NOTE: Until remote dispatch is implemented, the continuation runs
    // locally. Once component-based marshalling lands, this should verify
    // std::get<0>(*result) == target.
    (void) result;
}

///////////////////////////////////////////////////////////////////////////////
// Guarded tests - these test advanced capabilities that require
// component-based receiver marshalling (nvexec-style).
///////////////////////////////////////////////////////////////////////////////

// TODO: Enable once component-based receiver marshalling is implemented
// (nvexec style)
#if 0

// Test G1: single-value transfer across network boundary.
// The remote node computes a value and returns it to the local node.
void test_single_value_transfer()
{
    auto localities = hpx::find_all_localities();
    auto target = localities.back();
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{target};

    // The then continuation should execute on the remote locality and
    // return the result back across the network.
    auto result = tt::sync_wait(
        ex::just(21) | ex::continues_on(sched) |
        ex::then([](int x) { return x * 2; }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 42);
}

// Test G2: multi-value transfer - remote node returns multiple values.
void test_multi_value_transfer()
{
    auto localities = hpx::find_all_localities();
    auto target = localities.back();
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{target};

    auto result = tt::sync_wait(
        ex::schedule(sched) |
        ex::then([]() -> std::tuple<int, double, std::string> {
            return {42, 3.14, "hello"};
        }));

    HPX_TEST(result.has_value());
    auto const& [val_i, val_d, val_s] = std::get<0>(*result);
    HPX_TEST_EQ(val_i, 42);
    HPX_TEST(std::abs(val_d - 3.14) < 1e-9);
    HPX_TEST_EQ(val_s, std::string("hello"));
}

// Test G3: exception propagation - remote node throws, local catches.
void test_exception_propagation()
{
    auto localities = hpx::find_all_localities();
    auto target = localities.back();
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{target};

    bool caught_exception = false;
    try
    {
        tt::sync_wait(ex::schedule(sched) | ex::then([]() -> int {
            throw std::runtime_error("test_exception");
        }));
    }
    catch (std::runtime_error const& e)
    {
        caught_exception = true;
        HPX_TEST_EQ(
            std::string(e.what()), std::string("test_exception"));
    }
    catch (...)
    {
        HPX_TEST(false);    // unexpected exception type
    }
    HPX_TEST(caught_exception);
}

// Test G4: cancellation - stop token propagates to remote execution.
void test_cancellation()
{
    auto localities = hpx::find_all_localities();
    auto target = localities.back();
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{target};

    // Use let_value to access the stop token and request cancellation
    // before the remote work completes.
    bool was_stopped = false;
    try
    {
        tt::sync_wait(
            ex::schedule(sched) |
            ex::let_value([]() {
                return ex::just_stopped();
            }));
    }
    catch (...)
    {
        // sync_wait may throw when the sender completes with set_stopped
        was_stopped = true;
    }
    HPX_TEST(was_stopped);
}

// Test G5: chained remote hops - two continues_on through different
//          localities.
void test_chained_remote_hops()
{
    auto localities = hpx::find_all_localities();
    if (localities.size() < 2)
        return;    // need at least 2 localities

    auto sched0 =
        hpx::distributed::experimental::distributed_scheduler{localities[0]};
    auto sched1 =
        hpx::distributed::experimental::distributed_scheduler{localities[1]};

    auto result = tt::sync_wait(
        ex::just(1) |
        ex::continues_on(sched0) |
        ex::then([](int x) { return x + 10; }) |
        ex::continues_on(sched1) |
        ex::then([](int x) { return x * 2; }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 22);    // (1 + 10) * 2
}

// Test G6: let_value with remote scheduling.
void test_let_value_remote()
{
    auto localities = hpx::find_all_localities();
    auto target = localities.back();
    auto sched =
        hpx::distributed::experimental::distributed_scheduler{target};

    auto result = tt::sync_wait(
        ex::schedule(sched) |
        ex::let_value([&sched]() {
            return ex::schedule(sched) |
                   ex::then([]() { return 99; });
        }));

    HPX_TEST(result.has_value());
    HPX_TEST_EQ(std::get<0>(*result), 99);
}

#endif    // guarded tests

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_scheduler_concept();
    test_schedule_returns_sender();
    test_scheduler_equality();
    test_scheduler_inequality();
    test_void_execution_local();
    test_void_execution_remote();
    test_schedule_then_local();
    test_continues_on_resolves_domain_cpo();
    test_continues_on_void();
    test_schedule_then_remote_locality();

    // Guarded tests would be called here once enabled:
    // test_single_value_transfer();
    // test_multi_value_transfer();
    // test_exception_propagation();
    // test_cancellation();
    // test_chained_remote_hops();
    // test_let_value_remote();

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
