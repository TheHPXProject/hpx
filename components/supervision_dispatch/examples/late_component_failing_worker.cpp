//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Acceptance check / usage example: a worker that dies mid-work without
// corrupting AGAS or hanging runtime shutdown.
//
// This models a failure mode distinct from the other two ways a worker can
// leave a supervision_dispatch application:
//   - plain_worker.cpp is the fully graceful case: it calls finalize(), which
//     publishes event::completed, unregisters both symbol names, and joins the
//     failure-detection/heartbeat background tasks before returning - peers
//     observe a clean running -> completed transition and no fencing is ever
//     triggered.
//   - A true crash (e.g. std::abort(), not modeled by any example in this
//     directory) leaves everything - the two background hpx::async loops, AGAS
//     registrations, and the parcelport connection - torn down abruptly and
//     inconsistently. This is exactly the scenario dispatch_work()'s
//     target_fenced/rejected_fenced fencing exists to protect against.
//   - This worker is in between: it connects to an already-running application
//     (hpx::runtime_mode::connect), does a bit of work, then departs *without*
//     calling finalize() - so, from supervision_dispatch's point of view, it is
//     indistinguishable from a crash: no event::completed is ever published and
//     neither symbol name is ever unregistered, so the sentinel's last
//     published event stays stale and peers' failure_detection_loop()
//     eventually times out and observes target_fenced/rejected_fenced against
//     it, exactly as for a true crash. Unlike a true crash, though, it calls
//     hpx::supervision::testing::stop_background_loops() before departing, so
//     the failure_detection_loop()/heartbeat_loop() hpx::async tasks are joined
//     cleanly first, and only then does it call hpx::disconnect(), which tears
//     down this locality's HPX runtime/AGAS/parcelport connection consistently
//     instead of leaving it in a raw SIGABRT state. Net effect: a clean
//     HPX-level exit that supervision_dispatch still cannot distinguish from a
//     crash.
//
// \note stop_background_loops() is testing/example-only: it is not part of the
//       public dispatch API. Without it, hpx::disconnect() would either hang
//       waiting on the still-running background tasks, or tear the runtime down
//       while they are mid-iteration dereferencing sentinel/ registry state --
//       trading AGAS-visible corruption for a runtime-shutdown hazard.

// This worker demonstrates its "bit of local work" against
// late_component::test_server (see late_component_worker.hpp), the same
// component type the eventual root launcher creates instances of on a worker's
// locality - reusing that shared header instead of an inline, look-alike
// component definition keeps this file aligned with what the launcher will
// hpx::new_<test_server>() against.

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/supervision.hpp>

#include <hpx/supervision_dispatch.hpp>

#include "late_component_worker.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

    // Long enough to comfortably absorb AGAS/peer-startup jitter; short enough
    // to keep this example fast (mirrors plain_worker.cpp).
    constexpr std::chrono::milliseconds worker_discovery_timeout{5000};

    // Testing-only knob: must be set before init() starts
    // failure_detection_loop() to take effect for this worker's own lifecycle,
    // so peers this worker joins are not watched via the real 60s
    // default_discovery_timeout.
    constexpr std::chrono::milliseconds failure_detection_poll_timeout{500};

    // Walks this worker through init() -> discover_and_join() -> a bit of
    // local work -> a departure that is clean at the HPX level but
    // indistinguishable from a crash to supervision_dispatch.
    int run_failing_worker_role()
    {
        hpx::supervision::testing::
            set_failure_detection_poll_timeout_for_testing(
                failure_detection_poll_timeout);

        hpx::supervision::registry const handle =
            hpx::supervision::init(hpx::launch::sync, worker_discovery_timeout);

        std::uint64_t const epoch =
            hpx::supervision::query_state(hpx::launch::sync, handle).epoch;

        hpx::supervision::publish_event(
            hpx::launch::sync, handle, hpx::supervision::event::running, epoch);

        std::vector<hpx::supervision::discovered_peer> const peers =
            hpx::supervision::discover_and_join(
                handle, worker_discovery_timeout);
        hpx::util::format_to(
            std::cout, "worker joined {} peer(s)\n", peers.size());

        // Do a bit of local work, demonstrating this worker was actually
        // active before departing.
        auto const worker =
            hpx::new_<late_component::test_client>(hpx::find_here());

        worker.set_message(
            "step 1: joined peers, epoch " + std::to_string(epoch));
        hpx::util::format_to(
            std::cout, "worker reported: {}\n", worker.get_message());

        worker.set_message("step 2: mid-work, about to depart");
        hpx::util::format_to(
            std::cout, "worker reported: {}\n", worker.get_message());

        // Depart mid-epoch: no finalize() (so no event::completed is ever
        // published and neither symbol name is ever unregistered - the
        // sentinel's last published event stays stale), and no std::abort() (so
        // AGAS/the parcelport connection are torn down consistently rather than
        // corrupted). First join the background loops so hpx::disconnect()
        // below does not hang waiting on them or race their teardown of
        // sentinel/registry state.
        hpx::supervision::testing::stop_background_loops();

        return hpx::disconnect();
    }
}    // namespace

int hpx_main()
{
    return run_failing_worker_role();
}

int main(int argc, char* argv[])
{
    // Enforce connect mode: this worker joins an already-running
    // supervision_dispatch application rather than starting one of its own.
    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    hpx::init_params init_args;
    init_args.mode = hpx::runtime_mode::connect;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
