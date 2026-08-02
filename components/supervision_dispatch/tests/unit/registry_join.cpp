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

#include <hpx/supervision_dispatch/registry.hpp>
#include <hpx/supervision_dispatch/sentinel.hpp>
#include <hpx/supervision_dispatch/server/registry.hpp>

#include <cstddef>
#include <vector>

// ============================================================================
// Test Cases
// ============================================================================

// Joining a peer sentinel must succeed and return a valid shadow target id that
// is distinct from the peer sentinel's own id, confirming that a local shadow
// was created rather than simply aliasing the peer.
void test_registry_join_creates_shadow()
{
    hpx::supervision::sentinel const peer(hpx::find_here());
    hpx::supervision::registry const r(hpx::find_here());

    hpx::id_type const shadow =
        r.join(hpx::launch::sync, peer, hpx::find_here());

    HPX_TEST_NEQ(shadow, hpx::invalid_id);
    HPX_TEST_NEQ(shadow, peer.get_id());
}

// join() is idempotent for a given peer sentinel: joining the same peer twice
// must return the same shadow id, confirming that the peer -> shadow mapping is
// tracked in registry state rather than recreated on every call.
void test_registry_join_idempotent()
{
    hpx::supervision::sentinel const peer(hpx::find_here());
    hpx::supervision::registry const r(hpx::find_here());

    hpx::id_type const shadow1 =
        r.join(hpx::launch::sync, peer, hpx::find_here());
    hpx::id_type const shadow2 =
        r.join(hpx::launch::sync, peer, hpx::find_here());

    HPX_TEST_EQ(shadow1, shadow2);
}

// Same as test_registry_join_creates_shadow(), but using the asynchronous
// overload of join().
void test_registry_join_async()
{
    hpx::supervision::sentinel const peer(hpx::find_here());
    hpx::supervision::registry const r(hpx::find_here());

    hpx::future<hpx::id_type> f = r.join(peer, hpx::find_here());
    hpx::id_type const shadow = f.get();

    HPX_TEST_NEQ(shadow, hpx::invalid_id);
}

// Two different registries joining two different peer sentinels must be tracked
// independently and get distinct shadow ids.
void test_registry_join_distinct_peers()
{
    hpx::supervision::sentinel const peer1(hpx::find_here());
    hpx::supervision::sentinel const peer2(hpx::find_here());
    hpx::supervision::registry const r(hpx::find_here());

    hpx::id_type const shadow1 =
        r.join(hpx::launch::sync, peer1, hpx::find_here());
    hpx::id_type const shadow2 =
        r.join(hpx::launch::sync, peer2, hpx::find_here());

    HPX_TEST_NEQ(shadow1, shadow2);
}

// join() seeds the local shadow with `event::started`, giving it a
// well-defined starting point in the lifecycle state machine before any
// notification for the peer arrives.
void test_registry_join_seeds_shadow_started()
{
    hpx::supervision::sentinel const peer(hpx::find_here());
    hpx::supervision::registry const r(hpx::find_here());

    hpx::id_type const shadow =
        r.join(hpx::launch::sync, peer, hpx::find_here());

    auto const seeded_state = hpx::supervision::query_state(shadow);
    HPX_TEST(seeded_state.last_event == hpx::supervision::event::started);
}

// Once a joined peer sentinel reaches the terminal `failed` event, the
// registry's lifecycle observer (registered as part of join()) must
// re-publish that event, at the same epoch, onto the local shadow, so that
// querying the shadow's state reflects the peer's terminal state.
void test_registry_join_mirrors_failed_event_on_shadow()
{
    hpx::supervision::sentinel const peer(hpx::find_here());
    hpx::supervision::registry const r(hpx::find_here());

    hpx::id_type const shadow =
        r.join(hpx::launch::sync, peer, hpx::find_here());

    // Bring the peer sentinel to a valid prior state (`started`); this is
    // required by hpx::supervision's own lifecycle state machine and is
    // independent of the shadow mirroring under test here.
    peer.start(hpx::launch::sync);

    hpx::supervision::publish_event(hpx::launch::sync, hpx::find_here(),
        peer.get_id(), hpx::supervision::event::failed, 0);

    auto const shadow_state = hpx::supervision::query_state(shadow);
    HPX_TEST(shadow_state.last_event == hpx::supervision::event::failed);
}

// Same as above, but for the terminal `completed` event, which (unlike
// `failed`) is only reachable from `running`/`suspending`; the registry's
// observer must bridge through an intermediate `running` transition on the
// shadow so the mirrored `completed` event is accepted.
void test_registry_join_mirrors_completed_event_on_shadow()
{
    hpx::supervision::sentinel const peer(hpx::find_here());
    hpx::supervision::registry const r(hpx::find_here());

    hpx::id_type const shadow =
        r.join(hpx::launch::sync, peer, hpx::find_here());

    peer.start(hpx::launch::sync);

    hpx::supervision::publish_event(hpx::launch::sync, hpx::find_here(),
        peer.get_id(), hpx::supervision::event::running, 0);
    hpx::supervision::publish_event(hpx::launch::sync, hpx::find_here(),
        peer.get_id(), hpx::supervision::event::completed, 0);

    auto const shadow_state = hpx::supervision::query_state(shadow);
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
    hpx::supervision::sentinel const peer(hpx::find_here());
    hpx::supervision::registry const r(hpx::find_here());

    constexpr std::size_t num_joiners = 64;

    std::vector<hpx::future<hpx::id_type>> futures;
    futures.reserve(num_joiners);
    for (std::size_t i = 0; i != num_joiners; ++i)
    {
        futures.push_back(r.join(peer, hpx::find_here()));
    }

    std::vector<hpx::id_type> const shadows = hpx::unwrap(std::move(futures));

    // Every concurrent joiner must observe the same, valid shadow id.
    HPX_TEST_NEQ(shadows.front(), hpx::invalid_id);
    HPX_TEST_NEQ(shadows.front(), peer.get_id());
    for (hpx::id_type const& shadow : shadows)
    {
        HPX_TEST_EQ(shadow, shadows.front());
    }

    // The winning reservation must have fully seeded the shadow's lifecycle
    // state; a losing/re-entrant caller returning early on a
    // half-constructed entry would leave this unset or inconsistent.
    auto const seeded_state = hpx::supervision::query_state(shadows.front());
    HPX_TEST(seeded_state.last_event == hpx::supervision::event::started);
}

// If register_observers() fails during join() -- e.g. because the peer's
// locality could not be validated -- the shadow minted and seeded just before
// that call must not be left behind: the failure path must remove its locally
// tracked supervision state, not just release the peer's reservation in peers_.
// hpx::supervision::server::detail::last_join_shadow() is testing
// infrastructure support that lets us retrieve the shadow that was minted for
// the failed attempt, since join() itself never hands it back to the caller on
// failure.
void test_registry_join_failure_removes_shadow_state()
{
    hpx::supervision::sentinel const peer(hpx::find_here());
    hpx::supervision::registry const r(hpx::find_here());

    // An invalid peer locality makes hpx::supervision::register_observer()
    // reject the call outright (see registry::register_observers()), so join()
    // fails before either observer is actually registered -- but only after
    // already minting and seeding a shadow for it.
    hpx::error_code ec;
    hpx::id_type const shadow =
        r.join(hpx::launch::sync, peer, hpx::invalid_id, ec);

    HPX_TEST(ec);
    HPX_TEST_EQ(shadow, hpx::invalid_id);

    hpx::id_type const failed_shadow =
        hpx::supervision::server::detail::last_join_shadow();
    HPX_TEST_NEQ(failed_shadow, hpx::invalid_id);

    // The failed attempt's shadow must have had its local state removed by
    // register_observers()'s catch block: querying it now must report the same
    // "never seen this target" result as a target that never published
    // anything, rather than the `started` event join() seeded it with.
    auto const state = hpx::supervision::query_state(failed_shadow);
    HPX_TEST(state.last_event == hpx::supervision::event::unknown);
    HPX_TEST(state.ec);

    // The failed reservation must also have been released: retrying with a
    // valid locality must succeed and mint a fresh shadow.
    hpx::id_type const retried_shadow =
        r.join(hpx::launch::sync, peer, hpx::find_here());
    HPX_TEST_NEQ(retried_shadow, hpx::invalid_id);
    HPX_TEST_NEQ(retried_shadow, failed_shadow);
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
    hpx::supervision::sentinel const peer(hpx::find_here());
    hpx::supervision::registry const r(hpx::find_here());

    hpx::id_type const shadow =
        r.join(hpx::launch::sync, peer, hpx::find_here());

    peer.start(hpx::launch::sync);
    hpx::supervision::publish_event(hpx::launch::sync, hpx::find_here(),
        peer.get_id(), hpx::supervision::event::failed, 0);

    // Eviction is dispatched asynchronously (via hpx::post()) once the terminal
    // publish above has completed, so poll re-joining until the peer's entry is
    // actually gone rather than assuming this has already happened by the time
    // publish_event() returns.
    hpx::id_type rejoined_shadow;
    for (int i = 0; i != 10000; ++i)
    {
        rejoined_shadow = r.join(hpx::launch::sync, peer, hpx::find_here());
        if (rejoined_shadow != shadow)
        {
            break;
        }
        hpx::this_thread::yield();
    }

    HPX_TEST_NEQ(rejoined_shadow, shadow);
}

// ============================================================================
// Main Test Entry Point
// ============================================================================
int hpx_main()
{
    test_registry_join_creates_shadow();
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
    test_registry_join_failure_removes_shadow_state();
    test_registry_join_terminal_peer_evicted_from_peers();

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
