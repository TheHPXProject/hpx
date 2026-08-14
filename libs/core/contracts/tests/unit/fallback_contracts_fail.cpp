//  Copyright (c) 2025 Alexandros Papadakis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test: Fallback contracts fail
// Tests fallback mode when __cpp_contracts is NOT available
// Should fail in Debug mode ONLY (due to HPX_ASSERT fallback)
// Expected to fail in Debug builds (WILL_FAIL property set in CMakeLists.txt)

#include <hpx/contracts.hpp>
#include <hpx/modules/testing.hpp>

#include <iostream>

int main() HPX_PRE(false)    // This precondition is ignored in fallback mode
    HPX_POST(false)          // This postcondition is ignored in fallback mode
{
    HPX_CONTRACT_ASSERT(true);

    // Add a failing assertion to test WILL_FAIL behavior.
    HPX_CONTRACT_ASSERT(false);    // This should abort in Debug mode
    HPX_TEST(true);

    return hpx::util::report_errors();
}
