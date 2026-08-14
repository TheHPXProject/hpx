//  Copyright (c) 2026 Bita Yekrang
//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Helper for the departed_locality_7384 regression test. This executable
// connects to the running test as a new locality, waits until the test has
// captured this locality's id, and disconnects gracefully.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/lcos.hpp>

#include <string>
#include <vector>

int hpx_main()
{
    // wait until the test driver has captured this locality's id
    hpx::distributed::latch sync;
    sync.connect_to("departed_locality_7384_sync");
    sync.arrive_and_wait();

    hpx::disconnect();
    return 0;
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {
        // Make sure hpx_main above will be executed even if this is not the
        // console locality.
        "hpx.run_hpx_main!=1",

        // Make sure networking will not be disabled.
        "hpx.expect_connecting_localities!=1"};

    // Note: this uses runtime_mode::connect to instruct this locality to
    // connect to the existing HPX application
    hpx::init_params init_args;
    init_args.mode = hpx::runtime_mode::connect;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}
#endif
