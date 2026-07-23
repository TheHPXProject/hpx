//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/supervision.hpp>

#include <atomic>
#include <iostream>
#include <mutex>
#include <vector>

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
test_context g_test_context;

// ============================================================================
// Test Cases: 2B.1 - Connector Publishes Explicit Completion
// ============================================================================
void test_publish_completion_local(hpx::id_type const& locality)
{
    hpx::id_type const target = hpx::find_here();

    // Publish a completed event
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
    hpx::id_type const target = hpx::find_here();

    // Publish asynchronously
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
    hpx::id_type const target = hpx::find_here();

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
    hpx::id_type const target = hpx::find_here();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
        });

    HPX_TEST_NEQ(observer_handle, hpx::invalid_id);

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
    hpx::id_type const target = hpx::find_here();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
        });

    g_test_context.reset();

    // Publish multiple events
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::running);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    // Wait for all callbacks
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

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
    hpx::id_type const target = hpx::find_here();

    auto observer_future = hpx::supervision::register_observer(locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
        });

    auto const observer_handle = observer_future.get();
    HPX_TEST_NEQ(observer_handle, hpx::invalid_id);

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
    hpx::id_type const target = hpx::find_here();

    // Establish state before any observer exists.
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);
    auto const expected =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);

    hpx::spinlock mtx;
    std::vector<hpx::supervision::lifecycle_event_notification> received;

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [&](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            std::scoped_lock<hpx::spinlock> l(mtx);
            received.push_back(notification);
        });

    // register_observer synchronously waits for its initial delivery.
    {
        std::scoped_lock<hpx::spinlock> l(mtx);
        HPX_TEST_EQ(received.size(), static_cast<std::size_t>(1));

        auto const& notification = received.front();
        HPX_TEST_EQ(notification.actor, target);
        HPX_TEST(notification.event == hpx::supervision::event::started);
        HPX_TEST_EQ(
            notification.event_sequence_number, expected.event_sequence_number);
        HPX_TEST(notification.event_time == expected.timestamp);
        HPX_TEST(notification.ec == expected.ec);
    }

    hpx::supervision::unregister_observer(
        hpx::launch::sync, locality, observer_handle);
}

void test_register_observer_keeps_initial_state_snapshot()
{
    hpx::id_type const target = hpx::find_here();

    hpx::supervision::publish_event(target, hpx::supervision::event::started);

    auto const initial_state = hpx::supervision::query_state(target);

    std::atomic<bool> snapshot_taken{false};
    std::atomic<bool> continue_registration{false};
    hpx::spinlock received_mtx;
    std::vector<hpx::supervision::lifecycle_event_notification> received;

    hpx::supervision::server::detail::set_register_observer_snapshot_hook([&] {
        snapshot_taken.store(true);
        while (!continue_registration.load())
        {
            hpx::this_thread::yield();
        }
    });

    auto clear_hook = hpx::experimental::scope_exit([] {
        hpx::supervision::server::detail::set_register_observer_snapshot_hook(
            {});
    });

    auto registration = hpx::async([&] {
        return hpx::supervision::register_observer(target,
            [&](hpx::supervision::lifecycle_event_notification const& n,
                hpx::error_code&) {
                std::scoped_lock<hpx::spinlock> l(received_mtx);
                received.push_back(n);
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
    hpx::id_type const target = hpx::find_here();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
        });

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
    hpx::id_type const target = hpx::find_here();

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
    hpx::id_type const target = hpx::find_here();

    std::vector<uint64_t> sequence_numbers;
    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [&sequence_numbers, target](
            hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            // query state during callback
            auto const state = hpx::supervision::query_state(
                hpx::launch::sync, target, notification.actor);
            sequence_numbers.push_back(state.event_sequence_number);

            // sequence numbers must match
            HPX_TEST(notification.event_sequence_number ==
                state.event_sequence_number);

            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
        });

    g_test_context.reset();

    // Publish 5 events
    for (int i = 0; i < 5; ++i)
    {
        hpx::supervision::publish_event(hpx::launch::sync, locality, target,
            hpx::supervision::event::running);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

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
    hpx::id_type const target = hpx::find_here();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code& ec) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            HPX_THROWS_IF(ec, hpx::error::no_success,
                "test_detect_connector_terminal", "testing error reporting");
        });

    g_test_context.reset();

    // Simulate connector reaching terminal state
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    bool const received =
        g_test_context.wait_for_callback(std::chrono::milliseconds(10));
    HPX_TEST(received);

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
    hpx::id_type const target = hpx::find_here();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code& ec) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
            HPX_THROWS_IF(ec, hpx::error::no_success,
                "test_detect_connector_terminal", "testing error reporting");
        });

    auto const observer_handle2 = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
        });

    g_test_context.reset();

    // Simulate connector reaching terminal state
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    bool const received =
        g_test_context.wait_for_callback(std::chrono::milliseconds(10));
    HPX_TEST(received);

    HPX_TEST_EQ(g_test_context.get_callback_count(), 2);

    auto const& events = g_test_context.get_events();
    HPX_TEST(events[0] == hpx::supervision::event::completed);
    HPX_TEST(events[1] == hpx::supervision::event::completed);

    auto const& errors = g_test_context.get_errors();
    HPX_TEST(errors[0] == hpx::make_success_code());
    HPX_TEST(errors[1] == hpx::make_success_code());

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
// Test Cases: Unregister Observer
// ============================================================================
void test_unregister_waits_for_in_flight_callback(hpx::id_type const& locality)
{
    hpx::id_type const target = hpx::find_here();

    std::atomic<bool> callback_entered{false};
    std::atomic<bool> release_callback{false};
    std::atomic<int> callback_count{0};
    std::atomic<bool> block_callbacks{false};

    auto const observer_handle =
        hpx::supervision::register_observer(hpx::launch::sync, locality, target,
            [&](hpx::supervision::lifecycle_event_notification const&,
                hpx::error_code&) {
                ++callback_count;
                if (block_callbacks.load())
                {
                    callback_entered.store(true);
                    while (!release_callback.load())
                    {
                        hpx::this_thread::yield();
                    }
                }
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
    hpx::id_type const target = hpx::find_here();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    int const count_after = g_test_context.get_callback_count();
    HPX_TEST_EQ(count_after, 0);
}

void test_multiple_observers_same_target(hpx::id_type const& locality)
{
    hpx::id_type const target = hpx::find_here();

    test_context ctx1, ctx2;

    auto const observer_handle1 = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [&ctx1](
            hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            ctx1.record_event(
                notification.actor, notification.event, notification.ec);
        });

    auto const observer_handle2 = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [&ctx2](
            hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            ctx2.record_event(
                notification.actor, notification.event, notification.ec);
        });

    ctx1.reset();
    ctx2.reset();

    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::completed);

    // Both observers should receive callback
    bool const received1 =
        ctx1.wait_for_callback(std::chrono::milliseconds(10));
    bool const received2 =
        ctx2.wait_for_callback(std::chrono::milliseconds(10));
    HPX_TEST(received1);
    HPX_TEST(received2);

    HPX_TEST_EQ(ctx1.get_callback_count(), 1);
    HPX_TEST_EQ(ctx2.get_callback_count(), 1);

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
    hpx::id_type const target = hpx::find_here();

    std::atomic<bool> first_callback_entered{false};
    std::atomic<bool> release_first_callback{false};
    std::atomic<int> blocker_invocations{0};
    std::atomic<bool> block_first_delivery{false};

    // The first observer blocks only the first publication. This lets the
    // second publication update states_[target] before the first publication
    // reaches the recorder below.
    auto const blocker =
        hpx::supervision::register_observer(hpx::launch::sync, locality, target,
            [&](hpx::supervision::lifecycle_event_notification const&,
                hpx::error_code&) {
                if (block_first_delivery.load() &&
                    blocker_invocations.fetch_add(1) == 0)
                {
                    first_callback_entered.store(true);
                    while (!release_first_callback.load())
                    {
                        hpx::this_thread::yield();
                    }
                }
            });

    hpx::spinlock received_mtx;
    std::vector<hpx::supervision::lifecycle_event_notification> received;

    auto const recorder = hpx::supervision::register_observer(hpx::launch::sync,
        locality, target,
        [&](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            std::scoped_lock<hpx::spinlock> l(received_mtx);
            received.push_back(notification);
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
    hpx::id_type const target = hpx::find_here();

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
    hpx::id_type const target = hpx::find_here();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code&) {
            g_test_context.record_event(
                notification.actor, notification.event, notification.ec);
        });

    g_test_context.reset();

    // Publish events rapidly
    std::vector<hpx::supervision::event> const events = {
        hpx::supervision::event::started, hpx::supervision::event::running,
        hpx::supervision::event::suspending, hpx::supervision::event::completed,
        hpx::supervision::event::failed};

    for (auto const event : events)
    {
        hpx::supervision::publish_event(
            hpx::launch::sync, locality, target, event);
    }

    hpx::this_thread::sleep_for(std::chrono::milliseconds(20));

    // All events should be recorded
    HPX_TEST_EQ(g_test_context.get_callback_count(), 5);

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
    hpx::id_type const target = hpx::find_here();

    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);

    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, locality, target);

    HPX_TEST(state.last_event == hpx::supervision::event::started);
    HPX_TEST(std::chrono::steady_clock::now() > state.timestamp);
}

// ============================================================================
// Performance Tests
// ============================================================================
void test_observer_latency_local()
{
    hpx::id_type const target = hpx::find_here();

    std::atomic<std::chrono::high_resolution_clock::time_point> callback_time;

    auto const observer_handle = hpx::supervision::register_observer(target,
        [&](hpx::supervision::lifecycle_event_notification const&,
            hpx::error_code&) {
            callback_time.store(std::chrono::high_resolution_clock::now());
        });

    auto const publish_time = std::chrono::high_resolution_clock::now();

    hpx::supervision::publish_event(target, hpx::supervision::event::completed);

    // Latency should be < 1ms for local observation
    auto const duration = callback_time.load() - publish_time;
    HPX_TEST(duration < std::chrono::milliseconds(1));

    hpx::supervision::unregister_observer(observer_handle);
}

void test_publication_throughput()
{
    hpx::id_type const target = hpx::find_here();

    std::atomic<int> event_count{0};
    auto const observer_handle = hpx::supervision::register_observer(target,
        [&event_count](hpx::supervision::lifecycle_event_notification const&,
            hpx::error_code&) { event_count.fetch_add(1); });

    event_count.store(0);

    auto const start = std::chrono::high_resolution_clock::now();

    // Publish 100 events
    for (int i = 0; i < 100; ++i)
    {
        hpx::supervision::publish_event(
            target, hpx::supervision::event::running);
    }

    // 100 events should complete in < 10ms
    auto const end = std::chrono::high_resolution_clock::now();
    auto const duration = end - start;

    HPX_TEST_EQ(event_count.load(), 100);
    HPX_TEST(duration < std::chrono::milliseconds(10));

    hpx::supervision::unregister_observer(observer_handle);
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

template <typename... Args>
void print(Args... args)
{
    (..., (std::cout << args));
}

template <typename T, typename... Args>
void print(T first, Args&&... rest)
{
    std::cout << first;
    print(std::forward<Args>(rest)...);
}

#define HPX_TEST_RUN(func, ...)                                                \
    std::cout << HPX_PP_STRINGIZE(func) << "(";                                \
    print(__VA_ARGS__);                                                        \
    std::cout << ")\n";                                                        \
    func(__VA_ARGS__)

int hpx_main()
{
    // 2B.1 Tests
    for (auto const& locality : hpx::find_all_localities())
    {
        HPX_TEST_RUN(test_publish_completion_local, locality);
        HPX_TEST_RUN(test_publish_completion_async, locality);
        HPX_TEST_RUN(test_publish_failed_state, locality);

        HPX_TEST_RUN(test_register_observer_local_completion, locality);
        HPX_TEST_RUN(test_register_observer_multiple_events, locality);
        HPX_TEST_RUN(test_register_observer, locality);
        HPX_TEST_RUN(test_register_observer_receives_existing_state, locality);

        HPX_TEST_RUN(test_observe_failure_detection, locality);

        HPX_TEST_RUN(test_sequence_numbers_monotonic, locality);
        HPX_TEST_RUN(test_sequence_numbers_no_gaps, locality);
        HPX_TEST_RUN(test_detect_connector_terminal, locality);
        HPX_TEST_RUN(test_error_does_not_stop_callbacks, locality);

        HPX_TEST_RUN(test_unregister_waits_for_in_flight_callback, locality);
        HPX_TEST_RUN(test_unregister_observer_stops_callbacks, locality);
        HPX_TEST_RUN(test_multiple_observers_same_target, locality);
        HPX_TEST_RUN(test_publish_delivers_its_own_event_snapshot, locality);

        HPX_TEST_RUN(test_publish_no_observers, locality);

        HPX_TEST_RUN(test_rapid_event_sequence, locality);

        HPX_TEST_RUN(test_query_after_publication, locality);
    }

    HPX_TEST_RUN(test_register_observer_keeps_initial_state_snapshot);
    HPX_TEST_RUN(test_query_nonexistent_actor);
    HPX_TEST_RUN(test_observer_latency_local);
    HPX_TEST_RUN(test_publication_throughput);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
