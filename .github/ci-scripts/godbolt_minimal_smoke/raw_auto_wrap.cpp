// Copyright (c) 2026 Rohan Pattanayak
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// No #include <hpx/hpx_main.hpp> here: run_raw_msvc_smoke.ps1 force-includes
// it with /FI, the way HPX::auto_wrap_main does on MSVC.
#include <hpx/experimental/sandbox.hpp>

#include <iostream>

int main()
{
    hpx::experimental::sandbox::describe_environment(std::cout);
    std::cout << "Hello raw auto-wrap\n";
    return 0;
}
