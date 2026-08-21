//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Helper for the launch_connecting_locality unit test. This executable connects
// to the running test as a new locality and disconnects gracefully.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

int hpx_main()
{
    hpx::disconnect();
    return 0;
}

int main(int argc, char* argv[])
{
    // Note: this uses runtime_mode::connect to instruct this locality to
    // connect to the existing HPX application.
    hpx::init_params init_args;
    init_args.mode = hpx::runtime_mode::connect;

    return hpx::init(argc, argv, init_args);
}

#endif
