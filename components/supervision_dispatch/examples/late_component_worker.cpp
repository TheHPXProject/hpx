//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A worker that connects to an already-running root locality (via
// hpx::runtime_mode::connect, mirroring the connect-mode pattern used by
// examples/heartbeat/heartbeat.cpp and examples/throttle/throttle_client.cpp
// for attaching to an existing application rather than co-launching every
// locality together, as plain_worker.cpp does), joins it under
// supervision_dispatch, and then polls locally until either the root goes
// silent or an idle threshold elapses, at which point it tears itself down.
//
// This worker never receives direct calls itself: root dispatches
// set_message_action traffic (see late_component_worker.hpp) to the test_server
// component instance(s) it owns, not to this process.
//
// This binary includes late_component_worker.hpp purely for type awareness (so
// the two sides of this pair name the same component/action types) - it never
// creates, addresses, or calls into a test_server instance itself. Consequently
// there is no direct "new work arrived" signal available here to reset an idle
// clock; the "idle" time tracked below is simply time since this loop
// started/last polled, used only to decide when to stop waiting on root's
// continued presence.
//
// This worker never uses any supervision_dispatch "stop" primitive: its own
// exit is entirely local, driven by either the idle-timeout or a fenced-root
// detection, never by an incoming message from root.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/include/threadmanager.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/supervision_dispatch.hpp>

#include "late_component_worker.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

    // Long enough to comfortably absorb AGAS/root-startup jitter; mirrors the
    // timeout used by plain_worker.cpp/fan_out_join.cpp.
    constexpr std::chrono::milliseconds worker_discovery_timeout{5000};
}    // namespace

// Walks this worker through its full lifecycle: init() -> publish_event(
// running) -> discover_and_join() the root -> poll (fencing against root's
// join epoch each iteration, and against an idle timeout) -> publish_event(
// completed) -> finalize() -> hpx::disconnect().
int run_worker(std::chrono::milliseconds idle_timeout,
    std::chrono::milliseconds poll_interval)
{
    // Registers this locality as a supervised participant and returns the
    // handle used for every subsequent supervision_dispatch call below.
    hpx::supervision::registry const handle =
        hpx::supervision::init(hpx::launch::sync, worker_discovery_timeout);

    // Pull the epoch init() just established rather than assuming epoch 0.
    std::uint64_t const epoch =
        hpx::supervision::query_state(hpx::launch::sync, handle).epoch;

    // Announce this locality has entered its active phase for the current
    // epoch. There is no per-component-instance event to publish here --
    // only this locality-scoped one; the test_server instance(s) root
    // dispatches work to are supervised purely as part of root's own
    // locality state, not separately.
    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::running, epoch);

    // Discover and join the root locality this worker connected to, so that
    // root_peer.locality/root_peer.join_epoch below can be used to fence
    // against it via check_admission().
    std::vector<hpx::supervision::discovered_peer> const peers =
        hpx::supervision::discover_and_join(handle, worker_discovery_timeout);

    if (peers.empty())
    {
        std::cerr << "late_component_worker: failed to discover/join root "
                     "locality\n";

        hpx::supervision::publish_event(
            hpx::launch::sync, handle, hpx::supervision::event::failed, epoch);
        hpx::supervision::finalize();
        return hpx::disconnect();
    }

    // find peer that represents the root locality
    auto const it = std::ranges::find_if(
        peers, [](hpx::supervision::discovered_peer const& p) {
            return hpx::naming::get_locality_id_from_id(p.locality) == 0;
        });

    hpx::supervision::discovered_peer root_peer;
    if (it != peers.end())
    {
        root_peer = *it;
    }
    else
    {
        std::cerr << "late_component_worker: failed to find root locality\n";
        hpx::supervision::publish_event(
            hpx::launch::sync, handle, hpx::supervision::event::failed, epoch);
        hpx::supervision::finalize();
        hpx::disconnect();
        return -1;
    }

    // "Last observed activity" -- see the file comment above for why this
    // can only ever mean "time since this loop started/last polled" here,
    // rather than time since any real work signal.
    hpx::chrono::high_resolution_timer idle_timer;
    for (;;)
    {
        hpx::this_thread::suspend(poll_interval);

        // Symmetric fencing: if root has latched a terminal event for the
        // epoch we joined it at, it is dead/silent -- shut down immediately,
        // skipping the idle-timeout wait entirely.
        hpx::supervision::dispatch_outcome const outcome =
            hpx::supervision::check_admission(
                root_peer.locality, root_peer.join_epoch);
        if (outcome == hpx::supervision::dispatch_outcome::rejected_fenced)
        {
            std::cerr << "late_component_worker: root locality fenced; "
                         "shutting down\n";
            break;
        }

        if (std::chrono::duration<double>(idle_timer.elapsed()) >= idle_timeout)
        {
            std::cerr << "late_component_worker: idle timeout elapsed with "
                         "no further root demand; shutting down\n";
            break;
        }
    }

    // Announce completion for the same epoch we started `running` under.
    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::completed, epoch);

    hpx::supervision::finalize();

    return hpx::disconnect();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::chrono::milliseconds const idle_timeout{
        vm["idle-timeout"].as<std::uint64_t>()};
    std::chrono::milliseconds const poll_interval{
        vm["poll-interval"].as<std::uint64_t>()};

    return run_worker(idle_timeout, poll_interval);
}

int main(int argc, char* argv[])
{
    hpx::program_options::options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    using hpx::program_options::value;
    desc_commandline.add_options()
        ("idle-timeout", value<std::uint64_t>()->default_value(30000),
         "milliseconds of no further root demand before this worker shuts "
         "itself down")
        ("poll-interval", value<std::uint64_t>()->default_value(500),
         "milliseconds to sleep between successive root-fencing/idle checks")
    ;
    // clang-format on

    // Connect to an already-running root locality rather.
    hpx::init_params init_args;
    init_args.mode = hpx::runtime_mode::connect;
    init_args.desc_cmdline = desc_commandline;

    return hpx::init(argc, argv, init_args);
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
