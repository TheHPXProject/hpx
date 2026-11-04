//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression check for #7825: finalize() called while a failure detection
// sweep is still waiting on its peers, immediately followed by runtime
// shutdown, must exit cleanly. The sweep's await_terminal() waiters (bounded
// by the default 60s poll timeout) are still pending when the runtime shuts
// down. They used to be broken only by ~supervision_manager, after the runtime
// had stopped, and their hpx::launch::async continuations then aborted the
// process trying to register a thread.
//
// Two things would resolve those waiters early and hide the problem, so this
// test avoids both, as does the scenario from #7825:
//   - The runtime is started with hpx::start() and no hpx_main. With
//     hpx_main, post_main() runs once it returns, while the runtime is still
//     active, and its supervision_manager::tidy() removes the peer's (mirrored)
//     state together with its waiters. Without it, post_main() runs at
//     startup instead. For the same reason, the global barrier (detached by
//     post_main()) is not available, named barriers are used instead.
//   - The peer never publishes event::completed: it only stops its background
//     loops. Otherwise the observer's registry would mirror that terminal
//     event onto its own supervision state and resolve the waiters normally.
//
// Deliberately isolated from failure_detection.cpp, whose scenarios depend on
// each other's state (and on a shortened poll timeout), while this needs the
// default poll timeout and shuts down right after finalize().

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_start.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading.hpp>

#include <hpx/supervision_dispatch.hpp>

#include <chrono>
#include <vector>

// ============================================================================
// Test Cases: finalize() with a sweep in flight
// ============================================================================

// The detection loop must have a sweep in flight, i.e. await_terminal()
// waiters pending against the (live) peer, when finalize() is called, and
// finalize() must still return promptly.
void test_finalize_with_pending_sweep(
    hpx::supervision::registry const& local_registry)
{
    // Both localities init() concurrently, so the peer may not have been
    // registered yet when this locality ran its own discovery; join again
    // until a sweep against it is in flight.
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(30);
    bool sweep_started = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (hpx::supervision::testing::
                failure_detection_sweep_in_flight_for_testing())
        {
            sweep_started = true;
            break;
        }
        hpx::supervision::discover_and_join(
            local_registry, std::chrono::milliseconds(500));
        hpx::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    HPX_TEST(sweep_started);

    auto const start = std::chrono::steady_clock::now();
    hpx::supervision::finalize();
    HPX_TEST(
        std::chrono::steady_clock::now() - start < std::chrono::seconds(2));
}

// ============================================================================
// Main Test Entry Point (SPMD across 2 localities)
// ============================================================================

// Runs on every locality, returns whether this is the observer (locality 0).
bool run_test()
{
    std::vector<hpx::id_type> const remote_localities =
        hpx::find_remote_localities();
    HPX_TEST(!remote_localities.empty());

    bool const is_observer = (hpx::get_locality_id() == 0);
    if (remote_localities.empty())
    {
        return is_observer;
    }

    // Keep the default poll timeout (60s): the waiters must outlive the rest
    // of this test, so that only shutdown can resolve them.
    hpx::supervision::registry local_registry;
    HPX_TEST_NO_THROW(
        local_registry = hpx::supervision::init(hpx::launch::sync));
    hpx::distributed::barrier("finalize_with_pending_sweep_init").wait();

    if (is_observer)
    {
        test_finalize_with_pending_sweep(local_registry);
    }

    // The peer stays alive until the observer is done, then both shut down
    // right away, as in #7825. The peer only stops its loops, so it never
    // publishes event::completed (see above).
    hpx::distributed::barrier("finalize_with_pending_sweep_done").wait();
    if (!is_observer)
    {
        hpx::supervision::testing::stop_background_loops();
    }

    return is_observer;
}

int main(int argc, char* argv[])
{
    // no hpx_main, see above
    if (!hpx::start(nullptr, argc, argv))
    {
        return 1;
    }

    if (hpx::run_as_hpx_thread(&run_test))
    {
        hpx::post([] { hpx::finalize(); });
    }

    // non-console localities may see the runtime being shutdown before they
    // call finalize, causing an error to be reported that can be ignored.
    int const result = hpx::stop();
    HPX_TEST(result == 0 || result == -1);

    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
