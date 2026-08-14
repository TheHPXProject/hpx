//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression checks for register_observers()'s mirroring callback (see
// registry_server.cpp): every mirrored event is re-published onto peer_locality
// via publish_event_no_notify() rather than publish_event(), specifically so
// that a notifying re-publish into that same local supervision_manager entry
// cannot synchronously re-invoke this very observer. Two things must hold as a
// result:
//
//   (a) this can never recurse/hang, even in the structural edge case where
//       the peer being mirrored is `here` itself (publish_event()/
//       publish_event_no_notify() for a given target id always resolve to the
//       same local supervision_manager singleton, regardless of what locality
//       that id nominally names);
//   (b) the mirrored shadow's event_sequence_number keeps advancing on every
//       event observed within a single epoch, rather than freezing after the
//       first one.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/supervision_dispatch.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

    // Long enough to comfortably absorb scheduler jitter; short enough that a
    // genuine hang (rather than a slow-but-completing run) still fails the test
    // promptly instead of relying on the overall ctest timeout.
    constexpr std::chrono::seconds test_mirroring_timeout{10};

}    // namespace

// See registry_join.cpp's reset_shared_shadow_state(): hpx::supervision keys
// its local per-target lifecycle state purely by hpx::id_type value, so
// `here`'s state must be reset before (re-)using it as a peer sentinel below.
void reset_shared_shadow_state()
{
    hpx::error_code ec(hpx::throwmode::lightweight);
    hpx::supervision::remove_target(hpx::find_here(), ec);
    HPX_TEST(!ec);
}

// Test A: same-locality mirroring completes without recursion/hang.
//
// Joins `here` against itself -- i.e. peer_locality == here, exactly the
// structural edge case that caused the original self-recursion bug, since
// publish_event()/publish_event_no_notify() for a given target id always
// resolve to the same local supervision_manager singleton regardless of the
// locality that id nominally names. Publishes a handful of events (started ->
// running -> running -> completed) into the mirrored shadow and asserts the
// whole sequence completes within a bounded timeout -- wrapped in hpx::async +
// wait_for() -- rather than hanging or overflowing the stack via recursive
// self-notification, and that the shadow's final state matches the last
// published event.
void test_same_locality_mirroring_completes_without_recursion()
{
    reset_shared_shadow_state();

    hpx::id_type const& here = hpx::find_here();
    hpx::supervision::registry const r(here);

    auto const [target, join_epoch] = r.join(hpx::launch::sync, here);
    HPX_TEST_EQ(target, here);

    hpx::future<void> f = hpx::async([here, join_epoch]() {
        hpx::supervision::publish_event(
            here, hpx::supervision::event::started, join_epoch);
        hpx::supervision::publish_event(
            here, hpx::supervision::event::running, join_epoch);
        hpx::supervision::publish_event(
            here, hpx::supervision::event::running, join_epoch);
        hpx::supervision::publish_event(
            here, hpx::supervision::event::completed, join_epoch);
    });

    auto const status = f.wait_for(test_mirroring_timeout);
    HPX_TEST(status == hpx::future_status::ready);
    if (status != hpx::future_status::ready)
    {
        return;
    }

    auto const shadow_state = hpx::supervision::query_state(target);
    HPX_TEST(shadow_state.last_event == hpx::supervision::event::completed);
}

// Test B: cross-locality event_sequence_number strictly increasing within one
// epoch.
//
// Joins a peer sentinel on a different (real, remote) locality than `here`,
// then publishes several consecutive `running` events for it, all within the
// same epoch. After each publish, queries the mirrored shadow's
// event_sequence_number and records it; the recorded sequence must be strictly
// increasing, directly catching the "frozen after first event" regression
// rather than only observing it indirectly via later fencing.
void test_cross_locality_sequence_number_increasing(
    hpx::id_type const& peer_locality)
{
    hpx::supervision::registry const r(hpx::find_here());

    hpx::supervision::joined_peer const peer =
        r.join(hpx::launch::sync, peer_locality);
    HPX_TEST_NEQ(peer.target, hpx::invalid_id);

    constexpr int num_events = 5;
    std::vector<std::uint64_t> sequence_numbers;
    sequence_numbers.reserve(num_events);

    for (int i = 0; i != num_events; ++i)
    {
        hpx::supervision::publish_event(
            peer_locality, hpx::supervision::event::running, peer.join_epoch);

        // The mirroring observer callback (see register_observers() in
        // registry_server.cpp) runs asynchronously with respect to this
        // publish_event() call; poll the shadow until its event_sequence_number
        // has advanced past whatever was recorded on the previous iteration,
        // rather than assuming the mirror has already landed by the time
        // publish_event() returns.
        std::uint64_t const previous =
            sequence_numbers.empty() ? 0 : sequence_numbers.back();

        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        hpx::supervision::lifecycle_state state =
            hpx::supervision::query_state(peer.target);
        while (state.event_sequence_number <= previous &&
            std::chrono::steady_clock::now() < deadline)
        {
            hpx::this_thread::yield();
            state = hpx::supervision::query_state(peer.target);
        }

        sequence_numbers.push_back(state.event_sequence_number);
    }

    HPX_TEST_EQ(sequence_numbers.size(), static_cast<std::size_t>(num_events));
    for (std::size_t i = 1; i != sequence_numbers.size(); ++i)
    {
        HPX_TEST(sequence_numbers[i] > sequence_numbers[i - 1]);
    }
}

// ============================================================================
// Main Test Entry Point
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

    HPX_TEST_NO_THROW(hpx::supervision::init(hpx::launch::sync));
    hpx::distributed::barrier::synchronize();

    if (is_observer)
    {
        test_same_locality_mirroring_completes_without_recursion();
        test_cross_locality_sequence_number_increasing(peer_locality);
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
