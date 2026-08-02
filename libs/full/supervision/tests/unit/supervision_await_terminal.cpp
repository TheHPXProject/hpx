//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/supervision.hpp>

#include "supervision_test_helpers.hpp"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <vector>

// ============================================================================
// Test Cases: await_terminal()
// ============================================================================

// If `target` has already reached a terminal event within `epoch` before
// await_terminal() is called, the returned future must be ready immediately
// (except if locality is remote: the returned future represents the inner
// future, i.e. the one to be unwrapped, which becomes available only once the
// outer future has become ready), holding that terminal state -- no waiter is
// registered/left pending.
void test_await_terminal_fast_path(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running(locality, target);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    auto f = hpx::supervision::await_terminal(locality, target);
    HPX_TEST(f.is_ready() || locality != hpx::find_here());

    auto const state = f.get();
    HPX_TEST_EQ(state.actor, target);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);
}

// Calling await_terminal() before the terminal event has been published
// registers a waiter (check-then-register happens under a single lock
// acquisition, closing the race between polling and subscribing); publishing
// the terminal event afterward must resolve the previously-returned future
// with the recorded state.
void test_await_terminal_registers_then_resolves(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running(locality, target);

    auto f = hpx::supervision::await_terminal(locality, target);
    HPX_TEST(!f.is_ready());

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    auto const state = f.get();
    HPX_TEST_EQ(state.actor, target);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);
}

// A waiter registered against a given epoch must never resolve from a
// terminal event published under a different (here, higher) epoch: the
// waiter table is keyed on the exact (target, epoch) pair, so a waiter for a
// superseded epoch is simply abandoned.
void test_await_terminal_epoch_mismatch_never_resolves(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running_at_epoch(locality, target, 1);

    auto const f = hpx::supervision::await_terminal(locality, target, 1);
    HPX_TEST(!f.is_ready());

    // A higher epoch reaching a terminal event must not resolve a waiter
    // registered against the old epoch.
    reach_running_at_epoch(locality, target, 2);

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed, 2);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));
    HPX_TEST(f.is_ready() && f.has_exception());
}

// Concurrent publish_event() + await_terminal() stress test: while a batch
// of callers is still concurrently registering (or fast-pathing through)
// await_terminal() for the same (target, epoch) pair, a terminal event is
// published for that pair exactly once, concurrently, from another task.
// Every await_terminal() future -- whether it observed the fast path or
// raced publish_event() into registering a waiter -- must resolve with the
// terminal state; this exercises the lock-protected extract -> unlock ->
// set_value path in publish_event() under contention.
void test_await_terminal_concurrent_publish_stress(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    reach_running(locality, target);

    constexpr int num_waiters = 50;

    hpx::mutex results_mtx;
    std::vector<hpx::future<hpx::supervision::lifecycle_state>> results;
    results.reserve(num_waiters);

    std::vector<hpx::future<void>> registrations;
    registrations.reserve(num_waiters);
    for (int i = 0; i != num_waiters; ++i)
    {
        registrations.push_back(hpx::async(hpx::launch::task, [&] {
            auto f = hpx::supervision::await_terminal(locality, target);
            std::scoped_lock<hpx::mutex> lock(results_mtx);
            results.push_back(HPX_MOVE(f));
        }));
    }

    auto publication = hpx::async(hpx::launch::task, [&] {
        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::completed);
    });

    hpx::wait_all(registrations);
    publication.get();

    HPX_TEST_EQ(results.size(), static_cast<std::size_t>(num_waiters));
    for (auto& f : results)
    {
        auto const state = f.get();
        HPX_TEST_EQ(state.actor, target);
        HPX_TEST(state.last_event == hpx::supervision::event::completed);
    }
}

// Cross-locality regression test: an outstanding await_terminal_action future
// issued from this locality against a remote locality must fail with
// hpx::error::invalid_status when a stale-epoch sweep drains its promise on the
// remote locality -- validating that the action wrapper correctly propagates
// the explicit invalidation exception to remote callers.
void test_await_terminal_cross_locality_epoch_bump_abandons_remote_waiter()
{
    std::vector<hpx::id_type> const remote_localities =
        hpx::find_remote_localities();
    if (remote_localities.empty())
    {
        // This regression test only exercises anything interesting when run
        // with more than one locality.
        return;
    }

    hpx::id_type const locality = remote_localities[0];
    hpx::id_type const target = make_test_target();
    reach_running_at_epoch(locality, target, 1);

    auto f = hpx::supervision::await_terminal(locality, target, 1);
    HPX_TEST(!f.is_ready());

    // A higher epoch reaching a terminal event on the remote locality must
    // sweep (and abandon) the outstanding remote waiter registered against the
    // old epoch.
    reach_running_at_epoch(locality, target, 2);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed, 2);

    bool caught_invalid_status = false;
    try
    {
        f.get();
        HPX_TEST(false);
    }
    catch (hpx::exception const& e)
    {
        caught_invalid_status = (e.get_error() == hpx::error::stale_state);
    }
    HPX_TEST(caught_invalid_status);
}

// A waiter that is reached by neither the exact-epoch resolution path nor the
// epoch-supersession invalidation path (e.g. the target never publishes another
// event) must still not accumulate in the server's waiters_ map indefinitely:
// it is expected to be swept and invalidated once its explicit `timeout`
// elapses. This is enforced independently by the server's background sweep
// timer; the unrelated publish_event() call for a different target below is
// only there to exercise the opportunistic fast-path sweep as well.
void test_await_terminal_abandoned_waiter_expires(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    reach_running(locality, target);

    auto f = hpx::supervision::await_terminal(
        locality, target, 0, std::chrono::milliseconds(20));
    HPX_TEST(!f.is_ready());

    hpx::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Trigger the opportunistic expiry sweep via an unrelated publish_event()
    // call; the abandoned waiter above must never resolve from this.
    hpx::id_type const other_target = make_test_target();
    reach_running(locality, other_target);

    HPX_TEST(!f.is_ready() || f.has_exception());

    bool caught_stale_state = false;
    try
    {
        f.get();
        HPX_TEST(false);
    }
    catch (hpx::exception const& e)
    {
        caught_stale_state = (e.get_error() == hpx::error::future_cancelled);
    }
    HPX_TEST(caught_stale_state);
}

// Leaving `timeout` at its sentinel value (i.e. not passing it at all) must
// fall back to the server's built-in default deadline (60s), which is far
// longer than what any of the other tests in this file exercise: a waiter
// registered this way must therefore still be pending well before that
// default could possibly elapse, and must resolve normally once the target
// reaches a terminal event.
void test_await_terminal_default_timeout_not_expired_early(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    reach_running(locality, target);

    auto f = hpx::supervision::await_terminal(locality, target);
    HPX_TEST(!f.is_ready());

    hpx::this_thread::sleep_for(std::chrono::milliseconds(50));
    HPX_TEST(!f.is_ready());

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    auto const state = f.get();
    HPX_TEST_EQ(state.actor, target);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);
}

// An explicit `timeout` shorter than the server's built-in default must not
// be capped or replaced by that default: a waiter registered with a short
// explicit timeout must still resolve normally, well within that timeout, if
// the target reaches a terminal event before it elapses.
void test_await_terminal_explicit_timeout_overrides_default(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    reach_running(locality, target);

    auto f = hpx::supervision::await_terminal(
        locality, target, 0, std::chrono::milliseconds(200));
    HPX_TEST(!f.is_ready());

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    auto const state = f.get();
    HPX_TEST_EQ(state.actor, target);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);
}

// Regression test pinning down the invariant that "check-then-register" in
// await_terminal() and "epoch bump + drain" in apply_new_epoch_locked() are
// serialized on the same mtx_: a batch of waiters concurrently registered
// against a not-yet-reached epoch must be deterministically resolved once
// that epoch's terminal event is published, regardless of lock-timing
// (i.e. no waiter is left hanging or resolved with a stale state due to a
// race between insertion into waiters_ and the epoch transition).
void test_await_terminal_concurrent_epoch_bump_resolves_waiters(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running_at_epoch(locality, target, 0);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed, 0);

    auto baseline = hpx::supervision::await_terminal(locality, target, 0);
    auto const baseline_state = baseline.get();
    HPX_TEST_EQ(baseline_state.epoch, static_cast<std::uint64_t>(0));
    HPX_TEST(baseline_state.last_event == hpx::supervision::event::completed);

    constexpr int num_waiters = 20;

    hpx::mutex results_mtx;
    std::vector<hpx::future<hpx::supervision::lifecycle_state>> results;
    results.reserve(num_waiters);

    std::vector<hpx::future<void>> registrations;
    registrations.reserve(num_waiters);
    for (int i = 0; i != num_waiters; ++i)
    {
        registrations.push_back(hpx::async(hpx::launch::task, [&] {
            auto f = hpx::supervision::await_terminal(
                locality, target, 1, std::chrono::seconds(5));
            std::scoped_lock<hpx::mutex> lock(results_mtx);
            results.push_back(HPX_MOVE(f));
        }));
    }
    hpx::wait_all(registrations);

    // Publishing the terminal event epoch 1 (after moving the state to running)
    // must drain and resolve every waiter registered above against (target, 1).
    reach_running_at_epoch(locality, target, 1);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed, 1);

    HPX_TEST_EQ(results.size(), static_cast<std::size_t>(num_waiters));
    for (auto& f : results)
    {
        auto const state = f.get();
        HPX_TEST_EQ(state.epoch, static_cast<std::uint64_t>(1));
        HPX_TEST(state.last_event == hpx::supervision::event::completed);
    }
}

// Companion to the above: waiters registered against an epoch that is then
// skipped entirely (the target jumps straight from epoch 0 to epoch 2) must
// be invalidated via drain_stale_waiters_locked() with the explicit
// "epoch advanced beyond the requested epoch" exception, rather than being
// left pending or resolved with the wrong state.
void test_await_terminal_concurrent_epoch_skip_invalidates_waiters(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running_at_epoch(locality, target, 0);

    constexpr int num_waiters = 20;

    hpx::mutex results_mtx;
    std::vector<hpx::future<hpx::supervision::lifecycle_state>> results;
    results.reserve(num_waiters);

    std::vector<hpx::future<void>> registrations;
    registrations.reserve(num_waiters);
    for (int i = 0; i != num_waiters; ++i)
    {
        registrations.push_back(hpx::async(hpx::launch::task, [&] {
            auto f = hpx::supervision::await_terminal(
                locality, target, 1, std::chrono::seconds(5));
            std::scoped_lock<hpx::mutex> lock(results_mtx);
            results.push_back(HPX_MOVE(f));
        }));
    }
    hpx::wait_all(registrations);

    // Skip epoch 1 entirely: publishing directly at epoch 2 must invalidate
    // (not resolve) every waiter registered against epoch 1.
    reach_running_at_epoch(locality, target, 2);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed, 2);

    HPX_TEST_EQ(results.size(), static_cast<std::size_t>(num_waiters));
    for (auto& f : results)
    {
        bool caught_stale_state = false;
        try
        {
            f.get();
            HPX_TEST(false);
        }
        catch (hpx::exception const& e)
        {
            caught_stale_state = (e.get_error() == hpx::error::stale_state);
        }
        HPX_TEST(caught_stale_state);
    }
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

int hpx_main()
{
    for (auto const& locality : hpx::find_all_localities())
    {
        HPX_SUPERVISION_TEST_RUN(test_await_terminal_fast_path, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_await_terminal_registers_then_resolves, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_await_terminal_epoch_mismatch_never_resolves, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_await_terminal_concurrent_publish_stress, locality);

        HPX_SUPERVISION_TEST_RUN(
            test_await_terminal_default_timeout_not_expired_early, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_await_terminal_abandoned_waiter_expires, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_await_terminal_explicit_timeout_overrides_default, locality);

        HPX_SUPERVISION_TEST_RUN(
            test_await_terminal_concurrent_epoch_bump_resolves_waiters,
            locality);
        HPX_SUPERVISION_TEST_RUN(
            test_await_terminal_concurrent_epoch_skip_invalidates_waiters,
            locality);
    }

    HPX_SUPERVISION_TEST_RUN(
        test_await_terminal_cross_locality_epoch_bump_abandons_remote_waiter);

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
