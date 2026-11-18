//  Copyright (c) 2026 Vaatsalya Srivastava
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// partial_sort and nth_element fall back to a heap if the recursion depth limit
// is reached. The fallback used to build a max-heap of the whole range and sort
// its first elements, i.e. it selected the largest instead of the smallest
// elements. An organ pipe input (0 1 2 ... 2 1 0) reaches the depth limit.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

using vec = std::vector<int>;

// 0 1 2 ... 2 1 0
vec organ_pipe(int n)
{
    vec result;
    result.reserve(n);
    for (int i = 0; i != n; ++i)
    {
        result.push_back((std::min) (i, n - 1 - i));
    }
    return result;
}

template <typename ExPolicy>
void test_partial_sort(ExPolicy&& policy, vec const& input, int k)
{
    vec result = input;
    hpx::partial_sort(std::forward<ExPolicy>(policy), result.begin(),
        result.begin() + k, result.end());

    vec expected = input;
    std::partial_sort(expected.begin(), expected.begin() + k, expected.end());

    HPX_TEST(std::equal(result.begin(), result.begin() + k, expected.begin()));
}

template <typename ExPolicy>
void test_nth_element(ExPolicy&& policy, vec const& input, int nth)
{
    vec result = input;
    hpx::nth_element(std::forward<ExPolicy>(policy), result.begin(),
        result.begin() + nth, result.end());

    vec sorted = input;
    std::sort(sorted.begin(), sorted.end());

    // the nth element is in its sorted position ...
    HPX_TEST_EQ(result[nth], sorted[nth]);

    // ... with nothing larger in front of it and nothing smaller behind it
    HPX_TEST(std::all_of(result.begin(), result.begin() + nth,
        [&](int x) { return x <= result[nth]; }));
    HPX_TEST(std::all_of(result.begin() + nth + 1, result.end(),
        [&](int x) { return x >= result[nth]; }));
}

void test_issue_case()
{
    namespace ex = hpx::execution;

    vec const input = organ_pipe(249);

    test_partial_sort(ex::seq, input, 185);
    test_partial_sort(ex::par, input, 185);
    test_nth_element(ex::seq, input, 184);
}

void test_all_sizes()
{
    namespace ex = hpx::execution;

    for (int n = 2; n <= 400; ++n)
    {
        vec const input = organ_pipe(n);

        for (int k = 0; k <= n; ++k)
        {
            test_partial_sort(ex::seq, input, k);
        }

        for (int nth = 0; nth != n; ++nth)
        {
            test_nth_element(ex::seq, input, nth);
        }
    }
}

int hpx_main()
{
    test_issue_case();
    test_all_sizes();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
