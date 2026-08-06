//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_main.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/supervision_dispatch.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>

struct test_server : hpx::components::component_base<test_server>
{
};

using server_type = hpx::components::component<test_server>;
HPX_REGISTER_COMPONENT(server_type, test_server)

// Verifies that after a single successful join(), snapshot_peers() reports
// exactly that peer, with peer_sentinel/peer_locality/shadow matching what
// join() recorded and returned. This is the basic contract the
// failure-detection poller depends on: one join -> one visible entry.
void test_snapshot_peers_reports_joined_peer()
{
    hpx::id_type const peer_locality = hpx::find_here();
    hpx::supervision::registry const r(peer_locality);
    hpx::supervision::sentinel const peer_sentinel(peer_locality);

    // join() both creates the shadow and (per this task) now persists
    // peer_locality into the entry so snapshot_peers() can report it.
    hpx::supervision::joined_peer const peer =
        r.join(hpx::launch::sync, peer_sentinel, peer_locality);

    auto const snapshot = r.snapshot_peers(hpx::launch::sync);
    HPX_TEST_EQ(snapshot.size(), static_cast<std::size_t>(1));
    if (!snapshot.empty())
    {
        HPX_TEST(snapshot[0].peer_sentinel == peer_sentinel.get_id());
        HPX_TEST(snapshot[0].peer_locality == peer_locality);
        HPX_TEST(snapshot[0].shadow == peer.shadow);
    }
}

// Verifies that snapshot_peers() aggregates all currently joined peers, not
// just the most recent one -- important because a poller needs to iterate
// the full peer set each cycle, not a single entry.
void test_snapshot_peers_reports_multiple_peers()
{
    hpx::id_type const peer_locality = hpx::find_here();
    hpx::supervision::registry const r(peer_locality);
    hpx::supervision::sentinel const p1(peer_locality);
    hpx::supervision::sentinel const p2(peer_locality);

    r.join(hpx::launch::sync, p1, peer_locality);
    r.join(hpx::launch::sync, p2, peer_locality);

    auto const snapshot = r.snapshot_peers(hpx::launch::sync);
    HPX_TEST_EQ(snapshot.size(), static_cast<std::size_t>(2));

    auto const contains_sentinel = [&](hpx::id_type const& expected) {
        return std::ranges::any_of(snapshot,
            [&](auto const& peer) { return peer.peer_sentinel == expected; });
    };

    HPX_TEST(contains_sentinel(p1.get_id()));
    HPX_TEST(contains_sentinel(p2.get_id()));
}

// Verifies that a peer which has reached a terminal lifecycle event and is
// evicted no longer appears in snapshot_peers(). Eviction is driven
// asynchronously via hpx::post() from the lifecycle-observer callback (see
// evict_peer()'s comment in registry.hpp), so this polls briefly instead of
// asserting immediately after publish_event() returns. This is the
// exclusion case snapshot_peers() exists to guarantee: a poller must never
// be handed a peer that is already tearing down.
void test_snapshot_peers_excludes_evicted_peer()
{
    hpx::id_type const peer_locality = hpx::find_here();
    hpx::supervision::registry const r(peer_locality);
    hpx::supervision::sentinel const peer_sentinel(peer_locality);

    r.join(hpx::launch::sync, peer_sentinel, peer_locality);
    HPX_TEST_EQ(r.snapshot_peers(hpx::launch::sync).size(),
        static_cast<std::size_t>(1));

    // Drive the peer to a terminal state so its lifecycle observer fires
    // and evict_peer() is posted.
    hpx::error_code ec(hpx::throwmode::lightweight);

    hpx::supervision::publish_event(hpx::launch::sync, peer_locality,
        peer_sentinel.get_id(), hpx::supervision::event::started, 0, ec);
    hpx::supervision::publish_event(hpx::launch::sync, peer_locality,
        peer_sentinel.get_id(), hpx::supervision::event::running, 0, ec);

    hpx::supervision::publish_event(hpx::launch::sync, peer_locality,
        peer_sentinel.get_id(), hpx::supervision::event::completed, 0, ec);

    // Eviction runs via hpx::post(); poll (bounded) until it lands rather
    // than asserting synchronously.
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!r.snapshot_peers(hpx::launch::sync).empty() &&
        std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::yield();
    }
    HPX_TEST(r.snapshot_peers(hpx::launch::sync).empty());
}

int main()
{
    test_snapshot_peers_reports_joined_peer();
    test_snapshot_peers_reports_multiple_peers();
    test_snapshot_peers_excludes_evicted_peer();

    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
