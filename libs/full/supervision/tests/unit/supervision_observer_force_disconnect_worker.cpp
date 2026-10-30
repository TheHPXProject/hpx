//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Helper for the supervision_observer_force_disconnect test. This executable
// is launched by that test (see process::launch_connecting_locality() there),
// each time connecting to the running test as an additional locality. It only
// provides the actions the test invokes on it, and is force-disconnected by
// the console locality (via hpx::force_disconnect); it never initiates its
// own disconnection.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/supervision.hpp>

// The launch latch is released before the worker finishes runtime startup,
// the test uses this to wait for the running state.
bool worker_is_running()
{
    return hpx::is_running();
}
HPX_PLAIN_ACTION(worker_is_running, worker_is_running_action)

// Registers an observer that lives on this locality for target with the
// supervision manager on manager_locality. Returns the observer handle.
hpx::id_type register_observer_here(
    hpx::id_type const& manager_locality, hpx::id_type const& target)
{
    return hpx::supervision::register_observer(hpx::launch::sync,
        manager_locality, target,
        [](hpx::supervision::lifecycle_event_notification const&) {
            return true;
        });
}
HPX_PLAIN_ACTION(register_observer_here, register_observer_here_action)

int hpx_main()
{
    // This locality doesn't call hpx::disconnect itself, but will be
    // disconnected by the console (using hpx::force_disconnect).
    return 0;
}

int main(int argc, char* argv[])
{
    // Note: this uses runtime_mode::connect to instruct this locality to
    // connect to the existing HPX application
    hpx::init_params init_args;
    init_args.mode = hpx::runtime_mode::connect;

    return hpx::init(argc, argv, init_args);
}

#endif
