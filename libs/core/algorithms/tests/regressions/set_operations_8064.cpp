//  Copyright (c) 2026 vivs24307-maker
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #8064: the parallel set operations split the first range into one chunk
// per core and let the partitioner of the given policy run them. A chunk size
// larger than one made the partitioner hand several of these chunks to a
// single task, which processed only the first of them. The output of all
// others was silently dropped, e.g. set_difference of {1, ..., 8} and
// {2, 4, 6, 8} with static_chunk_size(2) on two cores returned {1, 3} instead
// of {1, 3, 5, 7}.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <random>
#include <vector>

using vec = std::vector<int>;

template <typename ExPolicy>
void test_set_operations(ExPolicy&& policy, vec const& a, vec const& b)
{
    vec expected(a.size() + b.size());
    vec result(a.size() + b.size());

    {
        auto const expected_end = std::set_difference(
            a.begin(), a.end(), b.begin(), b.end(), expected.begin());
        auto const result_end = hpx::set_difference(
            policy, a.begin(), a.end(), b.begin(), b.end(), result.begin());

        HPX_TEST_EQ(
            result_end - result.begin(), expected_end - expected.begin());
        HPX_TEST(std::equal(result.begin(), result_end, expected.begin()));
    }

    {
        auto const expected_end = std::set_intersection(
            a.begin(), a.end(), b.begin(), b.end(), expected.begin());
        auto const result_end = hpx::set_intersection(
            policy, a.begin(), a.end(), b.begin(), b.end(), result.begin());

        HPX_TEST_EQ(
            result_end - result.begin(), expected_end - expected.begin());
        HPX_TEST(std::equal(result.begin(), result_end, expected.begin()));
    }

    {
        auto const expected_end = std::set_union(
            a.begin(), a.end(), b.begin(), b.end(), expected.begin());
        auto const result_end = hpx::set_union(
            policy, a.begin(), a.end(), b.begin(), b.end(), result.begin());

        HPX_TEST_EQ(
            result_end - result.begin(), expected_end - expected.begin());
        HPX_TEST(std::equal(result.begin(), result_end, expected.begin()));
    }

    {
        auto const expected_end = std::set_symmetric_difference(
            a.begin(), a.end(), b.begin(), b.end(), expected.begin());
        auto const result_end = hpx::set_symmetric_difference(
            policy, a.begin(), a.end(), b.begin(), b.end(), result.begin());

        HPX_TEST_EQ(
            result_end - result.begin(), expected_end - expected.begin());
        HPX_TEST(std::equal(result.begin(), result_end, expected.begin()));
    }
}

template <typename ExPolicy>
void test_with_policy(ExPolicy&& policy)
{
    // the case from the description above
    test_set_operations(policy, vec{1, 2, 3, 4, 5, 6, 7, 8}, vec{2, 4, 6, 8});

    // sorted random ranges of various sizes, with duplicates
    std::mt19937 gen(42);
    for (std::size_t size : {2, 3, 7, 16, 33, 100, 257, 1000})
    {
        std::uniform_int_distribution<int> dist(0, static_cast<int>(size));

        vec a(size);
        vec b(size / 2 + 1);
        std::generate(a.begin(), a.end(), [&] { return dist(gen); });
        std::generate(b.begin(), b.end(), [&] { return dist(gen); });
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());

        test_set_operations(policy, a, b);
        test_set_operations(policy, b, a);
    }
}

int hpx_main()
{
    namespace ex = hpx::execution;

    test_with_policy(ex::par);
    for (std::size_t chunk_size : {1, 2, 3, 5, 100})
    {
        test_with_policy(
            ex::par.with(ex::experimental::static_chunk_size(chunk_size)));
        test_with_policy(
            ex::par.with(ex::experimental::dynamic_chunk_size(chunk_size)));
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
