//  Copyright (c) 2026 Ayush (@Ayush-CS-89112521)
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// partial_sort must ensure that [first, middle) contains the smallest
// middle - first elements of the entire range [first, last). When nmid > 1024,
// partial_sort used to return prematurely if [first, middle) was sorted,
// ignoring smaller elements present in the suffix [middle, last).

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <numeric>
#include <random>
#include <string>
#include <vector>

template <typename ExPolicy, typename Comp = std::less<int>>
void verify_invariants(ExPolicy&& policy,
    std::vector<int> const& original,
    std::size_t nmid,
    Comp comp = Comp())
{
    std::vector<int> result = original;
    hpx::partial_sort(std::forward<ExPolicy>(policy),
        result.begin(), result.begin() + nmid, result.end(), comp);

    // Invariant A: Prefix sortedness
    HPX_TEST(std::is_sorted(result.begin(), result.begin() + nmid, comp));

    // Invariant B: Partition correctness (no element in suffix precedes
    // boundary)
    if (nmid > 0 && nmid < result.size())
    {
        auto const& boundary = result[nmid - 1];
        for (auto it = result.begin() + nmid; it != result.end(); ++it)
        {
            HPX_TEST(!comp(*it, boundary));
        }
    }

    // Invariant C: Permutation preservation (multiset equality)
    std::vector<int> sorted_orig = original;
    std::vector<int> sorted_res = result;
    std::sort(sorted_orig.begin(), sorted_orig.end(), comp);
    std::sort(sorted_res.begin(), sorted_res.end(), comp);
    HPX_TEST(sorted_orig == sorted_res);
}

template <typename ExPolicy>
void test_policy(ExPolicy&& policy)
{
    // 1. Boundary threshold testing: prefix sorted with nmid > 1024,
    // suffix contains smaller elements
    for (std::size_t nmid : {1025, 1200})
    {
        std::size_t const total_size = nmid + 800;
        std::vector<int> v(total_size);
        // Prefix is sorted: 1000, 1001, ..., 1000 + nmid - 1
        for (std::size_t i = 0; i < nmid; ++i)
        {
            v[i] = static_cast<int>(1000 + i);
        }
        // Suffix contains elements smaller than prefix boundary
        // (e.g. 500..899) and larger elements
        std::mt19937 gen(42);
        for (std::size_t i = nmid; i < total_size; ++i)
        {
            v[i] = static_cast<int>(500 + (gen() % 2000));
        }

        verify_invariants(policy, v, nmid);
    }

    // 2. Entire array sorted (fast path preserved)
    {
        std::vector<int> v(2000);
        std::iota(v.begin(), v.end(), 0);
        verify_invariants(policy, v, 1500);
    }

    // 3. Prefix sorted, suffix unordered but strictly larger
    // (valid partial sort fast path)
    {
        std::vector<int> v(2000);
        for (int i = 0; i < 1200; ++i)
        {
            v[i] = i;
        }
        std::mt19937 gen(1234);
        for (int i = 1200; i < 2000; ++i)
        {
            v[i] = static_cast<int>(2000 + (gen() % 1000));
        }
        verify_invariants(policy, v, 1200);
    }

    // 4. Prefix with duplicates and alien smaller suffix elements
    {
        std::size_t const nmid = 1100;
        std::vector<int> v;
        v.reserve(nmid + 500);
        for (std::size_t i = 0; i < nmid / 2; ++i)
        {
            v.push_back(static_cast<int>(1000 + i));
            v.push_back(static_cast<int>(1000 + i));
        }
        std::mt19937 gen(5678);
        for (std::size_t i = 0; i < 500; ++i)
        {
            v.push_back(static_cast<int>(500 + (gen() % 1500)));
        }
        verify_invariants(policy, v, nmid);
    }

    // 5. Descending comparator std::greater<int>
    {
        std::size_t const nmid = 1050;
        std::size_t const total_size = 1800;
        std::vector<int> v(total_size);
        for (std::size_t i = 0; i < nmid; ++i)
        {
            v[i] = static_cast<int>(3000 - i);
        }
        std::mt19937 gen(999);
        for (std::size_t i = nmid; i < total_size; ++i)
        {
            v[i] = static_cast<int>(2000 + (gen() % 3000));
        }
        verify_invariants(policy, v, nmid, std::greater<int>());
    }

    // 6. Edge boundaries: middle == first and middle == last
    {
        std::vector<int> v = {5, 4, 3, 2, 1};
        verify_invariants(policy, v, 0);
        verify_invariants(policy, v, v.size());
    }
}

void test_default()
{
    std::size_t const nmid = 1025;
    std::size_t const total_size = nmid + 800;
    std::vector<int> v(total_size);
    for (std::size_t i = 0; i < nmid; ++i)
    {
        v[i] = static_cast<int>(1000 + i);
    }
    std::mt19937 gen(42);
    for (std::size_t i = nmid; i < total_size; ++i)
    {
        v[i] = static_cast<int>(500 + (gen() % 2000));
    }

    std::vector<int> result = v;
    hpx::partial_sort(result.begin(), result.begin() + nmid, result.end());

    HPX_TEST(std::is_sorted(result.begin(), result.begin() + nmid));
    auto const& boundary = result[nmid - 1];
    for (auto it = result.begin() + nmid; it != result.end(); ++it)
    {
        HPX_TEST(!(*it < boundary));
    }

    std::vector<int> sorted_orig = v;
    std::vector<int> sorted_res = result;
    std::sort(sorted_orig.begin(), sorted_orig.end());
    std::sort(sorted_res.begin(), sorted_res.end());
    HPX_TEST(sorted_orig == sorted_res);
}

int hpx_main()
{
    test_default();
    test_policy(hpx::execution::seq);
    test_policy(hpx::execution::par);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
