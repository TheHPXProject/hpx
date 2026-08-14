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

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

// ============================================================================
// Compile-time checks: none of the register_activity_observer() /
// unregister_activity_observer() overloads take a `target` parameter.
// Unlike register_observer()/unregister_observer(), activity observers are
// scoped to a locality's supervision manager as a whole, not to an
// individual target.
// ============================================================================
namespace {

    template <typename... Args>
    concept invocable_register_activity_observer = requires(Args&&... args) {
        hpx::supervision::register_activity_observer(
            std::forward<Args>(args)...);
    };

    template <typename... Args>
    concept invocable_unregister_activity_observer = requires(Args&&... args) {
        hpx::supervision::unregister_activity_observer(
            std::forward<Args>(args)...);
    };

    // The legitimate, locality-scoped overloads remain callable.
    static_assert(invocable_register_activity_observer<hpx::id_type const&,
        hpx::supervision::activity_callback const&>);
    static_assert(invocable_register_activity_observer<hpx::launch::sync_policy,
        hpx::id_type const&, hpx::supervision::activity_callback const&>);
    static_assert(invocable_register_activity_observer<
        hpx::supervision::activity_callback const&>);

    static_assert(invocable_unregister_activity_observer<hpx::id_type const&,
        hpx::id_type const&>);
    static_assert(
        invocable_unregister_activity_observer<hpx::launch::sync_policy,
            hpx::id_type const&, hpx::id_type const&>);
    static_assert(invocable_unregister_activity_observer<hpx::id_type const&>);

    // None of the overloads accept a target argument: mirroring
    // register_observer()'s/unregister_observer()'s (locality, target, ...)
    // signatures against register_activity_observer()/
    // unregister_activity_observer() must not resolve to any overload.
    static_assert(!invocable_register_activity_observer<hpx::id_type const&,
        hpx::id_type const&, hpx::supervision::activity_callback const&,
        std::optional<std::uint64_t>>);
    static_assert(!invocable_register_activity_observer<hpx::id_type const&,
        hpx::id_type const&, hpx::supervision::activity_callback const&>);
    static_assert(
        !invocable_register_activity_observer<hpx::launch::sync_policy,
            hpx::id_type const&, hpx::id_type const&,
            hpx::supervision::activity_callback const&>);

    static_assert(!invocable_unregister_activity_observer<hpx::id_type const&,
        hpx::id_type const&, hpx::id_type const&>);
    static_assert(
        !invocable_unregister_activity_observer<hpx::launch::sync_policy,
            hpx::id_type const&, hpx::id_type const&, hpx::id_type const&>);

}    // namespace

// ============================================================================
// Test Infrastructure
// ============================================================================

// Records every activity_notification delivered to a single registered activity
// observer, and allows querying/waiting for notifications pertaining to a
// specific target. Since an activity observer sees transitions for *every*
// target tracked by a locality's supervision manager - including targets
// created by earlier, unrelated tests running in the same process - assertions
// must always be scoped to the target(s) a given test created, never to the
// total notification count.
struct activity_test_context
{
    mutable hpx::mutex mtx;
    hpx::condition_variable cv;
    std::vector<hpx::supervision::activity_notification> observed;

    void reset()
    {
        std::scoped_lock<hpx::mutex> lock(mtx);
        observed.clear();
    }

    void record(hpx::supervision::activity_notification const& notification)
    {
        {
            std::scoped_lock<hpx::mutex> lock(mtx);
            observed.push_back(notification);
        }
        cv.notify_all();
    }

    std::size_t count_for_target(hpx::id_type const& target) const
    {
        std::scoped_lock<hpx::mutex> lock(mtx);
        return static_cast<std::size_t>(std::ranges::count_if(
            observed, [&target](auto const& n) { return n.actor == target; }));
    }

    // Returns the recorded notifications for `target`, in delivery order.
    std::vector<hpx::supervision::activity_notification> for_target(
        hpx::id_type const& target) const
    {
        std::scoped_lock<hpx::mutex> lock(mtx);
        std::vector<hpx::supervision::activity_notification> result;
        std::ranges::copy_if(observed, std::back_inserter(result),
            [&target](auto const& n) { return n.actor == target; });
        return result;
    }

    bool wait_for_target_count(hpx::id_type const& target,
        std::size_t const expected, std::chrono::milliseconds const timeout)
    {
        std::unique_lock<hpx::mutex> lock(mtx);
        return cv.wait_for(lock, timeout, [&] {
            return static_cast<std::size_t>(std::ranges::count_if(
                       observed, [&target](auto const& n) {
                           return n.actor == target;
                       })) >= expected;
        });
    }
};

namespace {

    hpx::supervision::activity_callback make_recording_callback(
        activity_test_context& ctx)
    {
        return
            [&ctx](
                hpx::supervision::activity_notification const& notification) {
                ctx.record(notification);
                return true;
            };
    }

    bool no_op_lifecycle_observer(
        hpx::supervision::lifecycle_event_notification const&)
    {
        return true;
    }

}    // namespace

// ============================================================================
// Test Cases: Replay on registration
// ============================================================================

// A target already active - via either trigger - at the time an activity
// observer is registered must be replayed synchronously as
// activity_transition::already_active, as part of registration.
void test_activity_replay_on_registration(hpx::id_type const& locality)
{
    hpx::id_type const target_via_event = make_test_target();
    hpx::id_type const target_via_observer = make_test_target();

    // Activate one target by publishing an event, and the other by registering
    // a per-target lifecycle observer, before any activity observer exists.
    hpx::supervision::publish_event(hpx::launch::sync, locality,
        target_via_event, hpx::supervision::event::started);

    auto const lifecycle_handle =
        hpx::supervision::register_observer(hpx::launch::sync, locality,
            target_via_observer, no_op_lifecycle_observer);

    activity_test_context ctx;
    auto const activity_handle = hpx::supervision::register_activity_observer(
        hpx::launch::sync, locality, make_recording_callback(ctx));

    HPX_TEST(ctx.wait_for_target_count(
        target_via_event, 1, std::chrono::milliseconds(50)));
    HPX_TEST(ctx.wait_for_target_count(
        target_via_observer, 1, std::chrono::milliseconds(50)));

    auto const events_replay = ctx.for_target(target_via_event);
    HPX_TEST_EQ(events_replay.size(), static_cast<std::size_t>(1));
    if (!events_replay.empty())
    {
        HPX_TEST(
            events_replay[0].state == hpx::supervision::activity_state::active);
        HPX_TEST(events_replay[0].transition ==
            hpx::supervision::activity_transition::already_active);
    }

    auto const observer_replay = ctx.for_target(target_via_observer);
    HPX_TEST_EQ(observer_replay.size(), static_cast<std::size_t>(1));
    if (!observer_replay.empty())
    {
        HPX_TEST(observer_replay[0].state ==
            hpx::supervision::activity_state::active);
        HPX_TEST(observer_replay[0].transition ==
            hpx::supervision::activity_transition::already_active);
    }

    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, activity_handle);
    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, lifecycle_handle);
}

// ============================================================================
// Test Cases: Live transitions
// ============================================================================

// A target's very first publish_event() call triggers
// activity_transition::first_event for an already-registered activity observer.
void test_activity_live_first_event(hpx::id_type const& locality)
{
    activity_test_context ctx;
    auto const activity_handle = hpx::supervision::register_activity_observer(
        hpx::launch::sync, locality, make_recording_callback(ctx));

    hpx::id_type const target = make_test_target();
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    HPX_TEST(
        ctx.wait_for_target_count(target, 1, std::chrono::milliseconds(50)));

    auto const notifications = ctx.for_target(target);
    HPX_TEST_EQ(notifications.size(), static_cast<std::size_t>(1));
    if (!notifications.empty())
    {
        HPX_TEST(
            notifications[0].state == hpx::supervision::activity_state::active);
        HPX_TEST(notifications[0].transition ==
            hpx::supervision::activity_transition::first_event);
    }

    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, activity_handle);
}

// Registering the first per-target lifecycle observer for a target that has
// never published an event triggers activity_transition::first_observer.
void test_activity_live_first_observer(hpx::id_type const& locality)
{
    activity_test_context ctx;
    auto const activity_handle = hpx::supervision::register_activity_observer(
        hpx::launch::sync, locality, make_recording_callback(ctx));

    hpx::id_type const target = make_test_target();
    auto const lifecycle_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target, no_op_lifecycle_observer);

    HPX_TEST(
        ctx.wait_for_target_count(target, 1, std::chrono::milliseconds(50)));

    auto const notifications = ctx.for_target(target);
    HPX_TEST_EQ(notifications.size(), static_cast<std::size_t>(1));
    if (!notifications.empty())
    {
        HPX_TEST(
            notifications[0].state == hpx::supervision::activity_state::active);
        HPX_TEST(notifications[0].transition ==
            hpx::supervision::activity_transition::first_observer);
    }

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, lifecycle_handle);
    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, activity_handle);
}

// Unregistering a target's last remaining per-target lifecycle observer
// triggers activity_transition::last_observer_unregistered.
void test_activity_live_last_observer_unregistered(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    auto const lifecycle_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target, no_op_lifecycle_observer);

    activity_test_context ctx;
    auto const activity_handle = hpx::supervision::register_activity_observer(
        hpx::launch::sync, locality, make_recording_callback(ctx));

    // The registration above replays target's current (already active) state;
    // that replay is not what this test is exercising.
    HPX_TEST(
        ctx.wait_for_target_count(target, 1, std::chrono::milliseconds(50)));
    ctx.reset();

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, lifecycle_handle);

    HPX_TEST(
        ctx.wait_for_target_count(target, 1, std::chrono::milliseconds(50)));

    auto const notifications = ctx.for_target(target);
    HPX_TEST_EQ(notifications.size(), static_cast<std::size_t>(1));
    if (!notifications.empty())
    {
        HPX_TEST(notifications[0].state ==
            hpx::supervision::activity_state::inactive);
        HPX_TEST(notifications[0].transition ==
            hpx::supervision::activity_transition::last_observer_unregistered);
    }

    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, activity_handle);
}

// A target's latched activity state is not cleared by later publications: only
// the very first publish_event() for a target ever produces an activity
// notification; subsequent events (including a terminal one) must not trigger a
// spurious deactivation on the publishing path.
void test_activity_no_deactivation_on_publish_path(hpx::id_type const& locality)
{
    activity_test_context ctx;
    auto const activity_handle = hpx::supervision::register_activity_observer(
        hpx::launch::sync, locality, make_recording_callback(ctx));

    hpx::id_type const target = make_test_target();
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    HPX_TEST(
        ctx.wait_for_target_count(target, 1, std::chrono::milliseconds(50)));

    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::running);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    // Publish a sentinel event for an unrelated target and wait for its
    // notification to arrive. Since activity notifications are delivered in
    // publication order, this guarantees that any (unexpected) extra
    // notification for `target` above would already have been recorded by
    // the time the sentinel's notification is observed.
    hpx::id_type const sentinel_target = make_test_target();
    hpx::supervision::publish_event(hpx::launch::sync, locality,
        sentinel_target, hpx::supervision::event::started);
    HPX_TEST(ctx.wait_for_target_count(
        sentinel_target, 1, std::chrono::milliseconds(1000)));

    auto const notifications = ctx.for_target(target);
    HPX_TEST_EQ(notifications.size(), static_cast<std::size_t>(1));
    if (!notifications.empty())
    {
        HPX_TEST(notifications[0].transition ==
            hpx::supervision::activity_transition::first_event);
    }

    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, activity_handle);
}

// A single activity-observer registration must correctly attribute transitions
// across multiple, independently-triggered targets.
void test_activity_multiple_targets_single_registration(
    hpx::id_type const& locality)
{
    activity_test_context ctx;
    auto const activity_handle = hpx::supervision::register_activity_observer(
        hpx::launch::sync, locality, make_recording_callback(ctx));

    hpx::id_type const target_event_1 = make_test_target();
    hpx::id_type const target_observer = make_test_target();
    hpx::id_type const target_event_2 = make_test_target();

    hpx::supervision::publish_event(hpx::launch::sync, locality, target_event_1,
        hpx::supervision::event::started);
    auto const lifecycle_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target_observer, no_op_lifecycle_observer);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target_event_2,
        hpx::supervision::event::started);

    HPX_TEST(ctx.wait_for_target_count(
        target_event_1, 1, std::chrono::milliseconds(50)));
    HPX_TEST(ctx.wait_for_target_count(
        target_observer, 1, std::chrono::milliseconds(50)));
    HPX_TEST(ctx.wait_for_target_count(
        target_event_2, 1, std::chrono::milliseconds(50)));

    auto notifications = ctx.for_target(target_event_1);
    HPX_TEST_EQ(notifications.size(), static_cast<std::size_t>(1));
    if (!notifications.empty())
    {
        HPX_TEST(notifications[0].transition ==
            hpx::supervision::activity_transition::first_event);
    }

    notifications = ctx.for_target(target_observer);
    HPX_TEST_EQ(notifications.size(), static_cast<std::size_t>(1));
    if (!notifications.empty())
    {
        HPX_TEST(notifications[0].transition ==
            hpx::supervision::activity_transition::first_observer);
    }

    notifications = ctx.for_target(target_event_2);
    HPX_TEST_EQ(notifications.size(), static_cast<std::size_t>(1));
    if (!notifications.empty())
    {
        HPX_TEST(notifications[0].transition ==
            hpx::supervision::activity_transition::first_event);
    }

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, lifecycle_handle);
    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, activity_handle);
}

// ============================================================================
// Test Cases: epoch_filter
// ============================================================================

// A registration-time replay only covers targets whose (current) epoch matches
// epoch_filter, when one is engaged.
void test_activity_epoch_filter_replay(hpx::id_type const& locality)
{
    constexpr std::uint64_t filter_epoch = 11;
    constexpr std::uint64_t other_epoch = 12;

    hpx::id_type const target_match = make_test_target();
    hpx::id_type const target_mismatch = make_test_target();

    hpx::supervision::publish_event(hpx::launch::sync, locality, target_match,
        hpx::supervision::event::started, filter_epoch);
    hpx::supervision::publish_event(hpx::launch::sync, locality,
        target_mismatch, hpx::supervision::event::started, other_epoch);

    activity_test_context ctx;
    auto const activity_handle =
        hpx::supervision::register_activity_observer(hpx::launch::sync,
            locality, make_recording_callback(ctx), filter_epoch);

    HPX_TEST(ctx.wait_for_target_count(
        target_match, 1, std::chrono::milliseconds(50)));

    // Give a would-be (incorrect) replay for the mismatched target a chance
    // to arrive before asserting its absence.
    hpx::this_thread::sleep_for(std::chrono::milliseconds(50));
    HPX_TEST_EQ(
        ctx.count_for_target(target_mismatch), static_cast<std::size_t>(0));

    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, activity_handle);
}

// A live transition recorded under an epoch other than epoch_filter must be
// silently skipped for a filtered observer, while a matching-epoch transition
// still reaches it.
void test_activity_epoch_filter_live(hpx::id_type const& locality)
{
    constexpr std::uint64_t filter_epoch = 21;
    constexpr std::uint64_t other_epoch = 22;

    activity_test_context ctx;
    auto const activity_handle =
        hpx::supervision::register_activity_observer(hpx::launch::sync,
            locality, make_recording_callback(ctx), filter_epoch);

    hpx::id_type const target_match = make_test_target();
    hpx::id_type const target_mismatch = make_test_target();

    hpx::supervision::publish_event(hpx::launch::sync, locality, target_match,
        hpx::supervision::event::started, filter_epoch);
    hpx::supervision::publish_event(hpx::launch::sync, locality,
        target_mismatch, hpx::supervision::event::started, other_epoch);

    // Publish a fence event under the matching epoch and wait for its
    // notification to arrive. Activity notifications are delivered in
    // publication order, so this guarantees any (unexpected) notification
    // for `target_mismatch` would already be recorded by the time the
    // fence's notification is observed.
    hpx::id_type const fence_target = make_test_target();
    hpx::supervision::publish_event(hpx::launch::sync, locality, fence_target,
        hpx::supervision::event::started, filter_epoch);
    HPX_TEST(ctx.wait_for_target_count(
        fence_target, 1, std::chrono::milliseconds(1000)));

    hpx::this_thread::sleep_for(std::chrono::milliseconds(50));
    HPX_TEST_EQ(
        ctx.count_for_target(target_mismatch), static_cast<std::size_t>(0));

    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, activity_handle);
}

// ============================================================================
// Test Cases: Registration race
// ============================================================================

// Registering an activity observer concurrently with in-flight
// publish_event()/register_observer() calls for a batch of targets must deliver
// exactly one notification per target - either a live transition or a
// registration-time replay - never both, never neither.
void test_activity_registration_race(hpx::id_type const& locality)
{
    constexpr std::size_t num_targets = 20;

    std::vector<hpx::id_type> targets;
    targets.reserve(num_targets);
    for (std::size_t i = 0; i != num_targets; ++i)
    {
        targets.push_back(make_test_target());
    }

    hpx::mutex handles_mtx;
    std::vector<hpx::id_type> lifecycle_handles;

    std::vector<hpx::future<void>> activation_futures;
    activation_futures.reserve(num_targets);
    for (std::size_t i = 0; i != num_targets; ++i)
    {
        hpx::id_type const& target = targets[i];
        if (i % 2 == 0)
        {
            activation_futures.push_back(
                hpx::async(hpx::launch::task, [locality, target]() {
                    hpx::supervision::publish_event(hpx::launch::sync, locality,
                        target, hpx::supervision::event::started);
                }));
        }
        else
        {
            activation_futures.push_back(hpx::async(hpx::launch::task,
                [locality, target, &handles_mtx, &lifecycle_handles]() {
                    auto const handle =
                        hpx::supervision::register_observer(hpx::launch::sync,
                            locality, target, no_op_lifecycle_observer);
                    std::scoped_lock<hpx::mutex> lock(handles_mtx);
                    lifecycle_handles.push_back(handle);
                }));
        }
    }

    activity_test_context ctx;
    auto activity_handle_future =
        hpx::async(hpx::launch::task, [locality, &ctx]() {
            return hpx::supervision::register_activity_observer(
                hpx::launch::sync, locality, make_recording_callback(ctx));
        });

    hpx::wait_all(activation_futures);
    hpx::id_type const activity_handle = activity_handle_future.get();

    for (hpx::id_type const& target : targets)
    {
        HPX_TEST(ctx.wait_for_target_count(
            target, 1, std::chrono::milliseconds(50)));
        HPX_TEST_EQ(ctx.count_for_target(target), static_cast<std::size_t>(1));
    }

    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, activity_handle);

    for (hpx::id_type const& handle : lifecycle_handles)
    {
        hpx::supervision::unregister_observer(
            hpx::launch::sync, locality, handle);
    }
}

// ============================================================================
// Test Cases: Cross-API and unknown handle rejection
// ============================================================================

// A handle returned by register_activity_observer() must be rejected by
// unregister_observer(): the two registration APIs use distinct handle
// namespaces, and passing a foreign handle must not be silently ignored.
void test_unregister_observer_rejects_activity_observer_handle(
    hpx::id_type const& locality)
{
    activity_test_context ctx;
    auto const activity_handle = hpx::supervision::register_activity_observer(
        hpx::launch::sync, locality, make_recording_callback(ctx));

    hpx::error_code ec;
    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, activity_handle, ec);
    HPX_TEST(ec);
    HPX_TEST(ec.value() == hpx::error::bad_parameter);

    // Clean up the still-valid handle via the correct API.
    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, activity_handle);
}

// A handle returned by register_observer() must be rejected by
// unregister_activity_observer(), symmetrically to the case above.
void test_unregister_activity_observer_rejects_target_observer_handle(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    auto const lifecycle_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target, no_op_lifecycle_observer);

    hpx::error_code ec;
    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, lifecycle_handle, ec);
    HPX_TEST(ec);
    HPX_TEST(ec.value() == hpx::error::bad_parameter);

    // Clean up the still-valid handle via the correct API.
    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, lifecycle_handle);
}

// A handle that was never returned by either registration API must be rejected
// by unregister_observer() rather than silently falling through to a no-op.
void test_unregister_observer_rejects_unknown_handle(
    hpx::id_type const& locality)
{
    hpx::error_code ec;
    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, make_test_target(), ec);
    HPX_TEST(ec);
    HPX_TEST(ec.value() == hpx::error::bad_parameter);
}

// Same as above, for unregister_activity_observer().
void test_unregister_activity_observer_rejects_unknown_handle(
    hpx::id_type const& locality)
{
    hpx::error_code ec;
    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, make_test_target(), ec);
    HPX_TEST(ec);
    HPX_TEST(ec.value() == hpx::error::bad_parameter);
}

// Regression guard: a handle that does belong to unregister_observer()'s own
// namespace must continue to be accepted once the checks above are in place.
void test_unregister_observer_accepts_own_handle(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    auto const lifecycle_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target, no_op_lifecycle_observer);

    hpx::error_code ec;
    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, lifecycle_handle, ec);
    HPX_TEST(!ec);
}

// Regression guard: symmetric to the above, for unregister_activity_observer().
void test_unregister_activity_observer_accepts_own_handle(
    hpx::id_type const& locality)
{
    activity_test_context ctx;
    auto const activity_handle = hpx::supervision::register_activity_observer(
        hpx::launch::sync, locality, make_recording_callback(ctx));

    hpx::error_code ec;
    hpx::supervision::unregister_activity_observer(
        hpx::launch::sync, locality, activity_handle, ec);
    HPX_TEST(!ec);
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

int hpx_main()
{
    for (auto const& locality : hpx::find_all_localities())
    {
        HPX_SUPERVISION_TEST_RUN(
            test_activity_replay_on_registration, locality);

        HPX_SUPERVISION_TEST_RUN(test_activity_live_first_event, locality);
        HPX_SUPERVISION_TEST_RUN(test_activity_live_first_observer, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_activity_live_last_observer_unregistered, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_activity_no_deactivation_on_publish_path, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_activity_multiple_targets_single_registration, locality);

        HPX_SUPERVISION_TEST_RUN(test_activity_epoch_filter_replay, locality);
        HPX_SUPERVISION_TEST_RUN(test_activity_epoch_filter_live, locality);

        HPX_SUPERVISION_TEST_RUN(test_activity_registration_race, locality);

        HPX_SUPERVISION_TEST_RUN(
            test_unregister_observer_rejects_activity_observer_handle,
            locality);
        HPX_SUPERVISION_TEST_RUN(
            test_unregister_activity_observer_rejects_target_observer_handle,
            locality);
        HPX_SUPERVISION_TEST_RUN(
            test_unregister_observer_rejects_unknown_handle, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_unregister_activity_observer_rejects_unknown_handle, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_unregister_observer_accepts_own_handle, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_unregister_activity_observer_accepts_own_handle, locality);
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
