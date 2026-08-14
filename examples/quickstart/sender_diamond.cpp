//  Copyright (c) 2026 Kashy Namboothiri
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Note: This example is excluded from MSVC builds with C++ modules enabled
// due to a compiler ICE. See CMakeLists.txt for the build guard.

// Demonstrates a diamond dependency pattern using HPX sender/receiver
// primitives. A single sender (A) is shared between two independent continuations
// (B and C) using split. Their results are then joined by when_all and merged
// in a final step (D).

// Dependency structure:
//
//         A  (produce initial value)
//        / \   split dependencies
//       B   C  (independent transforms, dispatched to thread pool)
//        \ /   merge dependencies
//         D  (merge)

#include <hpx/assert.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>

#include <iostream>
#include <utility>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

int hpx_main()
{
    // A. Produces an initial value
    auto a = ex::just(10) | ex::then([](int x) {
        std::cout << "A: produced " << x << "\n";
        return x;
    });

    // split makes the sender A safe to connect more than once
    auto a_shared = ex::split(std::move(a));

    auto sched = ex::thread_pool_scheduler{};

    // Keep the stable runtime behavior here and silence the local deprecation
    // diagnostic so warning-as-error builds do not fail.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

    // B. Double the value from A
    auto b = a_shared | ex::transfer(sched) | ex::then([](int x) {
        int result = x * 2;
        std::cout << "B: " << x << " * 2 = " << result << "\n";
        return result;
    });

    // C. Triple the value from A
    auto c = a_shared | ex::transfer(sched) | ex::then([](int x) {
        int result = x * 3;
        std::cout << "C: " << x << " * 3 = " << result << "\n";
        return result;
    });

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    // D. Join B and C, then sum their results
    auto d = ex::then(
        ex::when_all(std::move(b), std::move(c)), [](int from_b, int from_c) {
            int result = from_b + from_c;
            std::cout << "D: " << from_b << " + " << from_c << " = " << result
                      << "\n";
            return result;
        });

    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    auto result = hpx::get<0>(*tt::sync_wait(std::move(d)));
    std::cout << "Final Result: " << result << "\n";

    // expected: A=10, B=20, C=30, D=50
    HPX_ASSERT(result == 50);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
