//  Copyright (c) 2026 Ishaan400
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// With the sequential policy, hpx::ranges::transform over two ranges of
// different length returned the end of both inputs as in1 and in2 instead of
// the position where the shorter range ran out. hpx::ranges::set_intersection
// returned the position where the loop stopped instead of the end of both
// inputs. Both now match std::ranges.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <vector>

template <typename Container>
void test_transform(Container const& a, Container const& b)
{
    std::vector<int> d1(a.size() + b.size()), d2(a.size() + b.size());

    auto expected = std::ranges::transform(a, b, d1.begin(), std::plus<>());
    auto result = hpx::ranges::transform(
        hpx::execution::seq, a, b, d2.begin(), std::plus<>());

    HPX_TEST(d1 == d2);
    HPX_TEST_EQ(std::distance(a.begin(), result.in1),
        std::distance(a.begin(), expected.in1));
    HPX_TEST_EQ(std::distance(b.begin(), result.in2),
        std::distance(b.begin(), expected.in2));
    HPX_TEST_EQ(std::distance(d2.begin(), result.out),
        std::distance(d1.begin(), expected.out));

    // the parallel version already behaved correctly, keep it that way
    auto par_result = hpx::ranges::transform(
        hpx::execution::par, a, b, d2.begin(), std::plus<>());
    HPX_TEST_EQ(std::distance(a.begin(), par_result.in1),
        std::distance(a.begin(), expected.in1));
    HPX_TEST_EQ(std::distance(b.begin(), par_result.in2),
        std::distance(b.begin(), expected.in2));
}

template <typename Container>
void test_set_intersection(Container const& a, Container const& b)
{
    std::vector<int> d1(a.size() + b.size()), d2(a.size() + b.size());

    auto expected = std::ranges::set_intersection(a, b, d1.begin());
    auto result =
        hpx::ranges::set_intersection(hpx::execution::seq, a, b, d2.begin());

    HPX_TEST(d1 == d2);
    HPX_TEST(result.in1 == a.end());
    HPX_TEST(result.in2 == b.end());
    HPX_TEST_EQ(std::distance(a.begin(), result.in1),
        std::distance(a.begin(), expected.in1));
    HPX_TEST_EQ(std::distance(b.begin(), result.in2),
        std::distance(b.begin(), expected.in2));
    HPX_TEST_EQ(std::distance(d2.begin(), result.out),
        std::distance(d1.begin(), expected.out));
}

template <typename Container>
void test_all()
{
    Container shorter{1, 2};
    Container longer{1, 2, 3, 4, 5};
    Container empty;

    test_transform(shorter, longer);
    test_transform(longer, shorter);
    test_transform(empty, longer);
    test_transform(longer, longer);

    // the first input runs out first, then the second one
    test_set_intersection(Container{1, 3}, Container{2, 3, 4, 5});
    test_set_intersection(Container{1, 3, 5, 7, 9}, Container{2, 3});
    test_set_intersection(empty, longer);
    test_set_intersection(longer, empty);
    test_set_intersection(Container{1, 2, 3, 4, 5, 6, 7, 8}, Container{9});
}

int hpx_main()
{
    test_all<std::vector<int>>();
    test_all<std::list<int>>();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
