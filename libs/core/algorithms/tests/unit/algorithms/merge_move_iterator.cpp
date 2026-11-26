//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpx::merge and hpx::ranges::merge with std::move_iterator inputs have to
// move each element to the output once. The sequential merge used to copy
// elements into local values before it knew which one was written next; with
// a move_iterator that moved elements out of the inputs too early, and the
// output got moved-from (empty) strings. The parallel merge read elements
// while other chunks moved them, and crashed.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

// zero-padded numbers longer than the small-string buffer: sorted as strings
// they are sorted as numbers, and a moved-from string is empty
std::vector<std::string> make_input(
    std::size_t count, std::size_t first, std::size_t step)
{
    std::vector<std::string> result;
    result.reserve(count);
    for (std::size_t i = 0; i != count; ++i)
    {
        std::string const number = std::to_string(first + i * step);
        result.push_back(std::string(40 - number.size(), '0') + number);
    }
    return result;
}

// Merge calls f(first1, last1, first2, last2, dest) with move iterators over
// the inputs; multiples of 2 and of 3 plus 1 interleave and share keys
template <typename Merge>
void test_merge_move(Merge&& merge, std::size_t count)
{
    std::vector<std::string> first = make_input(count, 0, 2);
    std::vector<std::string> second = make_input(count + 3, 1, 3);

    std::vector<std::string> expected(first.size() + second.size());
    std::merge(first.begin(), first.end(), second.begin(), second.end(),
        expected.begin());

    std::vector<std::string> result(expected.size());
    merge(std::make_move_iterator(first.begin()),
        std::make_move_iterator(first.end()),
        std::make_move_iterator(second.begin()),
        std::make_move_iterator(second.end()), result.begin());

    HPX_TEST(result == expected);
}

template <typename Merge>
void test_merge_move(Merge&& merge)
{
    // small, and large enough for several chunks of the parallel merge
    test_merge_move(merge, 7);
    test_merge_move(merge, 10007);
}

// a projection that returns its argument, so the merge takes the code path
// with projections
struct project
{
    std::string const& operator()(std::string const& value) const noexcept
    {
        return value;
    }
};

template <typename ExPolicy>
void test_merge_move_policy(ExPolicy policy)
{
    test_merge_move(
        [&](auto first1, auto last1, auto first2, auto last2, auto dest) {
            hpx::merge(policy, first1, last1, first2, last2, dest);
        });

    test_merge_move(
        [&](auto first1, auto last1, auto first2, auto last2, auto dest) {
            hpx::ranges::merge(policy, first1, last1, first2, last2, dest);
        });

    test_merge_move(
        [&](auto first1, auto last1, auto first2, auto last2, auto dest) {
            hpx::ranges::merge(policy, first1, last1, first2, last2, dest,
                std::less<>(), project(), project());
        });
}

template <typename ExPolicy>
void test_merge_move_policy_async(ExPolicy policy)
{
    test_merge_move(
        [&](auto first1, auto last1, auto first2, auto last2, auto dest) {
            hpx::merge(policy, first1, last1, first2, last2, dest).get();
        });

    test_merge_move(
        [&](auto first1, auto last1, auto first2, auto last2, auto dest) {
            hpx::ranges::merge(policy, first1, last1, first2, last2, dest,
                std::less<>(), project(), project())
                .get();
        });
}

int hpx_main()
{
    test_merge_move(
        [](auto first1, auto last1, auto first2, auto last2, auto dest) {
            hpx::merge(first1, last1, first2, last2, dest);
        });

    test_merge_move(
        [](auto first1, auto last1, auto first2, auto last2, auto dest) {
            hpx::ranges::merge(first1, last1, first2, last2, dest,
                std::less<>(), project(), project());
        });

    test_merge_move_policy(hpx::execution::seq);
    test_merge_move_policy(hpx::execution::par);
    test_merge_move_policy(hpx::execution::par_unseq);

    test_merge_move_policy_async(hpx::execution::seq(hpx::execution::task));
    test_merge_move_policy_async(hpx::execution::par(hpx::execution::task));

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
