//  Copyright (c) 2020-2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Verify that hpx/local.hpp provides access to the Standard Parallel Toolkit:
// parallel algorithms, numeric algorithms, and execution policies.
//
// We use hpx::local::init to avoid any dependency on the full wrap module.

#include <hpx/config.hpp>

#include <hpx/init.hpp>
#include <hpx/local.hpp>

#include <cstddef>
#include <iostream>
#include <numeric>
#include <vector>

int test_main(int argc, char* argv[])
{
    // 1. Verify parallel algorithms are reachable via hpx/local.hpp
    std::vector<int> v(100);
    std::iota(v.begin(), v.end(), 1);

    hpx::for_each(
        hpx::execution::par, v.begin(), v.end(), [](int& x) { x *= 2; });

    // 2. Verify numeric algorithms are reachable
    int sum = hpx::reduce(hpx::execution::par, v.begin(), v.end(), 0);

    std::cout << "reduce sum: " << sum << std::endl;

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(test_main, argc, argv);
}
