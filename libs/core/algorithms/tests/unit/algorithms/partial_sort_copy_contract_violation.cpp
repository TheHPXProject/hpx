//  Copyright (c) 2026 adhithyaragavan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test: HPX_PRE violation for hpx::partial_sort_copy
//
// hpx::partial_sort_copy requires d_first <= d_last on the destination
// range (the source range only needs to be an input/forward iterator range,
// which is not comparable with <=, so no precondition is placed on it).
// This test passes a destination range where d_last lies before d_first,
// which violates the precondition. Both destination iterators are
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
    std::vector<int> d(v.size());

    // d_last is before d_first -- violates d_first <= d_last
    [[maybe_unused]] auto result = hpx::partial_sort_copy(
        v.begin(), v.end(), d.begin() + 3, d.begin() + 1);

    return 0;
}
