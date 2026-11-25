//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A worker locality of the locality_removed test: it connects to the running
// test, receives device data, and is disconnected by the test, leaves on its
// own (leave_action), or is killed.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>

#include "locality_removed.hpp"

int hpx_main()
{
    // the runtime keeps running until the locality is disconnected
    return 0;
}

int main(int argc, char* argv[])
{
    hpx::init_params params;
    params.mode = hpx::runtime_mode::connect;
    return hpx::init(argc, argv, params);
}

#endif
