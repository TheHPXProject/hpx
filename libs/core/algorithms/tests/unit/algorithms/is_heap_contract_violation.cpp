//  Copyright (c) 2026 adhithyaragavan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test: HPX_PRE violation for hpx::is_heap
//
// hpx::is_heap requires first <= last. This test passes last that lies
// before first, which violates the precondition. Both iterators are
// individually valid and in-bounds (only their relative order is wrong), so
// constructing and comparing them is well-defined regardless of whether
// contracts are enabled -- only entering the algorithm body with this
// ordering would be unsafe.
//
// This test is only built when native C++26 contracts (HPX_WITH_CXX26_CONTRACTS)
// are enabled: the precondition check happens at the function boundary before
// the (unsafe) algorithm body ever runs, so the violation is caught safely by
// the contract machinery before any out-of-order iterator use occurs. Without
// native contracts, HPX_PRE is a no-op and the call would fall through into
// the algorithm body with this invalid ordering, which is undefined behavior
// unrelated to contracts -- so this test is intentionally excluded from
// non-contract builds rather than expected to "safely" no-op.

#include <hpx/algorithm.hpp>

#include <vector>

int main()
{
    std::vector<int> v{5, 3, 1, 4, 2};

    // last is before first -- violates first <= last
    [[maybe_unused]] bool result = hpx::is_heap(v.begin() + 3, v.begin() + 1);

    return 0;
}
