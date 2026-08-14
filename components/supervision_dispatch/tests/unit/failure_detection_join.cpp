//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression check for the join_epoch seeding fix: failure_detection_loop()'s
// query_failures map must seed a shadow's first tracking entry from its real
// join-time epoch (server::peer_snapshot::join_epoch), not a default-
// constructed 0. Otherwise, a peer that goes silent before the detector's very
// first successful query_state() against it is fenced (if at all) with a stale
// epoch of 0, which publish_event()'s "never regress" contract then silently
// rejects, permanently defeating the fence.
//
// Deliberately isolated from failure_detection.cpp: that file's
// suspend_heartbeat_for_testing() call is documented as one-shot/irreversible
// ("simulating a hard crash"). Reusing its single suspend point here would
// either permanently silence the peer before test_no_false_positives needs it
// live, or (if placed later) no longer exercise the bug, since by then the
// detector has already seeded a real epoch from earlier successful sweeps.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/async_distributed.hpp>
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

int probe()
{
    return 1;
}
HPX_PLAIN_ACTION(probe, probe_action);

namespace {

    constexpr std::chrono::milliseconds test_poll_timeout{500};

    bool wait_until_fenced(hpx::id_type const& target,
        std::uint64_t const epoch, std::chrono::milliseconds const bound)
    {
        auto const deadline = std::chrono::steady_clock::now() + bound;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (hpx::supervision::check_admission(target, epoch) ==
                hpx::supervision::dispatch_outcome::rejected_fenced)
            {
                return true;
            }
            hpx::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        return hpx::supervision::check_admission(target, epoch) ==
            hpx::supervision::dispatch_outcome::rejected_fenced;
    }
}    // namespace

// Regression check for register_observers()'s epoch-sensitive guard (see
// registry_server.cpp's "notification.epoch > join_epoch" handling): a fix that
// only re-arms mirroring for a peer's *first* epoch (e.g. by relying on some
// one-time registration/reset state that is never re-armed on epoch rollover)
// could still silently stop mirroring once the peer rejoins under a new epoch.
// This drives peer_locality through two distinct epochs against the same
// registry and asserts that mirroring - and the "never regress" / stale-epoch
// rejection contract of publish_event() - keeps working identically in the
// second epoch, not just the first.
//
// Reuses the same join()/publish_event() scaffolding as
// test_cross_locality_authoritative_fence() above. In particular,
// peer_locality's locally tracked shadow state carries over from whatever the
// previously run tests in this file left behind, so every epoch used below is
// read back from join()'s return value (peer1.join_epoch / peer2.join_epoch)
// rather than hardcoded - see that test's comment for why.
hpx::supervision::joined_peer test_mirroring_survives_epoch_rollover(
    hpx::id_type const& peer_locality)
{
    hpx::supervision::registry const r(hpx::find_here());

    // --- Epoch N ---
    hpx::supervision::joined_peer const peer1 =
        r.join(hpx::launch::sync, peer_locality);
    HPX_TEST_NEQ(peer1.target, hpx::invalid_id);

    std::uint64_t const epoch_n = peer1.join_epoch + 1;

    // Drive started -> running -> running under epoch N, confirming mirroring
    // (mirrored epoch matches, event_sequence_number strictly advancing) - the
    // same monotonicity check used by
    // test_cross_locality_sequence_number_increasing() in
    // registry_mirroring.cpp.
    std::vector<hpx::supervision::event> const first_sequence{
        hpx::supervision::event::started, hpx::supervision::event::running,
        hpx::supervision::event::running};

    std::uint64_t previous = 0;
    for (auto const ev : first_sequence)
    {
        hpx::supervision::publish_event(
            hpx::launch::sync, peer_locality, peer_locality, ev, epoch_n);

        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        hpx::supervision::lifecycle_state state =
            hpx::supervision::query_state(peer1.target);
        while (state.event_sequence_number <= previous &&
            std::chrono::steady_clock::now() < deadline)
        {
            hpx::this_thread::yield();
            state = hpx::supervision::query_state(peer1.target);
        }

        HPX_TEST(state.event_sequence_number > previous);
        HPX_TEST_EQ(state.epoch, epoch_n);
        HPX_TEST(state.last_event == ev);

        previous = state.event_sequence_number;
    }

    // Drive the peer to a terminal event so the registry's own lifecycle
    // observer (registered by the join() above) evicts this entry from its
    // internal peers_ map (see registry::evict_peer() in registry_server.cpp) -
    // the same mechanism test_registry_join_terminal_peer_evicted_from_peers()
    // in registry_join.cpp relies on to make a *later* join() for the same peer
    // mint a fresh epoch instead of returning this same, now-terminal entry.
    hpx::supervision::publish_event(hpx::launch::sync, peer_locality,
        peer_locality, hpx::supervision::event::failed, epoch_n);

    // Eviction is dispatched asynchronously (via hpx::post(), see
    // register_observers()); poll re-joining via the very same registry `r`
    // until it returns a different entry, exactly as
    // test_registry_join_terminal_peer_evicted_from_peers() does.
    hpx::supervision::joined_peer peer2 = peer1;
    auto const rejoin_deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < rejoin_deadline)
    {
        peer2 = r.join(hpx::launch::sync, peer_locality);
        if (peer2 != peer1)
        {
            break;
        }
        hpx::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    HPX_TEST(peer2 != peer1);
    HPX_TEST_NEQ(peer2.target, hpx::invalid_id);

    std::uint64_t const epoch_n1 = peer2.join_epoch;
    HPX_TEST(epoch_n1 > epoch_n);

    // --- Simulated rejoin: epoch N+1 ---
    //
    // Same started -> running -> running -> completed sequence and monotonicity
    // checks as epoch N above, but the mirrored epoch must now track epoch_n1,
    // not remain stuck at epoch_n - exactly the gap a fix that only re-arms
    // mirroring once (for the peer's first epoch) would fail to close.
    std::vector<hpx::supervision::event> const second_sequence{
        hpx::supervision::event::started,
        hpx::supervision::event::running,
        hpx::supervision::event::running,
        hpx::supervision::event::completed,
    };

    previous = 0;
    for (auto const ev : second_sequence)
    {
        hpx::supervision::publish_event(
            hpx::launch::sync, peer_locality, peer_locality, ev, epoch_n1);

        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        hpx::supervision::lifecycle_state state =
            hpx::supervision::query_state(peer2.target);

        while (state.event_sequence_number <= previous &&
            std::chrono::steady_clock::now() < deadline)
        {
            hpx::this_thread::yield();
            state = hpx::supervision::query_state(peer2.target);
        }

        HPX_TEST_EQ(state.epoch, epoch_n1);
        HPX_TEST(state.last_event == ev);
        HPX_TEST(state.event_sequence_number > previous);

        previous = state.event_sequence_number;
    }

    // A stale-epoch (N) publish attempted after the rejoin must be rejected the
    // same way as before the rejoin - no leakage into the N+1 state.
    hpx::supervision::lifecycle_state const before_stale =
        hpx::supervision::query_state(
            hpx::launch::sync, peer_locality, peer2.target);

    hpx::supervision::publish_event(hpx::launch::sync, peer_locality,
        peer_locality, hpx::supervision::event::running, epoch_n);

    hpx::supervision::lifecycle_state const after_stale =
        hpx::supervision::query_state(
            hpx::launch::sync, peer_locality, peer2.target);

    HPX_TEST_EQ(after_stale.epoch, before_stale.epoch);
    HPX_TEST_EQ(
        after_stale.event_sequence_number, before_stale.event_sequence_number);
    HPX_TEST(after_stale.last_event == before_stale.last_event);

    return peer2;
}

// The peer suspends its heartbeat immediately after the second barrier
// completes, before the observer's failure_detection_loop() has any realistic
// chance to run a single successful query_state() sweep against it - the exact
// window the old default-0 query_failures seeding mishandled.
void test_fencing_without_prior_successful_query(
    hpx::id_type const& peer_locality,
    hpx::supervision::joined_peer const& peer)
{
    std::uint64_t const join_epoch = peer.join_epoch;
    HPX_TEST_NEQ(join_epoch, static_cast<std::uint64_t>(0));

    auto const state = hpx::supervision::query_state(
        hpx::launch::sync, peer_locality, peer_locality);
    HPX_TEST_EQ(state.epoch, join_epoch);
    HPX_TEST(state.last_event == hpx::supervision::event::completed);

    // the peer's state should have been mirrored here
    auto const local_state = hpx::supervision::query_state(peer_locality);
    HPX_TEST_EQ(local_state.epoch, join_epoch);
    HPX_TEST(local_state.last_event == hpx::supervision::event::completed);

    bool const fenced =
        wait_until_fenced(peer_locality, join_epoch, std::chrono::seconds(5));
    HPX_TEST(fenced);

    HPX_TEST(hpx::supervision::check_admission(peer_locality, join_epoch) ==
        hpx::supervision::dispatch_outcome::rejected_fenced);

    hpx::future<int> f = hpx::supervision::dispatch_work<probe_action>(
        peer_locality, join_epoch);

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

    hpx::supervision::testing::set_failure_detection_poll_timeout_for_testing(
        test_poll_timeout);

    HPX_TEST_NO_THROW(hpx::supervision::init(hpx::launch::sync));
    hpx::distributed::barrier::synchronize();

    // Peer suspends heartbeat immediately after join, before any barrier-
    // synchronized step could let the observer succeed even once. No
    // no-false-positives case to preserve in this file, unlike
    // failure_detection.cpp.
    if (!is_observer)
    {
        hpx::supervision::testing::suspend_heartbeat_for_testing();
    }
    hpx::distributed::barrier::synchronize();

    if (is_observer)
    {
        hpx::supervision::joined_peer peer =
            test_mirroring_survives_epoch_rollover(peer_locality);
        test_fencing_without_prior_successful_query(peer_locality, peer);
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
