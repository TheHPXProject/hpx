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

#include <atomic>

#include "supervision_test_helpers.hpp"

// ============================================================================
// Test Cases: remove_target() state clearing
// ============================================================================

// remove_target() must erase any recorded lifecycle state for the target: after
// removal, query_state() must report the same "never seen this target" result
// it reports for a target that never published anything.
void test_remove_target_clears_published_state(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running(locality, target);

    auto const before =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(before.last_event == hpx::supervision::event::running);
    HPX_TEST(!before.ec);

    hpx::supervision::remove_target(hpx::launch::sync, locality, target);

    auto const after =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(after.last_event == hpx::supervision::event::unknown);
    HPX_TEST(after.ec);
    HPX_TEST(after.ec.value() == hpx::error::stale_state);
}

// remove_target() must also drop any observers still registered for the target:
// once removed, further publications must not invoke callbacks that were
// registered for it.
void test_remove_target_stops_observer_callbacks(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    std::atomic<int> callback_count{0};
    hpx::supervision::register_observer(hpx::launch::sync, locality, target,
        [&](hpx::supervision::lifecycle_event_notification const&) {
            callback_count.fetch_add(1);
            return true;
        });

    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);
    HPX_TEST_EQ(callback_count.load(), 1);

    hpx::supervision::remove_target(hpx::launch::sync, locality, target);

    // The observer entry for `target` is gone: a later publication for the same
    // target must not reach the callback above.
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);
    HPX_TEST_EQ(callback_count.load(), 1);
}

// A target with no recorded state and no observers has nothing to remove;
// remove_target() must be a safe no-op rather than throwing.
void test_remove_target_unknown_target_is_noop(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::remove_target(hpx::launch::sync, locality, target);

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::unknown);
    HPX_TEST(state.ec);
    HPX_TEST(state.ec.value() == hpx::error::stale_state);
}

// Once a target's state has been removed, its id is free to be reused as if it
// had never published anything: a fresh `started` event must be accepted even
// though this same id previously reached a terminal `completed` state (which
// would otherwise make `started` an illegal transition).
void test_remove_target_allows_republish_after_removal(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::running);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    hpx::supervision::remove_target(hpx::launch::sync, locality, target);

    hpx::error_code ec;
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, 0, ec);
    HPX_TEST(!ec);

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::started);
    HPX_TEST_EQ(state.event_sequence_number, 1u);
}

// Regression test: calling remove_target() on `target` from within one of its
// own observer callbacks must not deadlock, mirroring
// test_unregister_observer_from_within_callback(). Since fire_event() only
// holds the manager's internal lock briefly (released before the agent callback
// runs), remove_target() re-acquiring that lock from within the callback must
// be safe.
void test_remove_target_from_within_observer_callback(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    std::atomic<int> invocation_count{0};
    hpx::supervision::register_observer(hpx::launch::sync, locality, target,
        [&](hpx::supervision::lifecycle_event_notification const&) {
            invocation_count.fetch_add(1);
            hpx::supervision::remove_target(
                hpx::launch::sync, locality, target);
            return true;
        });

    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);
    HPX_TEST_EQ(invocation_count.load(), 1);

    // The target's state and its (former) observer are both gone now: a later
    // publication must be treated as this target's first-ever event and must
    // not reach the callback above again.
    hpx::error_code ec;
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, 0, ec);
    HPX_TEST(!ec);
    HPX_TEST_EQ(invocation_count.load(), 1);
}

// The purely local overload (no locality argument) must behave the same way for
// a target whose supervision manager lives on the calling locality.
void test_remove_target_local()
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(target, hpx::supervision::event::started);
    hpx::supervision::publish_event(target, hpx::supervision::event::running);

    hpx::supervision::remove_target(target);

    auto const state = hpx::supervision::query_state(target);
    HPX_TEST(state.last_event == hpx::supervision::event::unknown);
    HPX_TEST(state.ec);
    HPX_TEST(state.ec.value() == hpx::error::stale_state);
}

// The asynchronous, future-returning overload must also clear state.
void test_remove_target_async(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    hpx::supervision::remove_target(locality, target).get();

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::unknown);
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

int hpx_main()
{
    for (auto const& locality : hpx::find_all_localities())
    {
        HPX_SUPERVISION_TEST_RUN(
            test_remove_target_clears_published_state, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_remove_target_stops_observer_callbacks, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_remove_target_unknown_target_is_noop, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_remove_target_allows_republish_after_removal, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_remove_target_from_within_observer_callback, locality);
        HPX_SUPERVISION_TEST_RUN(test_remove_target_async, locality);
    }

    HPX_SUPERVISION_TEST_RUN(test_remove_target_local);

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
