//  Copyright (c) 2026 Animesh Srivastava
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test: HPX_PRE violation for hpx::latch::arrive_and_wait
//
// A negative n violates n >= 0. Only built with native C++26 contracts
// (HPX_WITH_CXX26_CONTRACTS), as HPX_PRE is a no-op otherwise.

#include <hpx/latch.hpp>

#include <cstddef>

int main()
{
    hpx::latch l(2);

    // n is negative -- violates n >= 0
    l.arrive_and_wait(std::ptrdiff_t(-1));

    return 0;
}
