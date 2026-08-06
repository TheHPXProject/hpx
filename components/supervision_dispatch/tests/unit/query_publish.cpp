//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Acceptance check for the supervision_handle-based query_state()/
// publish_event() overloads added to dispatch_api.hpp/dispatch_api.cpp: these
// are purely additive, forwarding-only convenience overloads over the raw-id
// hpx::supervision::query_state()/publish_event() free functions, so this file
// focuses on confirming the forwarding wiring itself (which id/locality each
// overload actually reads/writes, and that the sync overloads agree with their
// async counterparts) rather than re-testing the underlying lifecycle state
// machine or distributed join/discovery machinery, which are already covered
// elsewhere (registry_join.cpp, discover_and_join.cpp, failure_detection*.cpp).
//
// Most scenarios below deliberately construct sentinels/discovered_peer values
// directly, the same way registry_join.cpp does, instead of going through a
// full init()/discover_and_join() cycle: the handle-only and handle+peer
// overloads never touch dispatch_state (see dispatch_api.cpp), so exercising
// them against ad hoc components keeps these tests deterministic and
// independent of the background failure-detection/ heartbeat loops. The one
// exception is test_epoch_after_init(), which specifically verifies the epoch
// init() itself established.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/supervision_dispatch/dispatch_api.hpp>

#include <chrono>
#include <cstdint>

namespace {

    // No remote localities are ever present in this (single-locality) test, so
    // discover_and_join()'s single reactive pass inside init() returns
    // immediately regardless of this bound; kept short purely for hygiene.
    constexpr std::chrono::seconds test_discovery_timeout{2};
}    // namespace

// Scenario 1: epoch-after-init.
//
// query_state(sync, handle) immediately after init() must reflect the real
// epoch run_init_sequence() established via local_sentinel.start(sync,
// new_epoch) -- not a stale/default value -- and a subsequent init() cycle
// (after finalize()) must observe a strictly higher epoch than the previous
// cycle, mirroring epoch_'s monotonically-increasing contract.
void test_epoch_after_init()
{
    HPX_TEST(!hpx::supervision::is_initialized());

    hpx::supervision::supervision_handle const handle1 =
        hpx::supervision::init(hpx::launch::sync, test_discovery_timeout);

    hpx::supervision::lifecycle_state const state1 =
        hpx::supervision::query_state(hpx::launch::sync, handle1);
    HPX_TEST(state1.last_event == hpx::supervision::event::started);
    HPX_TEST_NEQ(state1.epoch, static_cast<std::uint64_t>(0));

    hpx::supervision::finalize();
    HPX_TEST(!hpx::supervision::is_initialized());

    hpx::supervision::supervision_handle const handle2 =
        hpx::supervision::init(hpx::launch::sync, test_discovery_timeout);

    hpx::supervision::lifecycle_state const state2 =
        hpx::supervision::query_state(hpx::launch::sync, handle2);
    HPX_TEST(state2.last_event == hpx::supervision::event::started);
    HPX_TEST_EQ(state2.epoch, state1.epoch + 1);

    hpx::supervision::finalize();
    HPX_TEST(!hpx::supervision::is_initialized());
}

// Scenario 2: publish-then-query round trip.
//
// publish_event(handle, ev, epoch) (and its sync overload) must be visible to a
// subsequent query_state(sync, handle) call, confirming that both overloads
// forward to the same (hpx::find_here(), handle.sentinel_client) target rather
// than e.g. mismatched localities.
void test_publish_then_query_round_trip()
{
    hpx::supervision::sentinel const s(hpx::find_here());
    hpx::supervision::supervision_handle const handle{
        .sentinel_client = s, .registry_client = hpx::supervision::registry()};

    constexpr std::uint64_t epoch = 1;

    HPX_TEST(hpx::supervision::publish_event(
                 handle, hpx::supervision::event::started, epoch)
                 .get() == hpx::supervision::publish_result::applied);

    HPX_TEST(hpx::supervision::publish_event(hpx::launch::sync, handle,
                 hpx::supervision::event::running,
                 epoch) == hpx::supervision::publish_result::applied);

    hpx::supervision::lifecycle_state const state =
        hpx::supervision::query_state(hpx::launch::sync, handle);
    HPX_TEST(state.last_event == hpx::supervision::event::running);
    HPX_TEST_EQ(state.epoch, epoch);
    HPX_TEST_EQ(state.actor, s.get_id());

    hpx::supervision::remove_target(s.get_id());
}

// Scenario 3: peer cross-check.
//
// query_state(handle, peer) must resolve against peer.locality and
// peer.sentinel_client.get_id() -- not handle's own sentinel -- while
// query_state(handle) (self) must keep resolving against handle's own sentinel.
// Uses two distinct sentinel components, advanced to different lifecycle
// states, so a forwarding bug that mixed the two up would be immediately
// observable.
void test_query_state_peer_overload_uses_peer_not_handle()
{
    hpx::supervision::sentinel const handle_sentinel(hpx::find_here());
    hpx::supervision::supervision_handle const handle{
        .sentinel_client = handle_sentinel,
        .registry_client = hpx::supervision::registry()};
    HPX_TEST(hpx::supervision::publish_event(
                 handle, hpx::supervision::event::started, 0)
                 .get() == hpx::supervision::publish_result::applied);

    hpx::supervision::sentinel const peer_sentinel(hpx::find_here());
    hpx::supervision::discovered_peer const peer{.locality = hpx::find_here(),
        .sentinel_client = peer_sentinel,
        .registry_client = hpx::supervision::registry(),
        .join_epoch = 0};
    HPX_TEST(hpx::supervision::publish_event(hpx::find_here(),
                 peer_sentinel.get_id(), hpx::supervision::event::started, 0)
                 .get() == hpx::supervision::publish_result::applied);
    HPX_TEST(hpx::supervision::publish_event(hpx::find_here(),
                 peer_sentinel.get_id(), hpx::supervision::event::running, 0)
                 .get() == hpx::supervision::publish_result::applied);

    hpx::supervision::lifecycle_state const self_state =
        hpx::supervision::query_state(handle).get();
    hpx::supervision::lifecycle_state const peer_state =
        hpx::supervision::query_state(handle, peer).get();

    HPX_TEST(self_state.last_event == hpx::supervision::event::started);
    HPX_TEST_EQ(self_state.actor, handle_sentinel.get_id());

    HPX_TEST(peer_state.last_event == hpx::supervision::event::running);
    HPX_TEST_EQ(peer_state.actor, peer_sentinel.get_id());

    hpx::supervision::remove_target(handle_sentinel.get_id());
    hpx::supervision::remove_target(peer_sentinel.get_id());
}

// Scenario 4: fenced/terminal state check.
//
// Once a peer has been driven to a terminal lifecycle event (mirroring what
// dispatch_api.cpp's failure_detection_loop() does when it fences an
// unresponsive peer), query_state(handle, peer) must reflect that terminal
// state -- and keep reflecting it, rather than any later rejected publication,
// since terminal events are latched per (target, epoch).
void test_query_state_reflects_peer_terminal_state()
{
    hpx::supervision::sentinel const handle_sentinel(hpx::find_here());
    hpx::supervision::supervision_handle const handle{
        .sentinel_client = handle_sentinel,
        .registry_client = hpx::supervision::registry()};

    hpx::supervision::sentinel const peer_sentinel(hpx::find_here());
    constexpr std::uint64_t join_epoch = 3;
    hpx::supervision::discovered_peer const peer{.locality = hpx::find_here(),
        .sentinel_client = peer_sentinel,
        .registry_client = hpx::supervision::registry(),
        .join_epoch = join_epoch};

    // Legal path to a terminal state: started -> running -> failed.
    HPX_TEST(hpx::supervision::publish_event(hpx::find_here(),
                 peer_sentinel.get_id(), hpx::supervision::event::started,
                 join_epoch)
                 .get() == hpx::supervision::publish_result::applied);
    HPX_TEST(hpx::supervision::publish_event(hpx::find_here(),
                 peer_sentinel.get_id(), hpx::supervision::event::running,
                 join_epoch)
                 .get() == hpx::supervision::publish_result::applied);
    HPX_TEST(
        hpx::supervision::publish_event(hpx::find_here(),
            peer_sentinel.get_id(), hpx::supervision::event::failed, join_epoch)
            .get() == hpx::supervision::publish_result::applied);

    hpx::supervision::lifecycle_state const peer_state =
        hpx::supervision::query_state(hpx::launch::sync, handle, peer);
    HPX_TEST(peer_state.last_event == hpx::supervision::event::failed);
    HPX_TEST_EQ(peer_state.epoch, join_epoch);

    // Terminal latching: a later publication for the same target/epoch is a
    // no-op, and query_state(handle, peer) must keep reflecting the
    // already-fenced state rather than anything from the rejected call.
    HPX_TEST(
        hpx::supervision::publish_event(hpx::find_here(),
            peer_sentinel.get_id(), hpx::supervision::event::failed, join_epoch)
            .get() == hpx::supervision::publish_result::already_terminal);

    hpx::supervision::lifecycle_state const peer_state_after =
        hpx::supervision::query_state(hpx::launch::sync, handle, peer);
    HPX_TEST(peer_state_after.last_event == hpx::supervision::event::failed);
    HPX_TEST_EQ(peer_state_after.epoch, join_epoch);

    hpx::supervision::remove_target(handle_sentinel.get_id());
    hpx::supervision::remove_target(peer_sentinel.get_id());
}

// Scenario 5: sync-vs-async equivalence.
//
// For query_state() (both the handle-only and handle+peer overloads) and for
// publish_event() (the handle-only overload -- publish_event() has no
// handle+peer overload), the sync overload's result must equal .get() on the
// async overload's future for the same inputs, confirming the sync overloads
// really do just forward to `.get()` on their async counterpart rather than
// diverging in behavior.
void test_sync_vs_async_equivalence()
{
    hpx::supervision::sentinel const s(hpx::find_here());
    hpx::supervision::supervision_handle const handle{
        .sentinel_client = s, .registry_client = hpx::supervision::registry()};
    HPX_TEST(hpx::supervision::publish_event(
                 handle, hpx::supervision::event::started, 7)
                 .get() == hpx::supervision::publish_result::applied);

    // query_state(): handle-only overload.
    hpx::supervision::lifecycle_state const async_self =
        hpx::supervision::query_state(handle).get();
    hpx::supervision::lifecycle_state const sync_self =
        hpx::supervision::query_state(hpx::launch::sync, handle);
    HPX_TEST(async_self.last_event == sync_self.last_event);
    HPX_TEST_EQ(async_self.epoch, sync_self.epoch);
    HPX_TEST_EQ(async_self.actor, sync_self.actor);

    // query_state(): handle+peer overload.
    hpx::supervision::sentinel const peer_sentinel(hpx::find_here());
    hpx::supervision::discovered_peer const peer{.locality = hpx::find_here(),
        .sentinel_client = peer_sentinel,
        .registry_client = hpx::supervision::registry(),
        .join_epoch = 0};
    HPX_TEST(hpx::supervision::publish_event(hpx::find_here(),
                 peer_sentinel.get_id(), hpx::supervision::event::started, 0)
                 .get() == hpx::supervision::publish_result::applied);

    hpx::supervision::lifecycle_state const async_peer =
        hpx::supervision::query_state(handle, peer).get();
    hpx::supervision::lifecycle_state const sync_peer =
        hpx::supervision::query_state(hpx::launch::sync, handle, peer);
    HPX_TEST(async_peer.last_event == sync_peer.last_event);
    HPX_TEST_EQ(async_peer.epoch, sync_peer.epoch);
    HPX_TEST_EQ(async_peer.actor, sync_peer.actor);

    // publish_event(): handle-only overload. `running -> running` is a legal
    // self-loop (see hpx::supervision::is_valid_transition()), so issuing the
    // exact same call once through each overload is expected to yield `applied`
    // both times rather than one of them being rejected.
    hpx::supervision::publish_result const async_publish =
        hpx::supervision::publish_event(
            handle, hpx::supervision::event::running, 7)
            .get();
    hpx::supervision::publish_result const sync_publish =
        hpx::supervision::publish_event(
            hpx::launch::sync, handle, hpx::supervision::event::running, 7);
    HPX_TEST(async_publish == hpx::supervision::publish_result::applied);
    HPX_TEST(sync_publish == hpx::supervision::publish_result::applied);

    hpx::supervision::remove_target(s.get_id());
    hpx::supervision::remove_target(peer_sentinel.get_id());
}

int hpx_main()
{
    test_epoch_after_init();
    test_publish_then_query_round_trip();
    test_query_state_peer_overload_uses_peer_not_handle();
    test_query_state_reflects_peer_terminal_state();
    test_sync_vs_async_equivalence();

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
