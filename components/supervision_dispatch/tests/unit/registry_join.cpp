//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Acceptance check: a registry client can join a peer sentinel -- creating a
// local shadow target, registering as an observer of the peer's lifecycle/
// activity events via the public hpx::supervision API, and re-publishing
// terminal notifications for the peer onto the local shadow -- with no
// compile/link errors. This test only exercises join() with explicit,
// manually-supplied peer ids; it deliberately never exercises name discovery.

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/supervision_dispatch.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

// ============================================================================
// Test Cases
// ============================================================================

// Every test below uses hpx::find_here() as the peer sentinel's locality;
// registry::join() derives peer_locality from that same id (see
// sentinel::get_locality()), and registry::register_observers() (see
// registry_server.cpp) seeds/mirrors the peer's lifecycle state directly onto
// that peer_locality id. hpx::supervision keys its local per-target
// lifecycle state (including the current epoch) purely by hpx::id_type
// value. Without resetting that shared state, whatever epoch/event one test
// leaves behind on `here` - in particular a terminal event, which bumps the
// *next* join's seed epoch via `local_state.epoch + 1` - leaks into and
// corrupts whichever test runs next in this binary. Call this at the start
// of every test function below to guarantee each one starts from a clean,
// "never seen this target" state.
void reset_shared_shadow_state()
{
    hpx::error_code ec(hpx::throwmode::lightweight);
    hpx::supervision::remove_target(hpx::find_here(), ec);
    HPX_TEST(!ec);
}

// Joining a peer locality must create or reuse local mirrored state and return
// the peer locality as the joined target.
void test_registry_join_creates_state()
{
    reset_shared_shadow_state();

    auto const here = hpx::find_here();
    hpx::supervision::registry const r(here);

    auto const [target, _] = r.join(hpx::launch::sync, here);

    HPX_TEST_EQ(target, here);
    HPX_TEST_NEQ(target, hpx::invalid_id);
}

// join() is idempotent for a given peer sentinel: joining the same peer twice
// must return the same shadow id, confirming that the peer -> shadow mapping is
// tracked in registry state rather than recreated on every call.
void test_registry_join_idempotent()
{
    reset_shared_shadow_state();

    hpx::id_type const& here = hpx::find_here();
    hpx::supervision::registry const r(here);

    hpx::supervision::joined_peer const peer1 = r.join(hpx::launch::sync, here);
    hpx::supervision::joined_peer const peer2 = r.join(hpx::launch::sync, here);

    HPX_TEST_EQ(peer1, peer2);
}

// Same as test_registry_join_creates_state(), but using the asynchronous
// overload of join().
void test_registry_join_async()
{
    reset_shared_shadow_state();

    hpx::id_type const& here = hpx::find_here();
    hpx::supervision::registry const r(here);

    hpx::future<hpx::supervision::joined_peer> f = r.join(here);
    hpx::supervision::joined_peer const peer = f.get();

    HPX_TEST_NEQ(peer.target, hpx::invalid_id);
}

// Two distinct peer sentinels joined against the same registry (even via the
// same peer_locality) must be tracked independently: each gets its own entry in
// the registry's internal peers_ map. `joined_peer` itself is not a reliable
// witness for this - its `target` field is peer_locality-derived rather than
// peer-derived (so it is identical for both joins here), and its `join_epoch`
// field is seeded from the peer's own published state (so it is also identical
// - both epoch 0 - for two freshly joined sentinels). Distinct per-sentinel
// tracking is instead verified via snapshot_peers(), which exposes one entry
// per tracked peer_sentinel.
void test_registry_join_distinct_peers()
{
    reset_shared_shadow_state();

    hpx::id_type const& here = hpx::find_here();

    hpx::supervision::registry const r(hpx::find_here());

    r.join(hpx::launch::sync, here);
    r.join(hpx::launch::sync, here);

    std::vector<hpx::supervision::server::peer_snapshot> const peers =
        r.snapshot_peers(hpx::launch::sync);

    HPX_TEST_EQ(peers.size(), static_cast<std::size_t>(1));

    bool const found_peer1 = std::ranges::find_if(peers, [&](auto const& peer) {
        return peer.peer_locality == here;
    }) != peers.end();

    HPX_TEST(found_peer1);
}

// join() seeds the local shadow with `event::started`, giving it a
// well-defined starting point in the lifecycle state machine before any
// notification for the peer arrives.
void test_registry_join_seeds_shadow_started()
{
    reset_shared_shadow_state();

    hpx::id_type const& here = hpx::find_here();

    hpx::supervision::registry const r(here);

    auto const [target, _] = r.join(hpx::launch::sync, here);

    auto const seeded_state = hpx::supervision::query_state(target);
    HPX_TEST(seeded_state.last_event == hpx::supervision::event::started);
}

// Once a joined peer sentinel reaches the terminal `failed` event, the
// registry's lifecycle observer (registered as part of join()) must
// re-publish that event, at the same epoch, onto the local shadow, so that
// querying the shadow's state reflects the peer's terminal state.
void test_registry_join_mirrors_failed_event_on_shadow()
{
    reset_shared_shadow_state();

    hpx::id_type const& here = hpx::find_here();

    hpx::supervision::registry const r(here);

    auto const [target, _] = r.join(hpx::launch::sync, here);

    // Bring the peer sentinel to a valid prior state (`started`); this is
    // required by hpx::supervision's own lifecycle state machine and is
    // independent of the shadow mirroring under test here.
    hpx::supervision::publish_event(here, hpx::supervision::event::started, 0);

    // sentinel::start()/publish_event() publish under the sentinel's own
    // hosting locality (hpx::find_here(), evaluated on that locality) rather
    // than the sentinel's own component id; register_observers() now watches
    // that same locality-keyed target, so the simulated `failed` event below
    // must target it too for the registry's lifecycle observer to fire.
    hpx::supervision::publish_event(here, hpx::supervision::event::failed, 0);

    auto const shadow_state = hpx::supervision::query_state(target);
    HPX_TEST(shadow_state.last_event == hpx::supervision::event::failed);
}

// Same as above, but for the terminal `completed` event, which (unlike
// `failed`) is only reachable from `running`/`suspending`; the registry's
// observer must bridge through an intermediate `running` transition on the
// shadow so the mirrored `completed` event is accepted.
void test_registry_join_mirrors_completed_event_on_shadow()
{
    reset_shared_shadow_state();

    hpx::id_type const& here = hpx::find_here();

    hpx::supervision::registry const r(here);

    auto const [target, _] = r.join(hpx::launch::sync, here);

    hpx::supervision::publish_event(here, hpx::supervision::event::started, 0);

    // See test_registry_join_mirrors_failed_event_on_shadow() above for why
    // these target hpx::find_here() rather than peer_sentinel.get_id().
    hpx::supervision::publish_event(
        hpx::find_here(), hpx::supervision::event::running, 0);
    hpx::supervision::publish_event(
        hpx::find_here(), hpx::supervision::event::completed, 0);

    auto const shadow_state = hpx::supervision::query_state(target);
    HPX_TEST(shadow_state.last_event == hpx::supervision::event::completed);
}

// Several concurrent calls to join() for the *same* peer sentinel from the same
// registry must race safely on the "reserve ownership" path in registry_server:
// exactly one caller creates the shadow/observers, and every other concurrent
// caller must block until that reservation completes and then observe the very
// same shadow id -- never a partially-constructed state, a duplicate shadow, or
// a deadlock.
void test_registry_join_concurrent_race()
{
    reset_shared_shadow_state();

    hpx::id_type const& here = hpx::find_here();

    hpx::supervision::registry const r(here);

    constexpr std::size_t num_joiners = 64;

    std::vector<hpx::future<hpx::supervision::joined_peer>> futures;
    futures.reserve(num_joiners);
    for (std::size_t i = 0; i != num_joiners; ++i)
    {
        futures.push_back(r.join(here));
    }

    std::vector<hpx::supervision::joined_peer> const peers =
        hpx::unwrap(std::move(futures));

    // Every concurrent joiner must observe the same, valid shadow id.
    HPX_TEST_EQ(peers.size(), num_joiners);
    if (!peers.empty())
    {
        HPX_TEST_NEQ(peers.front().target, hpx::invalid_id);
        for (hpx::supervision::joined_peer const& peer : peers)
        {
            HPX_TEST_EQ(peer, peers.front());
        }

        // The winning reservation must have fully seeded the shadow's lifecycle
        // state; a losing/re-entrant caller returning early on a
        // half-constructed entry would leave this unset or inconsistent.
        auto const seeded_state =
            hpx::supervision::query_state(peers.front().target);
        HPX_TEST(seeded_state.last_event == hpx::supervision::event::started);
    }
}

// Once a joined peer sentinel reaches a terminal lifecycle event, its entry
// must be erased from the registry's peers_ map (see registry::evict_peer())
// instead of accumulating there indefinitely. Since peers_ is private registry
// state, this is observed indirectly: re-joining the same peer sentinel
// afterward must mint a fresh shadow (and re-run the full registration path)
// rather than returning the very same shadow that was created before the
// terminal event -- which is exactly what would happen if the (now-terminal)
// entry were still sitting in peers_.
void test_registry_join_terminal_peer_evicted_from_peers()
{
    reset_shared_shadow_state();

    hpx::id_type const& here = hpx::find_here();

    hpx::supervision::registry const r(here);

    hpx::supervision::joined_peer const peer = r.join(hpx::launch::sync, here);

    hpx::supervision::publish_event(here, hpx::supervision::event::started, 0);

    // See test_registry_join_mirrors_failed_event_on_shadow() above for why
    // this targets hpx::find_here() rather than peer_sentinel.get_id().
    hpx::supervision::publish_event(
        hpx::find_here(), hpx::supervision::event::failed, 0);

    // Eviction is dispatched asynchronously (via hpx::post()) once the terminal
    // publish above has completed, so poll re-joining until the peer's entry is
    // actually gone rather than assuming this has already happened by the time
    // publish_event() returns.
    hpx::supervision::joined_peer rejoined_shadow;
    for (int i = 0; i != 10000; ++i)
    {
        rejoined_shadow = r.join(hpx::launch::sync, here);
        if (rejoined_shadow != peer)
        {
            break;
        }
        hpx::this_thread::yield();
    }

    HPX_TEST_NEQ(rejoined_shadow, peer);
}

// A peer_locality that does not represent a locality id must be rejected with
// hpx::error::bad_parameter, and rejected *before* any registration side
// effects occur - no shadow entry should be seeded/published, and
// snapshot_peers() must stay empty.
void test_registry_join_rejects_non_locality()
{
    reset_shared_shadow_state();

    hpx::id_type const& here = hpx::find_here();
    hpx::supervision::registry const r(here);

    // use invalid_id to try to join "as if" it were a peer locality.
    bool threw_bad_parameter = false;
    try
    {
        r.join(hpx::launch::sync, hpx::invalid_id);
        HPX_TEST(false);
    }
    catch (hpx::exception const& e)
    {
        threw_bad_parameter = (e.get_error() == hpx::error::bad_parameter);
    }
    HPX_TEST(threw_bad_parameter);

    // No new peer entry should have been created for the rejected id.
    auto const peers = r.snapshot_peers(hpx::launch::sync);
    HPX_TEST(peers.empty());
}

// ============================================================================
// Main Test Entry Point
// ============================================================================
int hpx_main()
{
    test_registry_join_creates_state();
    test_registry_join_idempotent();
    test_registry_join_async();
    test_registry_join_distinct_peers();
    test_registry_join_seeds_shadow_started();
    test_registry_join_mirrors_failed_event_on_shadow();
    test_registry_join_mirrors_completed_event_on_shadow();
    for (int i = 0; i < 5; ++i)
    {
        test_registry_join_concurrent_race();
    }
    test_registry_join_terminal_peer_evicted_from_peers();
    test_registry_join_rejects_non_locality();

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
