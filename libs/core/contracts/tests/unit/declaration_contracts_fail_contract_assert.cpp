//  Copyright (c) 2025 Alexandros Papadakis
//  Copyright (c) 2025 Panagiotis Syskakis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test: Declaration contracts fail (contract_assert)
// Tests C++26 CONTRACT_ASSERT failure in context of declaration contracts
// Expected to fail when contracts are active (both native and fallback modes)

#include <hpx/contracts.hpp>

// Function that uses contract_assert internally
int process_data(int const x) HPX_PRE(x >= 0)
{
    // Internal contract check that always fails
    HPX_CONTRACT_ASSERT(false);

    // The test should fail even in cases where the contracts are being ignored.
    return x + 1;
}

int main()
{
    // This should trigger CONTRACT_ASSERT failure or return 1 to make the test
    // fail.
    return process_data(5);
}
