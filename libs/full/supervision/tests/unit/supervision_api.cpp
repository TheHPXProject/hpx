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
#include <condition_variable>
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
// Test Actions
// ============================================================================

//void publish_completed_action()
//{
//    hpx::supervision::publish_event(hpx::launch::sync, hpx::find_here(),
//        hpx::supervision::event::completed);
//}
//HPX_PLAIN_ACTION(publish_completed_action);
//
//void publish_failed_action()
//{
//    hpx::supervision::publish_event(
//        hpx::launch::sync, hpx::find_here(), hpx::supervision::event::failed);
//}
//HPX_PLAIN_ACTION(publish_failed_action);
//
//void publish_async_completed_action()
//{
//    hpx::supervision::publish_event(
//        hpx::find_here(), hpx::supervision::event::completed)
//        .wait();
//}
//HPX_PLAIN_ACTION(publish_async_completed_action);
//
//void publish_started_action()
//{
//    hpx::supervision::publish_event(
//        hpx::launch::sync, hpx::find_here(), hpx::supervision::event::started);
//}
//HPX_PLAIN_ACTION(publish_started_action);
//
//void publish_running_action()
//{
//    hpx::supervision::publish_event(
//        hpx::launch::sync, hpx::find_here(), hpx::supervision::event::running);
//}
//HPX_PLAIN_ACTION(publish_running_action);

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
            hpx::error_code& ec) {
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
    HPX_TEST(events[1] == hpx::supervision::event::failed);

    auto const& errors = g_test_context.get_errors();
    HPX_TEST(errors[0] == hpx::make_success_code());
    HPX_TEST(errors[1].value() == hpx::error::no_success);

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
void test_unregister_observer_stops_callbacks(hpx::id_type const& locality)
{
    hpx::id_type const target = hpx::find_here();

    auto const observer_handle = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [](hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code& ec) {
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
            hpx::error_code& ec) {
            ctx1.record_event(
                notification.actor, notification.event, notification.ec);
        });

    auto const observer_handle2 = hpx::supervision::register_observer(
        hpx::launch::sync, locality, target,
        [&ctx2](
            hpx::supervision::lifecycle_event_notification const& notification,
            hpx::error_code& ec) {
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
            hpx::error_code& ec) {
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

#if 0
// ============================================================================
// Test Cases: Observer Staleness Detection
// ============================================================================
void test_observer_staleness_detection()
{
    std::vector<hpx::id_type> const localities = hpx::find_all_localities();

    if (localities.size() < 2)
    {
        HPX_TEST_MSG(false, "Test requires at least 2 localities");
        return;
    }

    hpx::id_type remote_locality = localities[1];

    // Register observer from non-owner locality
    std::atomic<bool> observer_registered{false};
    auto const observer_handle =
        hpx::supervision::register_observer(hpx::launch::sync, remote_locality,
            [&observer_registered](hpx::id_type actor,
                hpx::supervision::event event, hpx::error_code& ec) {
                if (!ec)
                {
                    observer_registered.store(true);
                }
            });

    // Query state - may indicate staleness if observer hasn't synced
    auto const state =
        hpx::supervision::query_state(hpx::launch::sync, remote_locality);

    // If observer_staleness is set, it indicates the remote observer
    // may not have latest state
    if (state.observer_staleness)
    {
        HPX_TEST_MSG(
            true, "Observer staleness detected and reported correctly");
    }

    hpx::supervision::unregister_observer(hpx::launch::sync, observer_handle);
}

// ============================================================================
// Test Cases: Bulk Observation (Locality-Level)
// ============================================================================
void test_bulk_observe_locality_completions()
{
    hpx::id_type const locality = hpx::find_here();

    std::atomic<int> summary_count{0};
    std::atomic<uint64_t> last_version{0};

    auto const observer_handle =
        hpx::supervision::observe_locality_completions(locality,
            [&summary_count, &last_version](
                hpx::supervision::locality_completion_summary const& summary,
                hpx::error_code& ec) {
                if (!ec)
                {
                    summary_count.fetch_add(1);
                    last_version.store(summary.summary_version);
                }
            });

    HPX_TEST_NEQ(observer_handle, hpx::invalid_id);

    // Publish multiple events
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, hpx::supervision::event::started);
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, hpx::supervision::event::running);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Should receive updates for each event
    HPX_TEST_GT(summary_count.load(), 0);
    HPX_TEST_GT(last_version.load(), 0u);

    hpx::supervision::unregister_observer(hpx::launch::sync, observer_handle);
}

void test_bulk_observe_version_increments()
{
    hpx::id_type const locality = hpx::find_here();

    std::vector<uint64_t> versions;

    auto const observer_handle =
        hpx::supervision::observe_locality_completions(locality,
            [&versions](
                hpx::supervision::locality_completion_summary const& summary,
                hpx::error_code& ec) {
                if (!ec)
                {
                    versions.push_back(summary.summary_version);
                }
            });

    // Publish multiple events
    for (int i = 0; i < 3; ++i)
    {
        hpx::supervision::publish_event(
            hpx::launch::sync, locality, hpx::supervision::event::running);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Versions should increment
    for (size_t i = 1; i < versions.size(); ++i)
    {
        HPX_TEST_GT(versions[i], versions[i - 1]);
    }

    hpx::supervision::unregister_observer(hpx::launch::sync, observer_handle);
}
#endif

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

        HPX_TEST_RUN(test_observe_failure_detection, locality);

        HPX_TEST_RUN(test_sequence_numbers_monotonic, locality);
        HPX_TEST_RUN(test_sequence_numbers_no_gaps, locality);
        HPX_TEST_RUN(test_detect_connector_terminal, locality);
        HPX_TEST_RUN(test_error_does_not_stop_callbacks, locality);

        HPX_TEST_RUN(test_unregister_observer_stops_callbacks, locality);
        HPX_TEST_RUN(test_multiple_observers_same_target, locality);

        HPX_TEST_RUN(test_publish_no_observers, locality);

        HPX_TEST_RUN(test_rapid_event_sequence, locality);

        HPX_TEST_RUN(test_query_after_publication, locality);
    }

    HPX_TEST_RUN(test_query_nonexistent_actor);
    HPX_TEST_RUN(test_observer_latency_local);
    HPX_TEST_RUN(test_publication_throughput);

#if 0
    HPX_TEST_RUN(test_observer_staleness_detection);

    HPX_TEST_RUN(test_bulk_observe_locality_completions);
    HPX_TEST_RUN(test_bulk_observe_version_increments);
#endif

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
