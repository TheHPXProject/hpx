//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression tests for #7924: a lifecycle transition of a fenced peer that
// happens between registry::evict_fenced()'s terminal check and its removal of
// the peer's entry must not let the eviction remove an entry whose local state
// has become active again. testing::set_evict_fenced_hook_for_testing() runs
// code in exactly that window, so the interleavings below are deterministic.
//
// The peer is this locality itself, so the lifecycle observer join()
// registered is notified synchronously by a local publish_event(), and a
// failure-detection fence is simulated by publish_event_no_notify(), which
// latches the terminal state without notifying that observer, the same way a
// fence by failure detection never reaches the peer's own observers.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_main.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading.hpp>

#include <hpx/supervision_dispatch.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

    void evict_fenced_sync(hpx::supervision::registry const& r,
        hpx::id_type const& peer_locality, std::uint64_t const generation)
    {
        // registry::evict_fenced() is fire-and-forget, wait for it here
        hpx::async<hpx::supervision::server::registry::evict_fenced_action>(
            r.get_id(), peer_locality, generation)
            .get();
    }

    std::vector<hpx::supervision::server::peer_snapshot> snapshot(
        hpx::supervision::registry const& r)
    {
        return r.snapshot_peers(hpx::launch::sync);
    }

    hpx::supervision::lifecycle_state local_state(
        hpx::id_type const& peer_locality)
    {
        hpx::error_code ec(hpx::throwmode::lightweight);
        auto state = hpx::supervision::query_state(peer_locality, ec);
        HPX_TEST(!ec);
        return state;
    }

    void publish(hpx::id_type const& peer_locality,
        hpx::supervision::event const ev, std::uint64_t const epoch)
    {
        hpx::error_code ec(hpx::throwmode::lightweight);
        hpx::supervision::publish_event(peer_locality, ev, epoch, ec);
        HPX_TEST(!ec);
    }

    // Latches event::failed on peer_locality's local state, as failure
    // detection does, without the peer's lifecycle observer seeing it.
    void fence(hpx::id_type const& peer_locality, std::uint64_t const epoch)
    {
        hpx::error_code ec(hpx::throwmode::lightweight);
        hpx::supervision::publish_event_no_notify(
            peer_locality, hpx::supervision::event::failed, epoch, ec);
        HPX_TEST(!ec);
        HPX_TEST(hpx::supervision::is_terminal(
            local_state(peer_locality).last_event));
    }

    struct joined
    {
        // the generation of the peer's new registry entry
        std::uint64_t generation;

        // the epoch the peer's local state was seeded at
        std::uint64_t epoch;
    };

    joined join(
        hpx::supervision::registry const& r, hpx::id_type const& peer_locality)
    {
        r.join(hpx::launch::sync, peer_locality);

        auto const peers = snapshot(r);
        HPX_TEST_EQ(peers.size(), static_cast<std::size_t>(1));
        std::uint64_t const generation =
            peers.empty() ? 0 : peers.front().generation;

        auto const state = local_state(peer_locality);
        HPX_TEST(state.last_event == hpx::supervision::event::started);
        return {.generation = generation, .epoch = state.epoch};
    }

    void wait_until_evicted(hpx::supervision::registry const& r)
    {
        // Reactive eviction runs via hpx::post() from the lifecycle observer.
        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (
            !snapshot(r).empty() && std::chrono::steady_clock::now() < deadline)
        {
            hpx::this_thread::yield();
        }
        HPX_TEST(snapshot(r).empty());
    }

    struct reset_hook
    {
        ~reset_hook()
        {
            hpx::supervision::testing::set_evict_fenced_hook_for_testing({});
        }
    };
}    // namespace

// The peer comes back with a newer epoch after evict_fenced() found it fenced
// but before it removed the entry. The newer lifecycle wins: the entry stays,
// its local state stays active, and its observers stay registered, so the
// peer is still monitored.
void test_newer_lifecycle_between_check_and_removal_wins()
{
    hpx::id_type const here = hpx::find_here();
    hpx::supervision::registry const r(here);

    joined const peer = join(r, here);
    std::uint64_t const generation = peer.generation;
    std::uint64_t const epoch = peer.epoch;
    fence(here, epoch);

    bool hook_ran = false;
    reset_hook const reset;
    hpx::supervision::testing::set_evict_fenced_hook_for_testing([&] {
        hook_ran = true;
        publish(here, hpx::supervision::event::started, epoch + 1);
    });

    evict_fenced_sync(r, here, generation);
    hpx::supervision::testing::set_evict_fenced_hook_for_testing({});
    HPX_TEST(hook_ran);

    auto const peers = snapshot(r);
    HPX_TEST_EQ(peers.size(), static_cast<std::size_t>(1));
    if (!peers.empty())
    {
        HPX_TEST_EQ(peers.front().generation, generation);
    }

    auto const state = local_state(here);
    HPX_TEST(state.last_event == hpx::supervision::event::started);
    HPX_TEST_EQ(state.epoch, epoch + 1);

    // The lifecycle observer is still registered: a terminal event the peer
    // reports for its new epoch evicts it through the reactive path.
    publish(here, hpx::supervision::event::running, epoch + 1);
    publish(here, hpx::supervision::event::completed, epoch + 1);
    wait_until_evicted(r);
}

// Nothing happens between the terminal check and the removal, so the eviction
// wins: the peer is removed, and its fence stays in place as a tombstone. A
// later join() creates a new entry, which is what restores monitoring.
void test_eviction_without_newer_lifecycle_keeps_tombstone()
{
    hpx::id_type const here = hpx::find_here();
    hpx::supervision::registry const r(here);

    joined const peer = join(r, here);
    std::uint64_t const generation = peer.generation;
    std::uint64_t const epoch = peer.epoch;
    fence(here, epoch);

    evict_fenced_sync(r, here, generation);
    HPX_TEST(snapshot(r).empty());

    auto const state = local_state(here);
    HPX_TEST(state.last_event == hpx::supervision::event::failed);
    HPX_TEST_EQ(state.epoch, epoch);

    // Evicting it again is a no-op.
    evict_fenced_sync(r, here, generation);
    HPX_TEST(snapshot(r).empty());

    r.join(hpx::launch::sync, here);
    auto const peers = snapshot(r);
    HPX_TEST_EQ(peers.size(), static_cast<std::size_t>(1));
    if (!peers.empty())
    {
        HPX_TEST_NEQ(peers.front().generation, generation);
    }
    HPX_TEST(!hpx::supervision::is_terminal(local_state(here).last_event));

    // The new entry is monitored again.
    std::uint64_t const new_epoch = local_state(here).epoch;
    publish(here, hpx::supervision::event::running, new_epoch);
    publish(here, hpx::supervision::event::completed, new_epoch);
    wait_until_evicted(r);
}

// A newer lifecycle is mirrored between the check and the removal, but is
// fenced again before the removal. evict_fenced() backs off, since it cannot
// tell from its earlier check which state is current. A peer that stays
// fenced is evicted by the next call, as failure detection makes on its next
// sweep, and its newer fence stays in place as the tombstone.
void test_peer_that_stays_fenced_is_evicted_by_next_call()
{
    hpx::id_type const here = hpx::find_here();
    hpx::supervision::registry const r(here);

    joined const peer = join(r, here);
    std::uint64_t const generation = peer.generation;
    std::uint64_t const epoch = peer.epoch;
    fence(here, epoch);

    reset_hook const reset;
    hpx::supervision::testing::set_evict_fenced_hook_for_testing([&] {
        publish(here, hpx::supervision::event::started, epoch + 1);
        fence(here, epoch + 1);
    });

    evict_fenced_sync(r, here, generation);
    hpx::supervision::testing::set_evict_fenced_hook_for_testing({});
    HPX_TEST_EQ(snapshot(r).size(), static_cast<std::size_t>(1));

    evict_fenced_sync(r, here, generation);
    HPX_TEST(snapshot(r).empty());

    auto const state = local_state(here);
    HPX_TEST(state.last_event == hpx::supervision::event::failed);
    HPX_TEST_EQ(state.epoch, epoch + 1);
}

int main()
{
    test_newer_lifecycle_between_check_and_removal_wins();
    test_eviction_without_newer_lifecycle_keeps_tombstone();
    test_peer_that_stays_fenced_is_evicted_by_next_call();

    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
