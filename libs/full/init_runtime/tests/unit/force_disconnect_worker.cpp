//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Helper for the force_disconnect test. This executable is launched multiple
// times by force_disconnect.cpp (see process::launch_connecting_locality()
// there), each time connecting to the running test as an additional locality.
// Each instance simply keeps the runtime alive and is force-disconnected by
// the console locality (via hpx::force_disconnect); it never waits on a latch
// or initiates its own disconnection.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/collectives.hpp>

#include <chrono>
#include <cstdint>

// A trivial action used to probe whether a given locality is still reachable,
// i.e. still registered with AGAS and present in the connection caches.
std::uint32_t ping_locality()
{
    return hpx::get_locality_id();
}
HPX_PLAIN_ACTION(ping_locality, ping_locality_action)

// A long-running action used to simulate a parcel that is still in flight while
// the target locality is being force-disconnected.
std::uint32_t sleep_locality(int const ms)
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return hpx::get_locality_id();
}
HPX_PLAIN_ACTION(sleep_locality, sleep_locality_action)

int hpx_main()
{
    // This locality doesn't call hpx::disconnect itself, but will be either
    // disconnected by the console (using hpx::force_disconnect) or forcefully
    // terminated.
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
