//  Copyright (c) 2026 vivs24307-maker
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Follow-up to #8115: the algorithms built on the scan partitioner (copy_if,
// the scans, partition_copy, unique_copy, ...) and merge failed to compile
// with executor parameters that have a variable chunk size (e.g.
// guided_chunk_size). The scan partitioner forwarded arguments the variable
// chunk size path doesn't take, and the for_each partitioner didn't pass the
// number of cores to merge's reshape function.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <numeric>
#include <random>
#include <vector>

using vec = std::vector<int>;

template <typename ExPolicy>
void test_algorithms(ExPolicy&& policy, vec const& a, vec const& b)
{
    auto const pred = [](int x) { return x % 3 == 0; };
    auto const twice = [](int x) { return 2 * x; };
    std::size_t const n = a.size();

    {
        vec r(n), e(n);
        auto const rend =
            hpx::copy_if(policy, a.begin(), a.end(), r.begin(), pred);
        auto const eend = std::copy_if(a.begin(), a.end(), e.begin(), pred);
        HPX_TEST_EQ(rend - r.begin(), eend - e.begin());
        HPX_TEST(r == e);
    }
    {
        vec r(n), e(n);
        auto const rend =
            hpx::remove_copy_if(policy, a.begin(), a.end(), r.begin(), pred);
        auto const eend =
            std::remove_copy_if(a.begin(), a.end(), e.begin(), pred);
        HPX_TEST_EQ(rend - r.begin(), eend - e.begin());
        HPX_TEST(r == e);
    }
    {
        vec r(n), e(n);
        auto const rend =
            hpx::unique_copy(policy, a.begin(), a.end(), r.begin());
        auto const eend = std::unique_copy(a.begin(), a.end(), e.begin());
        HPX_TEST_EQ(rend - r.begin(), eend - e.begin());
        HPX_TEST(r == e);
    }
    {
        vec r1(n), r2(n), e1(n), e2(n);
        auto const rend = hpx::partition_copy(
            policy, a.begin(), a.end(), r1.begin(), r2.begin(), pred);
        auto const eend = std::partition_copy(
            a.begin(), a.end(), e1.begin(), e2.begin(), pred);
        HPX_TEST_EQ(rend.first - r1.begin(), eend.first - e1.begin());
        HPX_TEST_EQ(rend.second - r2.begin(), eend.second - e2.begin());
        HPX_TEST(r1 == e1);
        HPX_TEST(r2 == e2);
    }
    {
        vec r(n), e(n);
        hpx::inclusive_scan(policy, a.begin(), a.end(), r.begin());
        std::inclusive_scan(a.begin(), a.end(), e.begin());
        HPX_TEST(r == e);
    }
    {
        vec r(n), e(n);
        hpx::exclusive_scan(policy, a.begin(), a.end(), r.begin(), 7);
        std::exclusive_scan(a.begin(), a.end(), e.begin(), 7);
        HPX_TEST(r == e);
    }
    {
        vec r(n), e(n);
        hpx::transform_inclusive_scan(
            policy, a.begin(), a.end(), r.begin(), std::plus<>(), twice);
        std::transform_inclusive_scan(
            a.begin(), a.end(), e.begin(), std::plus<>(), twice);
        HPX_TEST(r == e);
    }
    {
        vec r(n), e(n);
        hpx::transform_exclusive_scan(
            policy, a.begin(), a.end(), r.begin(), 7, std::plus<>(), twice);
        std::transform_exclusive_scan(
            a.begin(), a.end(), e.begin(), 7, std::plus<>(), twice);
        HPX_TEST(r == e);
    }
    {
        vec sa = a;
        vec sb = b;
        std::sort(sa.begin(), sa.end());
        std::sort(sb.begin(), sb.end());

        vec r(sa.size() + sb.size()), e(sa.size() + sb.size());
        auto const rend = hpx::merge(
            policy, sa.begin(), sa.end(), sb.begin(), sb.end(), r.begin());
        auto const eend =
            std::merge(sa.begin(), sa.end(), sb.begin(), sb.end(), e.begin());
        HPX_TEST_EQ(rend - r.begin(), eend - e.begin());
        HPX_TEST(r == e);
    }
}

int hpx_main()
{
    namespace ex = hpx::execution;

    std::mt19937 gen(42);
    for (std::size_t size : {1, 2, 7, 64, 100, 1000, 10007})
    {
        std::uniform_int_distribution<int> dist(0, 10);

        vec a(size);
        vec b(size / 2 + 1);
        std::generate(a.begin(), a.end(), [&] { return dist(gen); });
        std::generate(b.begin(), b.end(), [&] { return dist(gen); });

        for (std::size_t chunk_size : {1, 2, 5, 100})
        {
            test_algorithms(
                ex::par.with(ex::experimental::guided_chunk_size(chunk_size)),
                a, b);
        }
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
