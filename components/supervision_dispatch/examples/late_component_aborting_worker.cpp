//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This binary intentionally exits without any cleanup, to model an
// unrecoverable worker crash: it never calls hpx::supervision::finalize(),
// never calls hpx::disconnect(), and never makes any supervision_dispatch call
// to "fake" a failure. After std::exit() below, this locality's registry entry
// on root simply stops being refreshed - there is no terminal event, no
// graceful teardown, nothing.
//
// Detection of this crash is entirely root-side: root's
// failure_detection_loop() (started internally by root's own
// hpx::supervision::init() call, see dispatch_api.cpp) notices the silence once
// the poll timeout elapses, fences this locality's shadow, and any subsequent
// dispatch_work()/check_admission() call against it observes
// hpx::error::target_fenced / hpx::supervision::dispatch_outcome::
// rejected_fenced.
//
// This is a standalone, self-contained worker: it creates its own
// late_component::test_server locally and exercises it briefly before crashing,
// purely so the demo has some observable "the worker was alive and doing real
// component work" evidence. In the full three-binary demo (root launcher + live
// worker + this failing worker), root would instead create this component
// itself (via spawn_worker) and hand the id to the worker - this task's
// standalone version only self-creates it so it can be built and exercised in
// isolation, ahead of that root-driven wiring.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/supervision.hpp>

#include <hpx/supervision_dispatch.hpp>

#include "late_component_worker.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

    // Long enough to comfortably absorb AGAS/peer-startup jitter; mirrors the
    // timeout used by late_component_worker.cpp/
    // late_component_failing_worker.cpp.
    constexpr std::chrono::milliseconds worker_discovery_timeout{5000};
}    // namespace

int hpx_main()
{
    hpx::util::format_to(std::cerr,
        "{}: late_component_aborting_worker: started\n", hpx::find_here());

    // Register this locality as a supervised participant and announce it has
    // entered its active phase - this is what creates the registry entry on
    // root that, per the file comment above, simply stops being refreshed once
    // std::exit() below fires.
    hpx::supervision::registry const handle =
        hpx::supervision::init(hpx::launch::sync, worker_discovery_timeout);

    std::uint64_t const epoch =
        hpx::supervision::query_state(hpx::launch::sync, handle).epoch;

    hpx::supervision::publish_event(
        hpx::launch::sync, handle, hpx::supervision::event::running, epoch);

    std::vector<hpx::supervision::discovered_peer> const peers =
        hpx::supervision::discover_and_join(handle, worker_discovery_timeout);

    if (peers.empty())
    {
        hpx::util::format_to(std::cerr,
            "{}: late_component_aborting_worker: failed to "
            "discover/join root locality\n",
            hpx::find_here());

        hpx::supervision::publish_event(
            hpx::launch::sync, handle, hpx::supervision::event::failed, epoch);
        return hpx::disconnect();
    }

    hpx::util::format_to(std::cerr,
        "{}: late_component_aborting_worker: aborting worker joined {} "
        "peer(s)\n",
        hpx::find_here(), peers.size());

    // Create/exercise its own local test_server briefly, as the file comment
    // above promises, purely so the demo has some observable "the worker was
    // alive and doing real component work" evidence before it crashes.
    auto const worker =
        hpx::new_<late_component::test_client>(hpx::find_here());

    worker.set_message("step 1: joined peers, epoch " + std::to_string(epoch));
    hpx::util::format_to(std::cerr,
        "{}: late_component_aborting_worker: aborting worker reported: {}\n",
        hpx::find_here(), worker.get_message());

    // Simulate a genuine, unrecoverable crash: no hpx::supervision::finalize(),
    // no hpx::disconnect(), no supervision_dispatch call to "fake" a failure.
    // This locality's registry/heartbeat entry simply stops being refreshed
    // from here on - root's failure_detection_loop() is what notices.
    std::exit(-1);
}

int main(int argc, char* argv[])
{
    // Connect to the already-running root application rather than bootstrapping
    // a new one, matching late_component_worker.cpp/ plain_worker.cpp.
    hpx::init_params init_args;
    init_args.mode = hpx::runtime_mode::connect;

    return hpx::init(argc, argv, init_args);
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
