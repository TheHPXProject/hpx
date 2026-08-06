//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Acceptance check: failure_detection_loop() (started by init(), stopped by
// finalize(), see dispatch_api.cpp) is the active-probing complement to the
// reactive eviction path already covered by registry_snapshot.cpp. Run with two
// localities so one side can be driven "unresponsive" (joined, then simply
// never publishes another event) while the other side's poller is the thing
// under test.
//
// Locality 0 is the "observer": it runs every assertion via dispatch_work()/
// check_admission() against its local view of locality 1's shadow. Locality 1
// is the "peer": for most scenarios it stays live (publishing nothing further
// is *not* required to be alive - only the fencing scenario deliberately
// withholds all further activity), and it self-paces via barrier::synchronize()
// so the observer can drive each scenario to completion before the peer moves
// on to teardown.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/supervision_dispatch.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

// Probe action used purely to exercise dispatch_work()'s admission check; its
// body is never expected to matter, only whether it dispatches at all.
int probe()
{
    return 1;
}
HPX_PLAIN_ACTION(probe, probe_action);

namespace {

    // Shrinks failure_detection_loop()'s await_terminal() bound well below
    // the real default_discovery_timeout (60s) so tests are deterministic
    // and fast. Must be set before init() starts the loop.
    constexpr std::chrono::milliseconds test_poll_timeout{500};

    // Polls query_state()+check_admission() in short increments (rather than
    // sleeping a hardcoded number of poll cycles) until `target` reports
    // fenced under `epoch`, or `bound` elapses. Returns whether fencing was
    // observed within `bound`.
    bool wait_until_fenced(hpx::supervision::shadow_id const& target,
        std::uint64_t const epoch, std::chrono::milliseconds const bound)
    {
        auto const deadline = std::chrono::steady_clock::now() + bound;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (hpx::supervision::check_admission(target.get(), epoch) ==
                hpx::supervision::dispatch_outcome::rejected_fenced)
            {
                return true;
            }
            hpx::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        return hpx::supervision::check_admission(target.get(), epoch) ==
            hpx::supervision::dispatch_outcome::rejected_fenced;
    }

    // Finds the joined peer's shadow id for `peer_locality` in this locality's
    // own registry, or nullopt if not (yet) joined.
    std::optional<hpx::supervision::shadow_id> find_shadow_for(
        hpx::id_type const& peer_locality)
    {
        auto const peers = hpx::supervision::testing::local_snapshot_peers();
        auto const it = std::ranges::find_if(peers, [&](auto const& peer) {
            return peer.peer_locality == peer_locality;
        });

        if (it != peers.end())
        {
            return it->shadow;
        }
        return std::nullopt;
    }

}    // namespace

// ============================================================================
// Test Cases: Failure Detection
// ============================================================================

// Scenario 1: detection causes fencing.
//
// The peer locality joins and then goes silent - no completed/failed event
// ever arrives, simulating a hard crash where no lifecycle callback fires.
// After one poll cycle (bounded by test_poll_timeout, not the real 60s
// default), the observer's local shadow for that peer must report fenced, and a
// subsequent dispatch_work() against it must fail with
// hpx::error::target_fenced - proving the *local* shadow was fenced by the
// detector, without the observer ever contacting the peer's own
// sentinel/registry.
void test_detection_causes_fencing(hpx::id_type const& peer_locality)
{
    auto const shadow = find_shadow_for(peer_locality);
    HPX_TEST(shadow.has_value());
    if (!shadow)
    {
        return;
    }

    // The epoch to assert fencing against is whatever the shadow was
    // joined/started under; query it directly rather than assuming 0, since
    // discover_and_join() seeds shadows with event::started at the peer's own
    // epoch.
    auto const state = hpx::supervision::query_state(shadow->get());
    std::uint64_t const epoch = state.epoch;

    // Give the poller a few sweep cycles' worth of time to notice the silence
    // and fence locally. Bound generously relative to test_poll_timeout so this
    // isn't flaky under scheduler jitter, but far short of the real
    // default_discovery_timeout.
    bool const fenced =
        wait_until_fenced(*shadow, epoch, std::chrono::seconds(5));
    HPX_TEST(fenced);

    hpx::future<int> f = hpx::supervision::dispatch_work<probe_action>(
        *shadow, peer_locality, epoch);

    bool caught = false;
    try
    {
        f.get();
        HPX_TEST(false);
    }
    catch (hpx::exception const& e)
    {
        caught = true;
        HPX_TEST(hpx::get_error(e) == hpx::error::target_fenced);
    }
    HPX_TEST(caught);
}

// Scenario 2: no false positives.
//
// A live, responsive peer must never be spuriously fenced across multiple poll
// cycles. Dispatches repeatedly, sleeping past at least one full poll cycle
// between each, and asserts every dispatch keeps succeeding.
void test_no_false_positives(hpx::id_type const& peer_locality)
{
    auto const shadow = find_shadow_for(peer_locality);
    HPX_TEST(shadow.has_value());
    if (!shadow)
    {
        return;
    }

    auto const state = hpx::supervision::query_state(shadow->get());
    std::uint64_t const epoch = state.epoch;

    constexpr int iterations = 5;
    for (int i = 0; i != iterations; ++i)
    {
        HPX_TEST(hpx::supervision::check_admission(shadow->get(), epoch) ==
            hpx::supervision::dispatch_outcome::admitted);

        hpx::future<int> f = hpx::supervision::dispatch_work<probe_action>(
            *shadow, peer_locality, epoch);
        HPX_TEST_NO_THROW(f.get());

        // Sleep past at least one full test_poll_timeout sweep so this
        // actually exercises repeated poll cycles, not just a single one.
        hpx::this_thread::sleep_for(test_poll_timeout * 2);
    }
}

// Scenario 3: clean shutdown while a sweep may still be in flight.
//
// Must run while `peer_locality` is still silent-but-joinable (heartbeat
// suspended, but its sentinel/registry components not yet torn down) -
// otherwise a fresh discover_and_join() finds no peer at all and
// sweep_in_flight_ can never become true. Re-arms the session under the real
// default_discovery_timeout to exercise finalize() racing a genuine
// long-running sweep, then restores the short test timeout and rejoins so
// later scenarios have a working session again.
void test_clean_shutdown_during_in_flight_sweep(
    hpx::id_type const& peer_locality)
{
    hpx::supervision::testing::set_failure_detection_poll_timeout_for_testing(
        hpx::supervision::default_discovery_timeout);

    // Tear down the short-timeout session and re-init under the long one;
    // the peer is still registered (only silent), so this rediscovers and
    // rejoins it.
    hpx::supervision::finalize();
    HPX_TEST_NO_THROW(hpx::supervision::init(hpx::launch::sync));

    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    bool sweep_started = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (hpx::supervision::testing::
                failure_detection_sweep_in_flight_for_testing())
        {
            sweep_started = true;
            break;
        }
        hpx::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    HPX_TEST(sweep_started);

    auto const start = std::chrono::steady_clock::now();
    hpx::supervision::finalize();
    auto const elapsed = std::chrono::steady_clock::now() - start;
    HPX_TEST(elapsed < std::chrono::seconds(2));

    // Restore the short timeout and rejoin so scenarios 1/4 (fencing,
    // idempotency) have a working session again. `peer_locality` is still
    // silent, so this rejoin picks it right back up.
    hpx::supervision::testing::set_failure_detection_poll_timeout_for_testing(
        test_poll_timeout);
    HPX_TEST_NO_THROW(hpx::supervision::init(hpx::launch::sync));
    (void) peer_locality;
}

// Scenario 4: idempotency of the fence.
//
// Once fenced by detection, repeated dispatches must keep failing with the same
// target_fenced error, and the shadow's epoch must not churn between calls --
// the detector must not re-fence (re-bump the epoch) on every subsequent sweep
// once a shadow is already fenced.
void test_fence_is_idempotent(hpx::id_type const& peer_locality)
{
    auto const shadow = find_shadow_for(peer_locality);
    HPX_TEST(shadow.has_value());
    if (!shadow)
    {
        return;
    }

    auto const initial_state = hpx::supervision::query_state(shadow->get());
    std::uint64_t const epoch = initial_state.epoch;

    HPX_TEST(wait_until_fenced(*shadow, epoch, std::chrono::seconds(5)));

    std::uint64_t const fenced_epoch =
        hpx::supervision::query_state(shadow->get()).epoch;

    constexpr int repeats = 3;
    for (int i = 0; i != repeats; ++i)
    {
        bool caught = false;
        try
        {
            hpx::supervision::dispatch_work<probe_action>(
                *shadow, peer_locality, epoch)
                .get();
            HPX_TEST(false);
        }
        catch (hpx::exception const& e)
        {
            caught = true;
            HPX_TEST(hpx::get_error(e) == hpx::error::target_fenced);
        }
        HPX_TEST(caught);

        // No epoch churn: a second/third sweep observing an already-fenced
        // shadow must not bump it again.
        HPX_TEST_EQ(
            hpx::supervision::query_state(shadow->get()).epoch, fenced_epoch);

        hpx::this_thread::sleep_for(test_poll_timeout);
    }
}

// ============================================================================
// Main Test Entry Point (SPMD across 2 localities)
// ============================================================================

int hpx_main()
{
    std::vector<hpx::id_type> const remote_localities =
        hpx::find_remote_localities();
    HPX_TEST(!remote_localities.empty());
    if (remote_localities.empty())
    {
        return hpx::finalize();
    }

    hpx::id_type const& peer_locality = remote_localities.front();
    bool const is_observer = (hpx::get_locality_id() == 0);

    // Shrink the poll timeout before init() starts the background loop, on
    // both sides - the peer side needs it too for scenarios 2/4's repeated
    // sweep timing to be meaningful.
    hpx::supervision::testing::set_failure_detection_poll_timeout_for_testing(
        test_poll_timeout);

    HPX_TEST_NO_THROW(hpx::supervision::init(hpx::launch::sync));
    hpx::distributed::barrier::synchronize();

    if (is_observer)
    {
        // Scenario 2 first: peer is still fully live at this point.
        test_no_false_positives(peer_locality);
    }
    hpx::distributed::barrier::synchronize();

    if (!is_observer)
    {
        // The peer now goes silent for the remainder of the test: no more
        // events are published from here on, simulating a hard crash with
        // no terminal callback ever firing.
        hpx::supervision::testing::suspend_heartbeat_for_testing();
    }
    hpx::distributed::barrier::synchronize();

    if (is_observer)
    {
        // Scenario 3: Runs first, while the peer is silent but still joinable -
        // otherwise a fresh discover_and_join() finds no peer at all.
        test_clean_shutdown_during_in_flight_sweep(peer_locality);

        // Scenario 1: peer is now silent; wait for detection.
        test_detection_causes_fencing(peer_locality);

        // Scenario 4: fence must be sticky, no epoch churn.
        test_fence_is_idempotent(peer_locality);
    }
    hpx::distributed::barrier::synchronize();

    hpx::supervision::finalize();

    return hpx::finalize();
}

int main(int const argc, char* argv[])
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
