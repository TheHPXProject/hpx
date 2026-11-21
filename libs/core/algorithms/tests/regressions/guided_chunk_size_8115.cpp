//  Copyright (c) 2026 vivs24307-maker
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #8115: with executor parameters that have a variable chunk size (e.g.
// guided_chunk_size) the executor returns one future holding the results of
// all chunks. The partitioner passed that future to the reduction function,
// which expects the results themselves, so the algorithms below failed to
// compile with such a policy.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <random>
#include <vector>

using vec = std::vector<int>;

template <typename ExPolicy>
void test_algorithms(ExPolicy&& policy, vec const& a, vec const& b)
{
    auto const pred = [](int x) { return x % 3 == 0; };

    HPX_TEST_EQ(hpx::all_of(policy, a.begin(), a.end(), pred),
        std::all_of(a.begin(), a.end(), pred));
    HPX_TEST_EQ(hpx::any_of(policy, a.begin(), a.end(), pred),
        std::any_of(a.begin(), a.end(), pred));
    HPX_TEST_EQ(hpx::none_of(policy, a.begin(), a.end(), pred),
        std::none_of(a.begin(), a.end(), pred));
    HPX_TEST_EQ(hpx::ranges::all_of(policy, a, pred),
        std::all_of(a.begin(), a.end(), pred));
    HPX_TEST_EQ(hpx::ranges::none_of(policy, a, pred),
        std::none_of(a.begin(), a.end(), pred));

    HPX_TEST_EQ(hpx::equal(policy, a.begin(), a.end(), b.begin()),
        std::equal(a.begin(), a.end(), b.begin()));
    HPX_TEST(hpx::equal(policy, a.begin(), a.end(), a.begin()));

    HPX_TEST_EQ(hpx::is_sorted(policy, a.begin(), a.end()),
        std::is_sorted(a.begin(), a.end()));
    HPX_TEST_EQ(hpx::is_partitioned(policy, a.begin(), a.end(), pred),
        std::is_partitioned(a.begin(), a.end(), pred));

    HPX_TEST_EQ(hpx::transform_reduce(policy, a.begin(), a.end(), b.begin(), 0),
        std::transform_reduce(a.begin(), a.end(), b.begin(), 0));

    vec sa = a;
    vec sb = b;
    std::sort(sa.begin(), sa.end());
    std::sort(sb.begin(), sb.end());
    std::size_t const half = sb.size() / 2;

    HPX_TEST(hpx::is_sorted(policy, sa.begin(), sa.end()));
    HPX_TEST_EQ(hpx::includes(policy, sa.begin(), sa.end(), sb.begin(),
                    sb.begin() + half),
        std::includes(sa.begin(), sa.end(), sb.begin(), sb.begin() + half));

    vec pa = a;
    std::partition(pa.begin(), pa.end(), pred);
    HPX_TEST(hpx::is_partitioned(policy, pa.begin(), pa.end(), pred));
}

// the hierarchical spawning parallel_executor returns one future per chunk
void test_hierarchical_executor(vec const& a, vec const& b)
{
    namespace ex = hpx::execution;

    auto const pred = [](int x) { return x % 3 == 0; };
    auto exec = ex::to_hierarchical_spawning(ex::parallel_executor());

    test_algorithms(
        ex::par.on(exec).with(ex::experimental::guided_chunk_size(2)), a, b);

    auto policy = ex::par(ex::task).on(exec);
    HPX_TEST_EQ(hpx::is_partitioned(policy, a.begin(), a.end(), pred).get(),
        std::is_partitioned(a.begin(), a.end(), pred));
    HPX_TEST_EQ(hpx::all_of(policy, a.begin(), a.end(), pred).get(),
        std::all_of(a.begin(), a.end(), pred));
    HPX_TEST_EQ(hpx::none_of(policy, a.begin(), a.end(), pred).get(),
        std::none_of(a.begin(), a.end(), pred));
    HPX_TEST_EQ(hpx::count(policy, a.begin(), a.end(), 3).get(),
        std::count(a.begin(), a.end(), 3));
    HPX_TEST_EQ(
        hpx::transform_reduce(policy, a.begin(), a.end(), b.begin(), 0).get(),
        std::transform_reduce(a.begin(), a.end(), b.begin(), 0));
}

int hpx_main()
{
    namespace ex = hpx::execution;

    std::mt19937 gen(42);
    for (std::size_t size : {0, 1, 2, 7, 64, 100, 1000, 10007})
    {
        std::uniform_int_distribution<int> dist(0, 10);

        vec a(size);
        vec b(size);
        std::generate(a.begin(), a.end(), [&] { return dist(gen); });
        std::generate(b.begin(), b.end(), [&] { return dist(gen); });

        for (std::size_t chunk_size : {1, 2, 5, 100})
        {
            test_algorithms(
                ex::par.with(ex::experimental::guided_chunk_size(chunk_size)),
                a, b);
        }

        test_hierarchical_executor(a, b);
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
