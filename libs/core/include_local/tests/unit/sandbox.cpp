//  Copyright (c) 2020-2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// HPX Sandbox Laboratory -- unit test
//
// Verifies hpx/experimental/sandbox.hpp by:
//   1. Introspecting the environment (describe_environment)
//   2. Running a comparative for_each benchmark (seq vs par)
//   3. Printing the full telemetry report

#include <hpx/hpx_main.hpp>

#include <hpx/algorithm.hpp>
#include <hpx/experimental/sandbox.hpp>

#include <cmath>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <vector>

int main()
{
    namespace sandbox = hpx::experimental::sandbox;

    // 1. Print environment report
    sandbox::describe_environment(std::cout);

    // 2. Prepare test data
    constexpr std::size_t N = 500'000;
    std::vector<double> data(N);
    std::iota(data.begin(), data.end(), 1.0);

    auto work = [](double& x) { x = std::sin(x) * std::cos(x) + std::sqrt(x); };

    // 3. Run comparative benchmark
    auto report = sandbox::benchmark(
        "for_each (sin*cos+sqrt on 500k doubles)",
        [&]() {
            hpx::for_each(hpx::execution::seq, data.begin(), data.end(), work);
        },
        [&]() {
            hpx::for_each(hpx::execution::par, data.begin(), data.end(), work);
        },
        5);

    // 4. Print the benchmark report
    report.print(std::cout);

    // 5. Sanity checks
    if (report.speedup < 0.0)
    {
        std::cerr << "ERROR: negative speedup\n";
        return 1;
    }

    std::cout << "\nSandbox test passed.\n";
    return 0;
}
