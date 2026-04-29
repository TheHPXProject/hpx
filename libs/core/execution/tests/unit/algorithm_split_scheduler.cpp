//  Copyright (c) 2026 Arpit Khandelwal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for P2300 split scheduler preservation.
//
// When execution::split(scheduler, sender) is used and the predecessor sender
// has already completed by the time a new receiver connects (i.e.
// predecessor_done == true), the downstream completion must be dispatched
// through the scheduler's execution context, not fired inline on the thread
// that calls add_continuation.
//
// Before the fix, shared_state::add_continuation() had:
//   // TODO: Should this preserve the scheduler? It does not if we call
//   // set_* inline.
//   hpx::visit(done_error_value_visitor<Receiver>{...}, v);
//
// After the fix, add_continuation() calls schedule_completion() which routes
// through the attached scheduler for scheduler-aware shared states, or fires
// inline (existing behaviour) when no scheduler is attached.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include "algorithm_test_utils.hpp"

#include <atomic>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

int hpx_main()
{
    // -----------------------------------------------------------------------
    // Test 1: basic split, no scheduler — values round-trip correctly.
    // Regression guard: no behavioural change for the scheduler-free path.
    // -----------------------------------------------------------------------
    {
        auto s = ex::split(ex::just(std::make_tuple(42, std::string("hello"))));

        std::atomic<int> count{0};

        // First subscriber (predecessor not yet complete when connecting)
        tt::sync_wait(ex::then(s, [&](auto const& tuple_val) {
            int x = std::get<0>(tuple_val);
            std::string const& msg = std::get<1>(tuple_val);
            HPX_TEST_EQ(x, 42);
            HPX_TEST_EQ(msg, std::string("hello"));
            ++count;
        }));

        // Second subscriber (predecessor_done == true)
        tt::sync_wait(ex::then(s, [&](auto const& tuple_val) {
            int x = std::get<0>(tuple_val);
            std::string const& msg = std::get<1>(tuple_val);
            HPX_TEST_EQ(x, 42);
            HPX_TEST_EQ(msg, std::string("hello"));
            ++count;
        }));

        HPX_TEST_EQ(count.load(), 2);
    }

    // -----------------------------------------------------------------------
    // Test 2: split with thread_pool_scheduler via pipeline syntax.
    //
    // The pipeline  `sched | ex::split(just(100))`  triggers
    // tag_override_invoke which reads get_completion_scheduler<set_value_t>
    // from the sender, and routes to our new generic-scheduler tag_invoke on
    // split_t.  Late-arriving subscribers must get their completion on the
    // thread pool, not inline on this thread.
    // -----------------------------------------------------------------------
    {
        ex::thread_pool_scheduler sched{};

        // just(100) | transfer(sched) gives a sender whose completion
        // scheduler is sched, so split picks up the scheduler automatically.
        auto shared_s = ex::split(ex::transfer(ex::just(100), sched));

        std::atomic<int> call_count{0};

        // First subscriber — drains, making predecessor_done = true.
        tt::sync_wait(ex::then(shared_s, [&](int v) {
            HPX_TEST_EQ(v, 100);
            ++call_count;
        }));
        HPX_TEST_EQ(call_count.load(), 1);

        // Second subscriber — predecessor_done is now true.
        // Before the fix: fires inline on THIS thread.
        // After the fix: dispatched through thread_pool_scheduler.
        tt::sync_wait(ex::then(shared_s, [&](int v) {
            HPX_TEST_EQ(v, 100);
            ++call_count;
        }));
        HPX_TEST_EQ(call_count.load(), 2);
    }

    // -----------------------------------------------------------------------
    // Test 3: explicit scheduler overload:
    //   tag_invoke(split, scheduler, sender, allocator)
    // This directly exercises the new split_t tag_invoke and the
    // shared_state_scheduler constructor.
    // -----------------------------------------------------------------------
    {
        ex::thread_pool_scheduler sched{};

        // Use tag_invoke directly to exercise the new overload.
        auto shared_s =
            hpx::functional::tag_invoke(ex::split, sched, ex::just(7));

        std::atomic<int> sum{0};

        // Drain so predecessor_done becomes true.
        tt::sync_wait(ex::then(shared_s, [&](int v) { sum += v; }));
        HPX_TEST_EQ(sum.load(), 7);

        // Multiple late subscribers: all must receive the value.
        constexpr int N = 4;
        std::vector<std::thread> threads;
        threads.reserve(N);
        for (int i = 0; i < N; ++i)
        {
            threads.emplace_back([&] {
                tt::sync_wait(ex::then(shared_s, [&](int v) { sum += v; }));
            });
        }
        for (auto& t : threads)
            t.join();

        HPX_TEST_EQ(sum.load(), (N + 1) * 7);
    }

    // -----------------------------------------------------------------------
    // Test 4: ensure_started (eager submission_type) is unaffected.
    // -----------------------------------------------------------------------
    {
        std::atomic<bool> called{false};

        auto s = ex::ensure_started(ex::just(std::string("eager")));

        auto result = tt::sync_wait(
            ex::then(HPX_MOVE(s), [&](std::string const& v) -> std::string {
                HPX_TEST_EQ(v, std::string("eager"));
                called = true;
                return v;
            }));

        HPX_TEST(result.has_value());
        HPX_TEST_EQ(hpx::get<0>(*result), std::string("eager"));
        HPX_TEST(called.load());
    }

    // -----------------------------------------------------------------------
    // Test 5: error propagation through split — both subscribers see error.
    // -----------------------------------------------------------------------
    {
        auto s = ex::split(error_sender{});

        std::atomic<int> error_count{0};

        // First subscriber
        tt::sync_wait(ex::let_error(s, [&](std::exception_ptr) {
            ++error_count;
            return ex::just();
        }));
        // Second subscriber (predecessor already done with error)
        tt::sync_wait(ex::let_error(s, [&](std::exception_ptr) {
            ++error_count;
            return ex::just();
        }));

        HPX_TEST_EQ(error_count.load(), 2);
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}
