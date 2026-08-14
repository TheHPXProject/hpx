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
#include <iostream>
#include <mutex>
#include <optional>
#include <vector>

#include "supervision_test_helpers.hpp"

// ============================================================================
// Test Infrastructure
// ============================================================================

struct test_context
{
    hpx::mutex mtx;
    hpx::condition_variable cv;
    std::vector<hpx::supervision::event> observed_events;
    std::vector<hpx::error_code> observed_errors;
    std::atomic<int> callback_count{0};
    std::atomic<bool> callback_received{false};

    void reset()
    {
        std::scoped_lock<hpx::mutex> lock(mtx);
        observed_events.clear();
        observed_errors.clear();
        callback_count.store(0);
        callback_received.store(false);
    }

    void record_event(hpx::id_type const&, hpx::supervision::event const event,
        hpx::error_code const& ec)
    {
        {
            std::scoped_lock<hpx::mutex> lock(mtx);
            observed_events.push_back(event);
            observed_errors.push_back(ec);
        }
        callback_count.fetch_add(1);
        callback_received.store(true);
        cv.notify_all();
    }

    bool wait_for_callback(std::chrono::milliseconds const timeout)
    {
        std::unique_lock<hpx::mutex> lock(mtx);
        return cv.wait_for(
            lock, timeout, [this] { return callback_received.load(); });
    }

    int get_callback_count() const
    {
        return callback_count.load();
    }

    std::vector<hpx::supervision::event> const& get_events() const
    {
        return observed_events;
    }

    std::vector<hpx::error_code> const& get_errors() const
    {
        return observed_errors;
    }
};

// Global test context
static test_context g_test_context;

// ============================================================================
// Test Cases: 2B.1 - Connector Publishes Explicit Completion
// ============================================================================
void test_publish_completion()
{
    hpx::id_type const locality = hpx::find_here();
    hpx::id_type const target = make_test_target();

    reach_running(locality, target);

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    // Query the state immediately (remote API)
    auto state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);

    HPX_TEST_EQ(state.actor, target);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);
    HPX_TEST(state.timestamp != std::chrono::steady_clock::time_point{});
    HPX_TEST_NEQ(state.event_sequence_number, 0u);

    // Query the state immediately (locally)
    state = hpx::supervision::query_state(target);

    HPX_TEST_EQ(state.actor, target);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);
    HPX_TEST(state.timestamp != std::chrono::steady_clock::time_point{});
    HPX_TEST_NEQ(state.event_sequence_number, 0u);
}

void test_publish_completion_async(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running(locality, target);

    auto const future = hpx::supervision::publish_event(
        locality, target, hpx::supervision::event::completed);
    future.wait();

    // Verify the state was recorded (remote API)
    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);
}

void test_publish_failed_state(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    // `failed` is reachable from `started` directly.
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::failed);

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::failed);
}

// ============================================================================
// Test Cases: 2B.2 - Root Observes Completion Without Polling
// ============================================================================
void test_register_observer_local_completion(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    HPX_TEST_NEQ(observer_handle, hpx::invalid_id);

    // Reach `completed` via a legal path; do this before resetting the test
    // context so these preliminary events don't count towards the
    // callback_count assertion below.
    reach_running(locality, target);

    g_test_context.reset();

    // Publish completion event
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    // Callback should fire within 10ms
    bool const received =
        g_test_context.wait_for_callback(std::chrono::milliseconds(10));

    HPX_TEST_MSG(received, "Callback not received within 10ms");
    HPX_TEST_EQ(g_test_context.get_callback_count(), 1);
    HPX_TEST(
        g_test_context.get_events()[0] == hpx::supervision::event::completed);
    HPX_TEST(g_test_context.get_errors()[0] == hpx::make_success_code());

    // Cleanup
    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

void test_register_observer_multiple_events(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    g_test_context.reset();

    // Publish multiple events
    reach_running(locality, target);

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    // Wait for all callbacks
    hpx::this_thread::sleep_for(std::chrono::milliseconds(10));

    HPX_TEST_EQ(g_test_context.get_callback_count(), 3);

    auto const& events = g_test_context.get_events();
    HPX_TEST(events[0] == hpx::supervision::event::started);
    HPX_TEST(events[1] == hpx::supervision::event::running);
    HPX_TEST(events[2] == hpx::supervision::event::completed);

    auto const& errors = g_test_context.get_errors();
    HPX_TEST(errors[0] == hpx::make_success_code());
    HPX_TEST(errors[1] == hpx::make_success_code());
    HPX_TEST(errors[2] == hpx::make_success_code());

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

void test_register_observer(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    auto observer_future = hpx::supervision::register_observer(locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    auto const observer_handle = observer_future.get();
    HPX_TEST_NEQ(observer_handle, hpx::invalid_id);

    reach_running(locality, target);

    g_test_context.reset();

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    bool const received =
        g_test_context.wait_for_callback(std::chrono::milliseconds(10));
    HPX_TEST(received);

    HPX_TEST_EQ(g_test_context.get_callback_count(), 1);

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

void test_register_observer_receives_existing_state(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    // Establish state before any observer exists.
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);
    auto const expected =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);

    hpx::spinlock mtx;
    std::vector<hpx::supervision::lifecycle_event_notification> received;

    auto const observer_handle =
        hpx::supervision::register_observer(hpx::launch::sync, locality, target,
            [&](hpx::supervision::lifecycle_event_notification const&
                    notification) {
                std::scoped_lock<hpx::spinlock> l(mtx);
                received.push_back(notification);
                return true;
            });

    // register_observer synchronously waits for its initial delivery.
    {
        std::scoped_lock<hpx::spinlock> l(mtx);
        HPX_TEST_EQ(received.size(), static_cast<std::size_t>(1));

        auto const& [actor, event, event_time, event_sequence_number, epoch,
            ec] = received.front();
        HPX_TEST_EQ(actor, target);
        HPX_TEST(event == hpx::supervision::event::started);
        HPX_TEST_EQ(event_sequence_number, expected.event_sequence_number);
        HPX_TEST(event_time == expected.timestamp);
        HPX_TEST(ec == expected.ec);
    }

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

void test_register_observer_keeps_initial_state_snapshot()
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(target, hpx::supervision::event::started);

    auto const initial_state = hpx::supervision::query_state(target);

    std::atomic<bool> snapshot_taken{false};
    std::atomic<bool> continue_registration{false};
    hpx::spinlock received_mtx;
    std::vector<hpx::supervision::lifecycle_event_notification> received;

    hpx::supervision::testing::set_register_observer_snapshot_hook([&] {
        snapshot_taken.store(true);
        while (!continue_registration.load())
        {
            hpx::this_thread::yield();
        }
    });

    auto clear_hook = hpx::experimental::scope_exit([] {
        hpx::supervision::testing::set_register_observer_snapshot_hook({});
    });

    auto registration = hpx::async([&] {
        return hpx::supervision::register_observer(target,
            [&](hpx::supervision::lifecycle_event_notification const& n) {
                std::scoped_lock<hpx::spinlock> l(received_mtx);
                received.push_back(n);
                return true;
            });
    });

    while (!snapshot_taken.load())
    {
        hpx::this_thread::yield();
    }

    // This replaces the manager's current state while registration is paused.
    hpx::supervision::publish_event(target, hpx::supervision::event::running);

    continue_registration.store(true);
    auto const observer_handle = registration.get();

    {
        std::scoped_lock<hpx::spinlock> l(received_mtx);
        HPX_TEST_EQ(received.size(), static_cast<std::size_t>(2));

        bool saw_initial_snapshot = false;
        bool saw_running_publication = false;

        for (auto const& notification : received)
        {
            if (notification.event == hpx::supervision::event::started &&
                notification.event_sequence_number ==
                    initial_state.event_sequence_number)
            {
                saw_initial_snapshot = true;
            }

            if (notification.event == hpx::supervision::event::running &&
                notification.event_sequence_number ==
                    initial_state.event_sequence_number + 1)
            {
                saw_running_publication = true;
            }
        }

        HPX_TEST(saw_initial_snapshot);
        HPX_TEST(saw_running_publication);
    }

    hpx::supervision::unregister_observer(observer_handle);
}

// ============================================================================
// Test Cases: 2B.3 - Root Observes Failure Without External Witness
// ============================================================================
void test_observe_failure_detection(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    // `failed` is reachable directly from `started`.
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    g_test_context.reset();

    // Publish a failure event
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::failed);

    bool const received =
        g_test_context.wait_for_callback(std::chrono::milliseconds(10));
    HPX_TEST(received);

    HPX_TEST_EQ(g_test_context.get_callback_count(), 1);
    HPX_TEST(g_test_context.get_events()[0] == hpx::supervision::event::failed);
    HPX_TEST(g_test_context.get_errors()[0] == hpx::make_success_code());

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

// ============================================================================
// Test Cases: 2B.4 - Sequence Numbers & Lost Connector Detection
// ============================================================================
void test_sequence_numbers_monotonic(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    auto const state1 =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    auto const state2 =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::running);

    auto const state3 =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);

    HPX_TEST_LT(state1.event_sequence_number, state2.event_sequence_number);
    HPX_TEST_LT(state2.event_sequence_number, state3.event_sequence_number);
}

void test_sequence_numbers_no_gaps(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    // Establish the required first event before running the repeated
    // `running` publications below (running -> running is a legal self
    // transition, used here purely to exercise sequence numbers).
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    std::vector<uint64_t> sequence_numbers;
    auto const observer_handle =
        hpx::supervision::register_observer(hpx::launch::sync, locality, target,
            [&sequence_numbers, locality](
                hpx::supervision::lifecycle_event_notification const&
                    notification) {
                // query state during callback
                auto const state = hpx::supervision::query_state(
                    hpx::launch::sync, locality, notification.actor);
                sequence_numbers.push_back(state.event_sequence_number);

                // sequence numbers must match
                HPX_TEST(notification.event_sequence_number ==
                    state.event_sequence_number);

                g_test_context.record_event(
                    notification.actor, notification.event, notification.ec);

                return true;
            });

    g_test_context.reset();

    // Publish 5 events
    for (int i = 0; i < 5; ++i)
    {
        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::running);
    }

    hpx::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Verify sequence numbers are consecutive or increasing
    for (size_t i = 1; i < sequence_numbers.size(); ++i)
    {
        HPX_TEST_LTE(sequence_numbers[i - 1], sequence_numbers[i]);
    }

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

void test_detect_connector_terminal(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running(locality, target);

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& n) -> bool {
            g_test_context.record_event(n.actor, n.event, n.ec);
            HPX_THROW_EXCEPTION(hpx::error::no_success,
                "test_detect_connector_terminal", "testing error reporting");
        });

    g_test_context.reset();

    // Simulate connector reaching terminal state
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Query to confirm failed state
    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::failed);

    // Verify error was reported back correctly
    HPX_TEST(state.ec.value() == hpx::error::no_success);

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

void test_error_does_not_stop_callbacks(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& n) -> bool {
            g_test_context.record_event(n.actor, n.event, n.ec);
            HPX_THROW_EXCEPTION(hpx::error::no_success,
                "test_detect_connector_terminal", "testing error reporting");
        });

    auto const observer_handle2 = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    // Reset before publishing so that wait_for_callback() and
    // get_callback_count() below only account for callbacks triggered by
    // this test, not any residual state left behind by earlier tests
    // sharing the global g_test_context.
    g_test_context.reset();

    // `started` raises an exception in one of the observers, causing the
    // state to shift to `failure`.
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    bool const received =
        g_test_context.wait_for_callback(std::chrono::milliseconds(10));
    HPX_TEST(received);

    HPX_TEST_EQ(g_test_context.get_callback_count(), 2);

    auto const& events = g_test_context.get_events();
    HPX_TEST(events[0] == hpx::supervision::event::started);
    HPX_TEST(events[1] == hpx::supervision::event::started);

    auto const& errors = g_test_context.get_errors();
    HPX_TEST(errors[0] == hpx::make_success_code());
    HPX_TEST(errors[1] == hpx::make_success_code());

    g_test_context.reset();

    // publish another event, will fail and not invoke callbacks
    auto const result = hpx::supervision::publish_event(hpx::launch::sync,
        locality, target, hpx::supervision::event::completed);
    HPX_TEST(result == hpx::supervision::publish_result::already_terminal);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));

    HPX_TEST_EQ(g_test_context.get_callback_count(), 0);

    // Query to confirm failed state
    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::failed);

    // Verify error was reported back correctly
    HPX_TEST(state.ec.value() == hpx::error::no_success);

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle2);
    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

// ============================================================================
// Test Cases: Exactly-Once Completion Semantics
// ============================================================================
void test_duplicate_completion_is_latched(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    reach_running(locality, target);

    g_test_context.reset();

    // First completion publication wins.
    auto const first_result = hpx::supervision::publish_event(hpx::launch::sync,
        locality, target, hpx::supervision::event::completed);
    HPX_TEST(first_result == hpx::supervision::publish_result::applied);

    auto const state_after_first =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(
        state_after_first.last_event == hpx::supervision::event::completed);

    // A duplicate completion publication is a latched no-op.
    auto const second_result =
        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::completed);
    HPX_TEST(
        second_result == hpx::supervision::publish_result::already_terminal);

    auto const state_after_second =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);

    // query_state must still reflect the original record, unchanged.
    HPX_TEST(
        state_after_second.last_event == hpx::supervision::event::completed);
    HPX_TEST_EQ(state_after_second.event_sequence_number,
        state_after_first.event_sequence_number);
    HPX_TEST(state_after_second.timestamp == state_after_first.timestamp);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Observers must be notified exactly once, not twice.
    HPX_TEST_EQ(g_test_context.get_callback_count(), 1);
    HPX_TEST(
        g_test_context.get_events()[0] == hpx::supervision::event::completed);

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

// ============================================================================
// Test Cases: Epoch-Scoped Idempotent Publishing
// ============================================================================

// Double-publishing `completed` at the same epoch must behave exactly like the
// (epoch-less) duplicate-completion latch: the sequence number does not
// advance, observers are notified exactly once, and the repeated call reports
// `already_terminal`.
void test_epoch_duplicate_completion_is_latched(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    std::uint64_t const epoch = 1;

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    reach_running_at_epoch(locality, target, epoch);

    g_test_context.reset();

    auto const first_result = hpx::supervision::publish_event(hpx::launch::sync,
        locality, target, hpx::supervision::event::completed, epoch);
    HPX_TEST(first_result == hpx::supervision::publish_result::applied);

    auto const state_after_first =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(
        state_after_first.last_event == hpx::supervision::event::completed);
    HPX_TEST_EQ(state_after_first.epoch, epoch);

    // Duplicate completion at the same epoch is a latched no-op.
    auto const second_result =
        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::completed, epoch);
    HPX_TEST(
        second_result == hpx::supervision::publish_result::already_terminal);

    auto const state_after_second =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST_EQ(state_after_second.event_sequence_number,
        state_after_first.event_sequence_number);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Observers must be notified exactly once for the completion.
    HPX_TEST_EQ(g_test_context.get_callback_count(), 1);
    HPX_TEST(
        g_test_context.get_events()[0] == hpx::supervision::event::completed);

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

// Publishing under a higher epoch than the target's current epoch resets the
// sequence number and is accepted, regardless of the previous epoch's recorded
// (possibly terminal) state.
void test_epoch_increase_resets_sequence_number(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running_at_epoch(locality, target, 1);

    auto const state_epoch1 =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST_EQ(state_epoch1.epoch, static_cast<std::uint64_t>(1));
    HPX_TEST_NEQ(state_epoch1.event_sequence_number, 0u);

    // A higher epoch is accepted and resets the sequence number, even though
    // `started` would otherwise be an illegal transition from `running`.
    hpx::error_code ec;
    auto const result = hpx::supervision::publish_event(hpx::launch::sync,
        locality, target, hpx::supervision::event::started, 2, ec);
    HPX_TEST(!ec);
    HPX_TEST(result == hpx::supervision::publish_result::applied);

    auto const state_epoch2 =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST_EQ(state_epoch2.epoch, static_cast<std::uint64_t>(2));
    HPX_TEST_EQ(
        state_epoch2.event_sequence_number, static_cast<std::uint64_t>(1));
    HPX_TEST(state_epoch2.last_event == hpx::supervision::event::started);
}

// A publication for an epoch lower than the target's current epoch must be
// rejected as stale: no state mutation, no observer notifications.
void test_stale_epoch_publish_is_noop(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, 5);

    auto const state_before =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);

    g_test_context.reset();

    auto const result = hpx::supervision::publish_event(hpx::launch::sync,
        locality, target, hpx::supervision::event::running, 3);
    HPX_TEST(result == hpx::supervision::publish_result::stale_epoch);

    auto const state_after =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state_after.last_event == state_before.last_event);
    HPX_TEST_EQ(
        state_after.event_sequence_number, state_before.event_sequence_number);
    HPX_TEST_EQ(state_after.epoch, state_before.epoch);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));
    HPX_TEST_EQ(g_test_context.get_callback_count(), 0);

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

// Concurrent publications spanning several epochs must deterministically settle
// on the highest epoch published, no matter the interleaving: any publication
// for the highest epoch always wins over lower (now stale) epochs, whichever
// order they are processed in.
void test_concurrent_publishes_settle_on_higher_epoch(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, 1);

    constexpr std::uint64_t highest_epoch = 6;
    std::vector<hpx::future<void>> publications;
    for (std::uint64_t epoch = 2; epoch <= highest_epoch; ++epoch)
    {
        publications.push_back(hpx::async([&locality, &target, epoch] {
            hpx::supervision::publish_event(hpx::launch::sync, locality, target,
                hpx::supervision::event::started, epoch);
        }));
    }
    hpx::wait_all(publications);

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST_EQ(state.epoch, highest_epoch);
    HPX_TEST_EQ(state.event_sequence_number, static_cast<std::uint64_t>(1));
}

// ============================================================================
// Test Cases: publish_event_no_notify()
//
// publish_event_no_notify() is a purely local API (no locality parameter), so
// every test below operates directly on the local supervision manager rather
// than looping over hpx::find_all_localities().
// ============================================================================

// publish_event_no_notify() must never invoke registered lifecycle observers,
// unlike publish_event() which does; exercised on independent targets so the
// notifying and non-notifying paths cannot interfere with each other.
void test_publish_event_no_notify_skips_observer()
{
    hpx::id_type const notify_target = make_test_target();
    hpx::id_type const no_notify_target = make_test_target();

    int notify_count = 0;
    int no_notify_count = 0;

    auto const notify_handle =
        hpx::supervision::register_observer(notify_target,
            [&](hpx::supervision::lifecycle_event_notification const&) {
                ++notify_count;
                return true;
            });

    auto const no_notify_handle =
        hpx::supervision::register_observer(no_notify_target,
            [&](hpx::supervision::lifecycle_event_notification const&) {
                ++no_notify_count;
                return true;
            });

    // Contrast: publish_event() does invoke the observer.
    hpx::supervision::publish_event(
        notify_target, hpx::supervision::event::started);
    HPX_TEST_EQ(notify_count, 1);

    // publish_event_no_notify() must not invoke the observer, across several
    // events including a terminal one.
    hpx::supervision::publish_event_no_notify(
        no_notify_target, hpx::supervision::event::started);
    hpx::supervision::publish_event_no_notify(
        no_notify_target, hpx::supervision::event::running);
    hpx::supervision::publish_event_no_notify(
        no_notify_target, hpx::supervision::event::completed);
    HPX_TEST_EQ(no_notify_count, 0);

    hpx::supervision::unregister_observer(notify_handle);
    hpx::supervision::unregister_observer(no_notify_handle);
}

// publish_event() and publish_event_no_notify() share apply_event_and_resolve()
// for all state mutation and only differ in whether they go on to notify
// observers; driving the identical event/epoch sequence - new-epoch open,
// same-epoch re-publish, stale-epoch rejection - through each on independent
// targets must therefore leave identical resulting state, as observed via
// query_state().
void test_publish_event_no_notify_state_parity_with_publish_event()
{
    hpx::id_type const via_notify = make_test_target();
    hpx::id_type const via_no_notify = make_test_target();

    // New-epoch open: epoch 1 has no prior entry, so it must be opened with a
    // legal `started` event.
    auto const opened_notify = hpx::supervision::publish_event(
        via_notify, hpx::supervision::event::started, 1);
    auto const opened_no_notify = hpx::supervision::publish_event_no_notify(
        via_no_notify, hpx::supervision::event::started, 1);
    HPX_TEST(opened_notify == hpx::supervision::publish_result::applied);
    HPX_TEST(opened_no_notify == hpx::supervision::publish_result::applied);

    // Same-epoch re-publish.
    auto const republish_notify = hpx::supervision::publish_event(
        via_notify, hpx::supervision::event::running, 1);
    auto const republish_no_notify = hpx::supervision::publish_event_no_notify(
        via_no_notify, hpx::supervision::event::running, 1);
    HPX_TEST(republish_notify == hpx::supervision::publish_result::applied);
    HPX_TEST(republish_no_notify == hpx::supervision::publish_result::applied);

    // Stale-epoch rejection: epoch 0 is lower than the current epoch (1) on
    // both targets and must be rejected as a no-op by both paths.
    auto const stale_notify = hpx::supervision::publish_event(
        via_notify, hpx::supervision::event::started, 0);
    auto const stale_no_notify = hpx::supervision::publish_event_no_notify(
        via_no_notify, hpx::supervision::event::started, 0);
    HPX_TEST(stale_notify == hpx::supervision::publish_result::stale_epoch);
    HPX_TEST(stale_no_notify == hpx::supervision::publish_result::stale_epoch);

    auto const state_notify = hpx::supervision::query_state(via_notify);
    auto const state_no_notify = hpx::supervision::query_state(via_no_notify);

    HPX_TEST(state_notify.last_event == state_no_notify.last_event);
    HPX_TEST_EQ(state_notify.epoch, state_no_notify.epoch);
    HPX_TEST_EQ(state_notify.event_sequence_number,
        state_no_notify.event_sequence_number);
}

// await_terminal() waiters are resolved by resolve_terminal_waiters(), called
// from within apply_event_and_resolve() - the state-mutation core shared by
// publish_event() and publish_event_no_notify() - rather than from either
// caller's notify-observers logic. A waiter pending on a target must therefore
// resolve correctly with the recorded terminal state even when that terminal
// event is published via publish_event_no_notify(), while a lifecycle observer
// registered for the same target is confirmed to remain uninvoked by that same
// call.
void test_await_terminal_resolves_via_publish_event_no_notify()
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(target, hpx::supervision::event::started);
    hpx::supervision::publish_event(target, hpx::supervision::event::running);

    int observer_calls = 0;
    auto const observer_handle = hpx::supervision::register_observer(
        target, [&](hpx::supervision::lifecycle_event_notification const&) {
            ++observer_calls;
            return true;
        });

    // register_observer() synchronously delivers one replay notification of
    // the target's current (`running`) state as part of registration; reset
    // the counter so the assertion below isolates the effect of the
    // publish_event_no_notify() call that follows.
    observer_calls = 0;

    auto f = hpx::supervision::await_terminal(target);
    HPX_TEST(!f.is_ready());

    auto const result = hpx::supervision::publish_event_no_notify(
        target, hpx::supervision::event::completed);
    HPX_TEST(result == hpx::supervision::publish_result::applied);

    auto const state = f.get();
    HPX_TEST_EQ(state.actor, target);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);

    HPX_TEST_EQ(observer_calls, 0);

    hpx::supervision::unregister_observer(observer_handle);
}

// ============================================================================
// Test Cases: Observer Epoch Filtering
// ============================================================================

// An observer registered with an epoch filter must be notified for events
// published under the matching epoch.
void test_epoch_filter_basic_match(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    constexpr std::uint64_t epoch = 4;

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        },
        epoch);

    g_test_context.reset();

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, epoch);

    bool const received =
        g_test_context.wait_for_callback(std::chrono::milliseconds(10));
    HPX_TEST_MSG(received, "Callback not received within 10ms");
    HPX_TEST_EQ(g_test_context.get_callback_count(), 1);
    HPX_TEST(
        g_test_context.get_events()[0] == hpx::supervision::event::started);

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

// An observer filtered to a specific epoch must not be notified for events
// published under a different epoch.
void test_epoch_filter_mismatch_ignored(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    constexpr std::uint64_t filter_epoch = 4;
    constexpr std::uint64_t other_epoch = 5;

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        },
        filter_epoch);

    g_test_context.reset();

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, other_epoch);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));
    HPX_TEST_EQ(g_test_context.get_callback_count(), 0);

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

// An observer registered without an epoch filter (the default) continues to
// receive notifications for every epoch (regression guard for existing,
// unfiltered behavior).
void test_epoch_filter_default_receives_all_epochs(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    g_test_context.reset();

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, 7);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, 9);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));
    HPX_TEST_EQ(g_test_context.get_callback_count(), 2);

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

// When a target has both a filtered and an unfiltered observer, an event
// published under an epoch that does not match the filter must only reach the
// unfiltered observer.
void test_epoch_filter_mixed_observers(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    constexpr std::uint64_t filter_epoch = 2;
    constexpr std::uint64_t publish_epoch = 3;

    test_context filtered_ctx, unfiltered_ctx;

    auto const filtered_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [&filtered_ctx](hpx::supervision::lifecycle_event_notification const&
                notification) {
            filtered_ctx.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        },
        filter_epoch);

    auto const unfiltered_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [&unfiltered_ctx](hpx::supervision::lifecycle_event_notification const&
                notification) {
            unfiltered_ctx.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    filtered_ctx.reset();
    unfiltered_ctx.reset();

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, publish_epoch);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));

    HPX_TEST_EQ(filtered_ctx.get_callback_count(), 0);
    HPX_TEST_EQ(unfiltered_ctx.get_callback_count(), 1);

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, filtered_handle);
    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, unfiltered_handle);
}

// Registering with an epoch filter while the target's currently recorded state
// belongs to a different epoch must not synchronously deliver that
// (non-matching) snapshot to the new observer. A later publication under the
// matching epoch must still reach it. This is the spec for the otherwise
// ambiguous "initial snapshot vs. filter" interaction: epoch_filter applies
// uniformly to every notification an observer can receive, including the
// initial state snapshot delivered at registration time.
void test_epoch_filter_initial_snapshot_respects_filter(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    constexpr std::uint64_t state_epoch = 1;
    constexpr std::uint64_t filter_epoch = 2;

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, state_epoch);

    hpx::spinlock mtx;
    std::vector<hpx::supervision::lifecycle_event_notification> received;

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [&](hpx::supervision::lifecycle_event_notification const&
                notification) {
            std::scoped_lock<hpx::spinlock> l(mtx);
            received.push_back(notification);
            return true;
        },
        filter_epoch);

    {
        std::scoped_lock<hpx::spinlock> l(mtx);
        HPX_TEST(received.empty());
    }

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, filter_epoch);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));

    {
        std::scoped_lock<hpx::spinlock> l(mtx);
        HPX_TEST_EQ(received.size(), static_cast<std::size_t>(1));
        HPX_TEST_EQ(received.front().epoch, filter_epoch);
    }

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

// Unregistering a filtered observer must remove its filter entry as well: a
// matching-epoch event published afterward must not invoke the callback (guards
// the unregister_observer lookup against the new epoch-filter storage).
void test_epoch_filter_unregister_removes_filter_entry(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    constexpr std::uint64_t epoch = 3;

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        },
        epoch);

    g_test_context.reset();

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, epoch);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));
    HPX_TEST_EQ(g_test_context.get_callback_count(), 0);
}

// ============================================================================
// Test Cases: Unregister Observer
// ============================================================================
void test_unregister_waits_for_in_flight_callback(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    std::atomic<bool> callback_entered{false};
    std::atomic<bool> release_callback{false};
    std::atomic<int> callback_count{0};
    std::atomic<bool> block_callbacks{false};

    auto const observer_handle =
        hpx::supervision::register_observer(hpx::launch::sync, locality, target,
            [&](hpx::supervision::lifecycle_event_notification const&) {
                ++callback_count;
                if (block_callbacks.load())
                {
                    callback_entered.store(true);
                    while (!release_callback.load())
                    {
                        hpx::this_thread::yield();
                    }
                }
                return true;
            });

    // Registration may synchronously deliver the state left by an earlier test.
    // Do not let that initial delivery participate in this test.
    callback_count.store(0);
    callback_entered.store(false);
    block_callbacks.store(true);

    auto publication = hpx::async([&] {
        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::started);
    });

    while (!callback_entered.load())
    {
        hpx::this_thread::yield();
    }

    auto unregistration = hpx::async([&] {
        hpx::supervision::unregister_observer(
            hpx::launch::sync, locality, observer_handle);
    });

    // unregister must wait until the callback already in progress completes.
    HPX_TEST(!unregistration.is_ready());

    release_callback.store(true);
    unregistration.get();
    publication.get();

    // After unregister returns, no later publication may invoke the callback.
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::running);
    HPX_TEST_EQ(callback_count.load(), 1);
}

void test_unregister_observer_stops_callbacks(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    g_test_context.reset();

    // Publish first event
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    bool const received =
        g_test_context.wait_for_callback(std::chrono::milliseconds(10));
    HPX_TEST(received);
    int const count_before = g_test_context.get_callback_count();
    HPX_TEST_EQ(count_before, 1);

    // Unregister the observer
    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);

    g_test_context.reset();

    // Publish second event
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::running);

    // Wait but should NOT receive callback
    hpx::this_thread::sleep_for(std::chrono::milliseconds(10));

    int const count_after = g_test_context.get_callback_count();
    HPX_TEST_EQ(count_after, 0);
}

// Regression test: unregister_observer() forced by returning false from
// the callback. Unregistering must not deadlock.
void test_unregister_observer_from_within_callback(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    std::atomic<int> invocation_count{0};

    hpx::id_type observer_handle =
        hpx::supervision::register_observer(hpx::launch::sync, locality, target,
            [&](hpx::supervision::lifecycle_event_notification const&) {
                invocation_count.fetch_add(1);
                return false;    // unregister this observer
            });

    // Triggers the callback above, which unregisters itself before returning.
    // If deactivate_and_wait() incorrectly waited on this in-flight invocation,
    // this call would hang forever.
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    HPX_TEST_EQ(invocation_count.load(), 1);

    // The observer must really be gone: later publications must not invoke
    // the callback again.
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::running);
    HPX_TEST_EQ(invocation_count.load(), 1);
}

void test_multiple_observers_same_target(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    test_context ctx1, ctx2;

    auto const observer_handle1 =
        hpx::supervision::register_observer(hpx::launch::sync, locality, target,
            [&ctx1](hpx::supervision::lifecycle_event_notification const&
                    notification) {
                ctx1.record_event(
                    notification.actor, notification.event, notification.ec);
                return true;
            });

    auto const observer_handle2 =
        hpx::supervision::register_observer(hpx::launch::sync, locality, target,
            [&ctx2](hpx::supervision::lifecycle_event_notification const&
                    notification) {
                ctx2.record_event(
                    notification.actor, notification.event, notification.ec);
                return true;
            });

    ctx1.reset();
    ctx2.reset();

    reach_running(locality, target);

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    // Both observers should receive callback
    auto const wait_for_count = [](test_context const& ctx, int const expected,
                                    std::chrono::milliseconds const timeout) {
        auto const deadline = std::chrono::steady_clock::now() + timeout;
        while (ctx.get_callback_count() < expected &&
            std::chrono::steady_clock::now() < deadline)
        {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return ctx.get_callback_count() >= expected;
    };

    bool const received1 =
        wait_for_count(ctx1, 3, std::chrono::milliseconds(100));
    bool const received2 =
        wait_for_count(ctx2, 3, std::chrono::milliseconds(100));
    HPX_TEST(received1);
    HPX_TEST(received2);

    HPX_TEST_EQ(ctx1.get_callback_count(), 3);
    HPX_TEST_EQ(ctx2.get_callback_count(), 3);

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle1);
    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle2);
}

// ============================================================================
// Test Cases: Event Snapshot Delivery
// ============================================================================
void test_publish_delivers_its_own_event_snapshot(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    std::atomic<bool> first_callback_entered{false};
    std::atomic<bool> release_first_callback{false};
    std::atomic<int> blocker_invocations{0};
    std::atomic<bool> block_first_delivery{false};

    // The first observer blocks only the first publication. This lets the
    // second publication update states_[target] before the first publication
    // reaches the recorder below.
    auto const blocker =
        hpx::supervision::register_observer(hpx::launch::sync, locality, target,
            [&](hpx::supervision::lifecycle_event_notification const&) {
                if (block_first_delivery.load() &&
                    blocker_invocations.fetch_add(1) == 0)
                {
                    first_callback_entered.store(true);
                    while (!release_first_callback.load())
                    {
                        hpx::this_thread::yield();
                    }
                }
                return true;
            });

    hpx::spinlock received_mtx;
    std::vector<hpx::supervision::lifecycle_event_notification> received;

    auto const recorder =
        hpx::supervision::register_observer(hpx::launch::sync, locality, target,
            [&](hpx::supervision::lifecycle_event_notification const&
                    notification) {
                std::scoped_lock<hpx::spinlock> l(received_mtx);
                received.push_back(notification);
                return true;
            });

    // Both registrations may have synchronously delivered the state retained
    // from earlier tests. Start the controlled race from a clean baseline.
    {
        std::scoped_lock<hpx::spinlock> l(received_mtx);
        received.clear();
    }

    blocker_invocations.store(0);
    first_callback_entered.store(false);
    block_first_delivery.store(true);

    auto first_publication = hpx::async([&] {
        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::started);
    });

    while (!first_callback_entered.load())
    {
        hpx::this_thread::yield();
    }

    // This updates the manager state while the first delivery is paused.
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::running);

    release_first_callback.store(true);
    first_publication.get();

    {
        std::scoped_lock<hpx::spinlock> l(received_mtx);
        HPX_TEST_EQ(received.size(), static_cast<std::size_t>(2));

        // Delivery order is intentionally not asserted. What matters is that
        // both distinct publications retain their own event snapshots.
        bool saw_started = false;
        bool saw_running = false;
        std::uint64_t started_sequence = 0;
        std::uint64_t running_sequence = 0;

        for (auto const& notification : received)
        {
            if (notification.event == hpx::supervision::event::started)
            {
                saw_started = true;
                started_sequence = notification.event_sequence_number;
            }
            else if (notification.event == hpx::supervision::event::running)
            {
                saw_running = true;
                running_sequence = notification.event_sequence_number;
            }
        }

        HPX_TEST(saw_started);
        HPX_TEST(saw_running);
        HPX_TEST_LT(started_sequence, running_sequence);
    }

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, recorder);
    hpx::supervision::unregister_observer(hpx::launch::sync, locality, blocker);
}

// ============================================================================
// Test Cases: Error Handling & Edge Cases
// ============================================================================
void test_query_nonexistent_actor()
{
    hpx::id_type const invalid_actor = hpx::invalid_id;

    hpx::error_code ec;
    auto state = hpx::supervision::query_state(invalid_actor, ec);
    HPX_TEST(ec);    // Should have error
}

void test_publish_no_observers(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running(locality, target);

    // Publishing with no observers should not fail
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    // Query should still work
    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);
}

void test_rapid_event_sequence(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            return true;
        });

    g_test_context.reset();

    // Publish events rapidly; this is a legal walk of the lifecycle diagram
    // that also exercises the suspending <-> running resume edge.
    std::vector<hpx::supervision::event> const events = {
        hpx::supervision::event::started, hpx::supervision::event::running,
        hpx::supervision::event::suspending, hpx::supervision::event::running,
        hpx::supervision::event::completed};

    for (auto const event : events)
    {
        auto const result = hpx::supervision::publish_event(
            hpx::launch::sync, locality, target, event);
        HPX_TEST(result == hpx::supervision::publish_result::applied);
    }

    // Once completed, the target's terminal state is latched: a further
    // terminal publication is a no-op and delivers no notification.
    auto const result = hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::failed);
    HPX_TEST(result == hpx::supervision::publish_result::already_terminal);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Only the non-latched events should be recorded
    HPX_TEST_EQ(
        g_test_context.get_callback_count(), static_cast<int>(events.size()));

    auto const observed = g_test_context.get_events();
    for (size_t i = 0; i < observed.size(); ++i)
    {
        HPX_TEST(observed[i] == events[i]);
    }

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

void test_query_after_publication(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);

    HPX_TEST(state.last_event == hpx::supervision::event::started);
    HPX_TEST(std::chrono::steady_clock::now() > state.timestamp);
}

// Miss case: querying a target for which no event has ever been recorded
// must report a staleness error code through lifecycle_state::ec, without
// the query call itself throwing or reporting failure through its own `ec`
// out-parameter.
void test_query_state_miss_returns_stale_state(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    hpx::error_code ec;
    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target, ec);
    HPX_TEST(!ec);
    HPX_TEST(state.last_event == hpx::supervision::event::unknown);
    HPX_TEST(state.ec);
    HPX_TEST(state.ec.value() == hpx::error::stale_state);
}

// Hit case (regression guard): once a target has a recorded event, querying
// it must still report success through lifecycle_state::ec.
void test_query_state_hit_returns_success(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    hpx::error_code ec;
    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target, ec);
    HPX_TEST(!ec);
    HPX_TEST(state.last_event == hpx::supervision::event::started);
    HPX_TEST(!state.ec);
    HPX_TEST(state.ec.value() == hpx::error::success);
}

// Sanity check: query_state() must remain safe to call concurrently with
// publish_event() mutating the same target's recorded state.
void test_query_state_concurrent_access(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    std::atomic<bool> stop{false};
    auto writer = hpx::async(hpx::launch::task, [&] {
        while (!stop.load(std::memory_order_relaxed))
        {
            hpx::supervision::publish_event(hpx::launch::sync, locality, target,
                hpx::supervision::event::running);
            hpx::this_thread::yield();
        }
    });

    std::vector<hpx::future<void>> readers;
    for (int i = 0; i != 4; ++i)
    {
        readers.push_back(hpx::async(hpx::launch::task, [&] {
            for (int j = 0; j != 200; ++j)
            {
                auto const state = hpx::supervision::query_state(
                    hpx::launch::sync, locality, target);
                HPX_TEST(state.last_event == hpx::supervision::event::started ||
                    state.last_event == hpx::supervision::event::running);
            }
        }));
    }

    hpx::wait_all(readers);

    stop.store(true, std::memory_order_relaxed);
    writer.get();
}

// ============================================================================
// Test Cases: Lifecycle Event Transition Validation
// ============================================================================
void test_illegal_transition_out_of_completed(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running(locality, target);

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    // `completed` is terminal; no further transitions are legal.
    hpx::error_code ec;
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::running, 0, ec);
    HPX_TEST(ec);
    HPX_TEST(ec.value() == hpx::error::bad_parameter);

    // The rejected transition must not have modified the recorded state.
    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);
}

void test_illegal_transition_unknown_to_completed(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    // The very first event recorded for a target must be `started`; jumping
    // straight to `completed` is illegal.
    hpx::error_code ec;
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed, 0, ec);
    HPX_TEST(ec);
    HPX_TEST(ec.value() == hpx::error::bad_parameter);

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::unknown);
}

void test_illegal_transitions_out_of_failed(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::failed);

    // `failed` is terminal; every possible outgoing event is illegal.
    for (auto const ev :
        {hpx::supervision::event::started, hpx::supervision::event::running,
            hpx::supervision::event::suspending,
            hpx::supervision::event::losing_locality})
    {
        hpx::error_code ec;
        hpx::supervision::publish_event(
            hpx::launch::sync, locality, target, ev, 0, ec);
        HPX_TEST(ec);
        HPX_TEST(ec.value() == hpx::error::bad_parameter);
    }

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::failed);
}

void test_legal_transitions_suspending_running_resume(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();

    reach_running(locality, target);

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::suspending);

    // Resume: suspending -> running is legal.
    hpx::error_code ec;
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::running, 0, ec);
    HPX_TEST(!ec);

    // running -> suspending is legal as well.
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::suspending, 0, ec);
    HPX_TEST(!ec);

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::suspending);
}

void test_legal_transition_losing_locality_to_failed(
    hpx::id_type const& locality)
{
    // losing_locality is reachable from started, running, and suspending, and
    // may only transition to failed.
    std::vector<hpx::supervision::event> const precursors = {
        hpx::supervision::event::started, hpx::supervision::event::running,
        hpx::supervision::event::suspending};

    for (auto const precursor : precursors)
    {
        hpx::id_type const target = make_test_target();

        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::started);
        if (precursor != hpx::supervision::event::started)
        {
            hpx::supervision::publish_event(hpx::launch::sync, locality, target,
                hpx::supervision::event::running);
        }
        if (precursor == hpx::supervision::event::suspending)
        {
            hpx::supervision::publish_event(hpx::launch::sync, locality, target,
                hpx::supervision::event::suspending);
        }

        hpx::error_code ec;
        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::losing_locality, 0, ec);
        HPX_TEST(!ec);

        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::failed, 0, ec);
        HPX_TEST(!ec);

        auto const state =
            hpx::supervision::query_state(hpx::launch::sync, locality, target);
        HPX_TEST(state.last_event == hpx::supervision::event::failed);
    }
}

// ============================================================================
// Performance Tests
// ============================================================================
void test_observer_latency_local()
{
    hpx::id_type const target = make_test_target();

    std::atomic<std::chrono::high_resolution_clock::time_point> callback_time;
    std::atomic<bool> delivered{false};

    auto const observer_handle = hpx::supervision::register_observer(
        target, [&](hpx::supervision::lifecycle_event_notification const&) {
            callback_time.store(std::chrono::high_resolution_clock::now());
            delivered.store(true);
            return true;
        });

    hpx::supervision::publish_event(target, hpx::supervision::event::started);

    // Registration/the started event above may already have delivered a
    // callback; ignore that delivery and only measure the one triggered by
    // the timed publish operation below.
    delivered.store(false);

    auto const publish_time = std::chrono::high_resolution_clock::now();

    hpx::supervision::publish_event(target, hpx::supervision::event::running);

    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    while (!delivered.load() && std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::yield();
    }
    HPX_TEST(delivered.load());

    // Latency should be < 10ms for local observation
    auto const duration = callback_time.load() - publish_time;
    HPX_TEST(duration < std::chrono::milliseconds(10));

    hpx::supervision::unregister_observer(observer_handle);
}

void test_publication_throughput()
{
    hpx::id_type const target = make_test_target();

    std::atomic<int> event_count{0};
    auto const observer_handle = hpx::supervision::register_observer(target,
        [&event_count](hpx::supervision::lifecycle_event_notification const&) {
            event_count.fetch_add(1);
            return true;
        });

    hpx::supervision::publish_event(target, hpx::supervision::event::started);

    event_count.store(0);

    // Publish 100 events
    for (int i = 0; i < 100; ++i)
    {
        hpx::supervision::publish_event(
            target, hpx::supervision::event::running);
    }

    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (
        event_count.load() < 100 && std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::yield();
    }

    HPX_TEST_EQ(event_count.load(), 100);

    hpx::supervision::unregister_observer(observer_handle);
}

// A target with an established history at epoch N must still reject a terminal
// event (completed/failed) attempting to open a *new*, higher epoch N+1 as its
// very first event -- entry into a new epoch is a transition from
// event::unknown, and event::unknown only legally transitions to
// event::started.
void test_illegal_new_epoch_opened_with_terminal(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    constexpr std::uint64_t epoch = 1;

    // Establish a normal history under epoch 1.
    reach_running_at_epoch(locality, target, epoch);

    // Register a waiter for the current epoch's terminal event *before* it is
    // published, so we can confirm it survives the illegal new-epoch publish
    // attempts below untouched, and still resolves once the legitimate terminal
    // event for `epoch` is published.
    hpx::future<hpx::supervision::lifecycle_state> waiter =
        hpx::supervision::await_terminal(locality, target, epoch);
    HPX_TEST(!waiter.is_ready());

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed, epoch);

    for (auto const ev :
        {hpx::supervision::event::completed, hpx::supervision::event::failed})
    {
        hpx::error_code ec;
        hpx::supervision::publish_event(
            hpx::launch::sync, locality, target, ev, epoch + 1, ec);
        HPX_TEST(ec);
        HPX_TEST(ec.value() == hpx::error::bad_parameter);
    }

    // The rejected new-epoch publish must not have advanced the recorded epoch
    // or overwritten the prior epoch's terminal state.
    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.epoch == epoch);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);

    // The waiter registered for the current epoch must have resolved from the
    // legitimate terminal publish, unaffected by the rejected illegal-epoch
    // attempts in between.
    auto const waited_state = waiter.get();
    HPX_TEST(waited_state.epoch == epoch);
    HPX_TEST(waited_state.last_event == hpx::supervision::event::completed);
}

// Non-terminal, non-started events must also be rejected as the first event of
// a new epoch -- only `started` may open an epoch.
void test_illegal_new_epoch_opened_with_non_started(
    hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    constexpr std::uint64_t epoch = 1;

    reach_running_at_epoch(locality, target, epoch);

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed, epoch);

    for (auto const ev :
        {hpx::supervision::event::running, hpx::supervision::event::suspending,
            hpx::supervision::event::losing_locality})
    {
        // we're forcing an unknown epoch to verify error handling
        hpx::error_code ec;
        hpx::supervision::publish_event(
            hpx::launch::sync, locality, target, ev, epoch + 1, ec);
        HPX_TEST(ec);
        HPX_TEST(ec.value() == hpx::error::bad_parameter);
    }

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.epoch == epoch);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);
}

// Regression guard: a legitimate `started` opening a brand-new, higher epoch
// after a prior epoch's terminal event must still succeed -- this is exactly
// the pattern init()/finalize() rely on across
// successive init/finalize cycles.
void test_legal_new_epoch_opened_with_started(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    constexpr std::uint64_t epoch = 1;

    reach_running_at_epoch(locality, target, epoch);

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed, epoch);

    hpx::error_code ec;
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, epoch + 1, ec);
    HPX_TEST(!ec);

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.epoch == epoch + 1);
    HPX_TEST(state.last_event == hpx::supervision::event::started);
}

// ---------------------------------------------------------------------------
// Regression coverage for two independent failure-detection paths in
// failure_detection_loop() (the query_failures consecutive-failure threshold,
// and the await_terminal timeout) can both call publish_event(shadow,
// event::failed, epoch, ...) for the same shadow from overlapping sweep phases,
// each with a potentially different captured epoch. These tests isolate and
// directly stress publish_event()'s own compare-and-mutate contract under
// concurrency, which is the primitive both paths rely on to stay safe.
// ---------------------------------------------------------------------------

// Case A: both callers race with the *same* epoch (models both detection
// paths deriving from an identical shadow snapshot). event::failed is
// terminal/latched, so the expected outcome is exactly one applied + one
// already_terminal, with the shadow ending up fenced at that one epoch
// either way.
void test_concurrent_publish_event_same_epoch(hpx::id_type const& locality)
{
    hpx::id_type const target = make_test_target();
    constexpr std::uint64_t epoch = 5;

    reach_running_at_epoch(locality, target, epoch);

    auto f1 = hpx::supervision::publish_event(
        locality, target, hpx::supervision::event::failed, epoch);
    auto f2 = hpx::supervision::publish_event(
        locality, target, hpx::supervision::event::failed, epoch);

    hpx::wait_all(f1, f2);

    auto const r1 = f1.get();
    auto const r2 = f2.get();

    bool const one_applied_one_latched =
        (r1 == hpx::supervision::publish_result::applied &&
            r2 == hpx::supervision::publish_result::already_terminal) ||
        (r2 == hpx::supervision::publish_result::applied &&
            r1 == hpx::supervision::publish_result::already_terminal);
    HPX_TEST(one_applied_one_latched);

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::failed);
    HPX_TEST_EQ(state.epoch, epoch);
}

// Case B: the two callers race with *different* epochs (models a concurrent
// reactive eviction, or a stale in-flight await_terminal continuation from an
// earlier sweep, bumping the epoch mid-race against a fresher query-failure
// fence). Invariant under test: the final stored state always converges to the
// *maximum* submitted epoch, independent of submission order.
//
// Run with both submission orders to rule out an ordering-dependent bug that
// only manifests when the lower epoch happens to be submitted first.
void test_concurrent_publish_event_racing_epochs(
    hpx::id_type const& locality, bool const submit_high_first)
{
    hpx::id_type const target = make_test_target();
    constexpr std::uint64_t epoch_low = 10;
    constexpr std::uint64_t epoch_high = 11;

    reach_running_at_epoch(locality, target, epoch_low);

    hpx::future<hpx::supervision::publish_result> f_low, f_high;
    if (submit_high_first)
    {
        hpx::future<hpx::supervision::publish_result> f_started =
            hpx::supervision::publish_event(
                locality, target, hpx::supervision::event::started, epoch_high);

        f_low = hpx::supervision::publish_event(
            locality, target, hpx::supervision::event::failed, epoch_low);

        f_started.get();
        f_high = hpx::supervision::publish_event(
            locality, target, hpx::supervision::event::failed, epoch_high);
    }
    else
    {
        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::started, epoch_high);
        f_low = hpx::supervision::publish_event(
            locality, target, hpx::supervision::event::failed, epoch_low);
        f_high = hpx::supervision::publish_event(
            locality, target, hpx::supervision::event::failed, epoch_high);
    }

    hpx::wait_all(f_low, f_high);

    auto const r_low = f_low.get();
    auto const r_high = f_high.get();

    HPX_TEST(r_high == hpx::supervision::publish_result::applied ||
        r_high == hpx::supervision::publish_result::already_terminal);
    HPX_TEST(r_low == hpx::supervision::publish_result::applied ||
        r_low == hpx::supervision::publish_result::stale_epoch ||
        r_low == hpx::supervision::publish_result::already_terminal);
    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::failed);
    HPX_TEST_EQ(state.epoch, epoch_high);
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

int hpx_main()
{
    for (auto const& locality : hpx::find_all_localities())
    {
        HPX_SUPERVISION_TEST_RUN(test_publish_completion_async, locality);
        HPX_SUPERVISION_TEST_RUN(test_publish_failed_state, locality);

        HPX_SUPERVISION_TEST_RUN(
            test_register_observer_local_completion, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_register_observer_multiple_events, locality);
        HPX_SUPERVISION_TEST_RUN(test_register_observer, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_register_observer_receives_existing_state, locality);

        HPX_SUPERVISION_TEST_RUN(test_observe_failure_detection, locality);

        HPX_SUPERVISION_TEST_RUN(test_sequence_numbers_monotonic, locality);
        HPX_SUPERVISION_TEST_RUN(test_sequence_numbers_no_gaps, locality);
        HPX_SUPERVISION_TEST_RUN(test_detect_connector_terminal, locality);
        HPX_SUPERVISION_TEST_RUN(test_error_does_not_stop_callbacks, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_duplicate_completion_is_latched, locality);

        HPX_SUPERVISION_TEST_RUN(
            test_unregister_waits_for_in_flight_callback, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_unregister_observer_stops_callbacks, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_unregister_observer_from_within_callback, locality);
        HPX_SUPERVISION_TEST_RUN(test_multiple_observers_same_target, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_publish_delivers_its_own_event_snapshot, locality);

        HPX_SUPERVISION_TEST_RUN(test_publish_no_observers, locality);

        HPX_SUPERVISION_TEST_RUN(test_rapid_event_sequence, locality);

        HPX_SUPERVISION_TEST_RUN(test_query_after_publication, locality);

        HPX_SUPERVISION_TEST_RUN(
            test_epoch_duplicate_completion_is_latched, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_epoch_increase_resets_sequence_number, locality);
        HPX_SUPERVISION_TEST_RUN(test_stale_epoch_publish_is_noop, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_concurrent_publishes_settle_on_higher_epoch, locality);

        HPX_SUPERVISION_TEST_RUN(test_epoch_filter_basic_match, locality);
        HPX_SUPERVISION_TEST_RUN(test_epoch_filter_mismatch_ignored, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_epoch_filter_default_receives_all_epochs, locality);
        HPX_SUPERVISION_TEST_RUN(test_epoch_filter_mixed_observers, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_epoch_filter_initial_snapshot_respects_filter, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_epoch_filter_unregister_removes_filter_entry, locality);

        HPX_SUPERVISION_TEST_RUN(
            test_query_state_miss_returns_stale_state, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_query_state_hit_returns_success, locality);
        HPX_SUPERVISION_TEST_RUN(test_query_state_concurrent_access, locality);

        HPX_SUPERVISION_TEST_RUN(
            test_illegal_transition_out_of_completed, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_illegal_transition_unknown_to_completed, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_illegal_transitions_out_of_failed, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_legal_transitions_suspending_running_resume, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_legal_transition_losing_locality_to_failed, locality);

        HPX_SUPERVISION_TEST_RUN(
            test_illegal_new_epoch_opened_with_terminal, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_illegal_new_epoch_opened_with_non_started, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_legal_new_epoch_opened_with_started, locality);

        HPX_SUPERVISION_TEST_RUN(
            test_concurrent_publish_event_same_epoch, locality);
        HPX_SUPERVISION_TEST_RUN(
            test_concurrent_publish_event_racing_epochs, locality, true);
        HPX_SUPERVISION_TEST_RUN(
            test_concurrent_publish_event_racing_epochs, locality, false);
    }

    HPX_SUPERVISION_TEST_RUN(test_publish_completion);
    HPX_SUPERVISION_TEST_RUN(
        test_register_observer_keeps_initial_state_snapshot);
    HPX_SUPERVISION_TEST_RUN(test_query_nonexistent_actor);
    HPX_SUPERVISION_TEST_RUN(test_observer_latency_local);
    HPX_SUPERVISION_TEST_RUN(test_publication_throughput);

    HPX_SUPERVISION_TEST_RUN(test_publish_event_no_notify_skips_observer);
    HPX_SUPERVISION_TEST_RUN(
        test_publish_event_no_notify_state_parity_with_publish_event);
    HPX_SUPERVISION_TEST_RUN(
        test_await_terminal_resolves_via_publish_event_no_notify);

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
