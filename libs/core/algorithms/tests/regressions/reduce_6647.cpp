//  Copyright (c) 2026 Bhoomish Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #6647: Incorrect reduce implementation

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <climits>
#include <numeric>
#include <utility>
#include <vector>

struct minmax
{
    std::pair<int, int> operator()(
        std::pair<int, int> lhs, std::pair<int, int> rhs) const
    {
        return {
            lhs.first < rhs.first ? lhs.first : rhs.first,
            lhs.second < rhs.second ? rhs.second : lhs.second,
        };
    }

    std::pair<int, int> operator()(std::pair<int, int> lhs, int rhs) const
    {
        return (*this)(lhs, std::pair<int, int>{rhs, rhs});
    }

    std::pair<int, int> operator()(int lhs, std::pair<int, int> rhs) const
    {
        return (*this)(std::pair<int, int>{lhs, lhs}, rhs);
    }

    std::pair<int, int> operator()(int lhs, int rhs) const
    {
        return (*this)(
            std::pair<int, int>{lhs, lhs}, std::pair<int, int>{rhs, rhs});
    }
};

void test_reduce_case(
    std::vector<int> const& c, std::pair<int, int> const& expected)
{
    auto const init = std::pair<int, int>{INT_MAX, INT_MIN};

    auto result =
        hpx::reduce(hpx::execution::seq, c.begin(), c.end(), init, minmax{});
    HPX_TEST_EQ(result.first, expected.first);
    HPX_TEST_EQ(result.second, expected.second);

    result =
        hpx::reduce(hpx::execution::par, c.begin(), c.end(), init, minmax{});
    HPX_TEST_EQ(result.first, expected.first);
    HPX_TEST_EQ(result.second, expected.second);
}

int hpx_main()
{
    test_reduce_case({}, {INT_MAX, INT_MIN});
    test_reduce_case({5}, {5, 5});
    test_reduce_case({3, 1}, {1, 3});
    test_reduce_case({3, 1, 4}, {1, 4});
    test_reduce_case({9, 2, 7, 1, 6}, {1, 9});
    test_reduce_case({3, 1, 4, 1, 5, 9, 2, 6}, {1, 9});

    for (int i = 1; i <= 200; ++i)
    {
        std::vector<int> v(i);
        std::iota(v.begin(), v.end(), 1);
        test_reduce_case(v, {1, i});
    }

    // Larger inputs that force multiple partitions on any machine and
    // exercise the num_elements % chunk_size == 1 edge case in
    // reduce_executor_parameters.
    for (int n : {997, 1000, 1024, 4096, 10000})
    {
        std::vector<int> v(n);
        std::iota(v.begin(), v.end(), 1);
        test_reduce_case(v, {1, n});
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
