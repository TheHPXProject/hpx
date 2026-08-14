//  Copyright (c) 2026 Arpit Khandelwal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for the inconsistency between operator== and operator!= in
// hpx::parallel::util::detail::prefetching_iterator.
//
// Before the fix, operator== checked both idx_ and base_ while operator!=
// only checked idx_. This meant two iterators with equal idx_ but different
// base_ would have both == and != return false -- a logical contradiction.

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <numeric>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
void test_same_context_consistency()
{
    std::size_t const prefetch_distance_factor = 2;
    std::vector<double> c(128, 1.0);
    std::vector<std::size_t> range(128);
    std::iota(range.begin(), range.end(), 0);

    auto ctx = hpx::parallel::util::make_prefetcher_context(
        range.begin(), range.end(), prefetch_distance_factor, c);

    auto it_end = ctx.end();
    for (auto it = ctx.begin(); it != it_end; ++it)
    {
        // operator!= and operator== must be logical negations at every step
        HPX_TEST(!(it == it_end));

        auto it2 = it;
        HPX_TEST(it2 == it);
        HPX_TEST(!(it2 != it));
    }

    auto it = ctx.end();
    HPX_TEST(it == it_end);
    HPX_TEST(!(it != it_end));
}

///////////////////////////////////////////////////////////////////////////////
// Directly verifies the pre-fix bug: two iterators with equal idx_ but
// different base_ must satisfy (a != b) == !(a == b).
// Before the fix, both == and != returned false simultaneously for this case.
void test_different_base_consistency()
{
    std::size_t const prefetch_distance_factor = 2;

    std::vector<double> c1(64, 1.0);
    std::vector<std::size_t> range1(64);
    std::iota(range1.begin(), range1.end(), 0);

    std::vector<double> c2(64, 2.0);
    std::vector<std::size_t> range2(64);
    std::iota(range2.begin(), range2.end(), 0);

    auto ctx1 = hpx::parallel::util::make_prefetcher_context(range1.data(),
        range1.data() + range1.size(), prefetch_distance_factor, c1);
    auto ctx2 = hpx::parallel::util::make_prefetcher_context(range2.data(),
        range2.data() + range2.size(), prefetch_distance_factor, c2);

    auto it1 = ctx1.begin();
    auto it2 = ctx2.begin();

    // idx_ is 0 for both; base_ points into different underlying sequences/contexts.
    HPX_TEST(!(it1 == it2));
    // Before the fix: operator!= only checked idx_, returning false here.
    HPX_TEST(it1 != it2);
    // The fundamental invariant: (a != b) == !(a == b)
    HPX_TEST((it1 != it2) == !(it1 == it2));
}

///////////////////////////////////////////////////////////////////////////////
void test_reflexive_symmetric()
{
    std::size_t const prefetch_distance_factor = 2;
    std::vector<double> c(32, 1.0);
    std::vector<std::size_t> range(32);
    std::iota(range.begin(), range.end(), 0);

    auto ctx = hpx::parallel::util::make_prefetcher_context(
        range.begin(), range.end(), prefetch_distance_factor, c);

    auto it = ctx.begin();
    auto it2 = it;

    HPX_TEST(it == it);
    HPX_TEST(!(it != it));
    HPX_TEST(it == it2);
    HPX_TEST(it2 == it);
    HPX_TEST(!(it != it2));
    HPX_TEST(!(it2 != it));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_same_context_consistency();
    test_different_base_consistency();
    test_reflexive_symmetric();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
