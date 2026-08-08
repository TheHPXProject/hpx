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

    std::optional<hpx::supervision::server::peer_snapshot> find_peer_for(
        hpx::id_type const& peer_locality)
    {
        auto const peers = hpx::supervision::testing::local_snapshot_peers();
        auto const it = std::ranges::find_if(peers, [&](auto const& peer) {
            return peer.peer_locality == peer_locality;
        });

        if (it != peers.end())
        {
            return *it;
        }
        return std::nullopt;
    }
}    // namespace

// The peer suspends its heartbeat immediately after join() completes, before
// the observer's failure_detection_loop() has any realistic chance to run a
// single successful query_state() sweep against it - the exact window the
// old default-0 query_failures seeding mishandled.
void test_fencing_without_prior_successful_query(
    hpx::id_type const& peer_locality)
{
    auto const peer = find_peer_for(peer_locality);
    HPX_TEST(peer.has_value());
    if (!peer)
    {
        return;
    }

    std::uint64_t const join_epoch = peer->join_epoch;
    HPX_TEST_NEQ(join_epoch, static_cast<std::uint64_t>(0));
    HPX_TEST_EQ(
        hpx::supervision::query_state(peer->peer_locality).epoch, join_epoch);

    bool const fenced = wait_until_fenced(
        peer->peer_locality, join_epoch, std::chrono::seconds(5));
    HPX_TEST(fenced);

    HPX_TEST(
        hpx::supervision::check_admission(peer->peer_locality, join_epoch) ==
        hpx::supervision::dispatch_outcome::rejected_fenced);

    hpx::future<int> f = hpx::supervision::dispatch_work<probe_action>(
        peer->peer_locality, join_epoch);

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

// Regression check for registry_server.cpp's peer_locality dual-publish fix:
// register_observers() and join() must mirror a joined peer's lifecycle events
// onto the peer's own locality (peer_locality), not just onto the joining
// registry's local shadow copy, so that invoke_fenced_action()'s authoritative
// re-check -- which always consults whichever locality it is actually executing
// on (see dispatch_work.hpp) -- can see the fence.
//
// Deliberately bypasses dispatch_work()'s caller-local early-out by dispatching
// fenced_action directly to peer_locality: that early-out independently
// observes the *local* mirror, which was never missing, so routing through
// dispatch_work() here would mask the bug under test. Before the
// registry_server.cpp fix, peer_locality's supervision_manager never received a
// publish_event() call for this shadow at all, so the authoritative re-check
// would silently admit; after the fix, it must observe the fence.
void test_cross_locality_authoritative_fence(hpx::id_type const& peer_locality)
{
    hpx::supervision::sentinel const peer_sentinel(peer_locality);
    hpx::supervision::registry const r(hpx::find_here());

    hpx::supervision::joined_peer const peer =
        r.join(hpx::launch::sync, peer_sentinel, peer_locality);
    HPX_TEST_NEQ(peer.target, hpx::invalid_id);

    // Bring the peer sentinel to a legal terminal state (started -> failed)
    // directly on peer_locality, triggering the registry's lifecycle observer.
    //
    // This test shares peer_locality with
    // test_fencing_without_prior_successful_query(), which runs first and can
    // drive peer_locality's shadow past epoch 0 (join()'s escalation logic
    // seeds a fresh epoch past whatever terminal state that earlier test left
    // behind). publish_event()'s "never regress" contract would then silently
    // drop a publish at a stale, lower epoch, so every epoch used below must
    // be the actual value returned by join() (peer.join_epoch), not a
    // hardcoded 0.
    peer_sentinel.start(hpx::launch::sync);
    hpx::supervision::publish_event(hpx::launch::sync, peer_locality,
        peer_sentinel.get_id(), hpx::supervision::event::failed,
        peer.join_epoch);

    // Wait for the observer's mirroring callback to run. Its dual-publish onto
    // peer_locality (see register_observers() in registry_server.cpp) happens
    // strictly before its local publish returns, so once the local mirror is
    // observed as fenced here, the peer_locality-side mirror is guaranteed to
    // be in place too.
    bool const fenced = wait_until_fenced(
        peer.target, peer.join_epoch, std::chrono::seconds(5));
    HPX_TEST(fenced);

    // Dispatch fenced_action directly to peer_locality, bypassing
    // dispatch_work()'s caller-local early-out entirely, so only the
    // authoritative re-check performed on peer_locality's own
    // supervision_manager decides the outcome.
    using probe_fenced_action = hpx::supervision::fenced_action<probe_action,
        hpx::id_type, std::uint64_t>;

    hpx::future<int> f =
        hpx::async(probe_fenced_action(), hpx::colocated(peer_locality),
            probe_action(), peer.target, peer.join_epoch);

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
        test_fencing_without_prior_successful_query(peer_locality);
        test_cross_locality_authoritative_fence(peer_locality);
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
