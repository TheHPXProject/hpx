//  Copyright (c) 2020-2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Verify that hpx/local.hpp provides access to the Standard Parallel Toolkit:
// futures, parallel algorithms, numeric algorithms, and execution policies.
//
// In local-only builds (HPX_WITH_DISTRIBUTED_RUNTIME=OFF), hpx/local.hpp also
// includes hpx_main.hpp for implicit main() wrapping. In full builds,
// the test includes it explicitly.

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
#include <hpx/hpx_main.hpp>
#endif

#include <hpx/local.hpp>

#include <cstddef>
#include <iostream>
#include <numeric>
#include <vector>

int main()
{
    // 1. Verify hpx::async and hpx::future are reachable
    hpx::future<int> f = hpx::async([]() { return 42; });
    int result = f.get();

    // 2. Verify parallel algorithms are reachable
    std::vector<int> v(100);
    std::iota(v.begin(), v.end(), 1);

    hpx::for_each(
        hpx::execution::par, v.begin(), v.end(), [](int& x) { x *= 2; });

    // 3. Verify numeric algorithms are reachable
    int sum = hpx::reduce(hpx::execution::par, v.begin(), v.end(), 0);

    std::cout << "async result: " << result << ", reduce sum: " << sum
              << std::endl;

    return 0;
}
