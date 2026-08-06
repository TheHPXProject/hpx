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
        hpx::supervision::query_state(peer->shadow.get()).epoch, join_epoch);

    bool const fenced = wait_until_fenced(
        peer->shadow.get(), join_epoch, std::chrono::seconds(5));
    HPX_TEST(fenced);

    HPX_TEST(
        hpx::supervision::check_admission(peer->shadow.get(), join_epoch) ==
        hpx::supervision::dispatch_outcome::rejected_fenced);

    hpx::future<int> f = hpx::supervision::dispatch_work<probe_action>(
        peer->shadow, peer_locality, join_epoch);

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
