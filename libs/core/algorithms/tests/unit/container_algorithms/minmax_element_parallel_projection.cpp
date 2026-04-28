//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for: incorrect return types in
// max_element::sequential_minmax_element_ind and
// minmax_element::sequential_minmax_element_ind, which are called exclusively
// in the parallel reduction phase. This file exercises the projection + par
// path to ensure the parallel aggregation returns an iterator (not a copy).

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <ctime>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// test_min_element_parallel_projection
// Verifies that hpx::ranges::min_element with par and a projection correctly
// identifies the minimum by projected value, exercising the parallel
// reduction path (sequential_minmax_element_ind returns Iter).
// ---------------------------------------------------------------------------
template <typename ExPolicy>
void test_min_element_parallel_projection(ExPolicy policy)
{
    // Pairs: compare by .second (the projected key)
    std::vector<std::pair<int, int>> c = {
        {10, 5}, {20, 1}, {30, 8}, {40, 3}, {50, 7}};

    auto proj = [](std::pair<int, int> const& p) { return p.second; };

    auto it = hpx::ranges::min_element(
        policy, c.begin(), c.end(), std::less<int>{}, proj);

    HPX_TEST(it != c.end());
    // Minimum second value is 1, belonging to {20, 1}
    HPX_TEST_EQ(it->second, 1);
    HPX_TEST_EQ(it->first, 20);
}

// ---------------------------------------------------------------------------
// test_max_element_parallel_projection
// Verifies that hpx::ranges::max_element with par and a projection correctly
// identifies the maximum by projected value, exercising the parallel
// reduction path (sequential_minmax_element_ind returns Iter, not value_type).
// ---------------------------------------------------------------------------
template <typename ExPolicy>
void test_max_element_parallel_projection(ExPolicy policy)
{
    std::vector<std::pair<int, int>> c = {
        {10, 5}, {20, 1}, {30, 8}, {40, 3}, {50, 7}};

    auto proj = [](std::pair<int, int> const& p) { return p.second; };

    auto it = hpx::ranges::max_element(
        policy, c.begin(), c.end(), std::less<int>{}, proj);

    HPX_TEST(it != c.end());
    // Maximum second value is 8, belonging to {30, 8}
    HPX_TEST_EQ(it->second, 8);
    HPX_TEST_EQ(it->first, 30);
}

// ---------------------------------------------------------------------------
// test_minmax_element_parallel_projection
// Verifies that hpx::ranges::minmax_element with par and a projection correctly
// identifies both min and max by projected value, exercising the parallel
// reduction path (sequential_minmax_element_ind returns minmax_element_result<Iter>).
// ---------------------------------------------------------------------------
template <typename ExPolicy>
void test_minmax_element_parallel_projection(ExPolicy policy)
{
    std::vector<std::pair<int, int>> c = {
        {10, 5}, {20, 1}, {30, 8}, {40, 3}, {50, 7}};

    auto proj = [](std::pair<int, int> const& p) { return p.second; };

    auto result = hpx::ranges::minmax_element(
        policy, c.begin(), c.end(), std::less<int>{}, proj);

    HPX_TEST(result.min != c.end());
    HPX_TEST(result.max != c.end());

    // Min by second value: 1 => {20, 1}
    HPX_TEST_EQ(result.min->second, 1);
    HPX_TEST_EQ(result.min->first, 20);

    // Max by second value: 8 => {30, 8}
    HPX_TEST_EQ(result.max->second, 8);
    HPX_TEST_EQ(result.max->first, 30);
}

// ---------------------------------------------------------------------------
// Larger dataset tests: forces actual parallel partitioning to occur,
// directly triggering the sequential_minmax_element_ind reduction path.
// ---------------------------------------------------------------------------
template <typename ExPolicy>
void test_min_element_parallel_projection_large(ExPolicy policy)
{
    // 10000 pairs: projected key is index % 97, minimum is at index 0 (key 0)
    std::size_t const N = 10000;
    std::vector<std::pair<std::size_t, int>> c;
    c.reserve(N);
    for (std::size_t i = 0; i < N; ++i)
        c.push_back({i, static_cast<int>(i % 97)});

    // Reference: minimum projected value is 0
    auto proj = [](std::pair<std::size_t, int> const& p) { return p.second; };
    auto ref_it = std::min_element(c.begin(), c.end(),
        [&proj](auto const& a, auto const& b) { return proj(a) < proj(b); });

    auto it = hpx::ranges::min_element(
        policy, c.begin(), c.end(), std::less<int>{}, proj);

    HPX_TEST(it != c.end());
    HPX_TEST_EQ(it->second, ref_it->second);
}

template <typename ExPolicy>
void test_max_element_parallel_projection_large(ExPolicy policy)
{
    std::size_t const N = 10000;
    std::vector<std::pair<std::size_t, int>> c;
    c.reserve(N);
    for (std::size_t i = 0; i < N; ++i)
        c.push_back({i, static_cast<int>(i % 97)});

    auto proj = [](std::pair<std::size_t, int> const& p) { return p.second; };
    auto ref_it = std::max_element(c.begin(), c.end(),
        [&proj](auto const& a, auto const& b) { return proj(a) < proj(b); });

    auto it = hpx::ranges::max_element(
        policy, c.begin(), c.end(), std::less<int>{}, proj);

    HPX_TEST(it != c.end());
    HPX_TEST_EQ(it->second, ref_it->second);
}

template <typename ExPolicy>
void test_minmax_element_parallel_projection_large(ExPolicy policy)
{
    std::size_t const N = 10000;
    std::vector<std::pair<std::size_t, int>> c;
    c.reserve(N);
    for (std::size_t i = 0; i < N; ++i)
        c.push_back({i, static_cast<int>(i % 97)});

    auto proj = [](std::pair<std::size_t, int> const& p) { return p.second; };

    auto ref_min = std::min_element(c.begin(), c.end(),
        [&proj](auto const& a, auto const& b) { return proj(a) < proj(b); });
    auto ref_max = std::max_element(c.begin(), c.end(),
        [&proj](auto const& a, auto const& b) { return proj(a) < proj(b); });

    auto result = hpx::ranges::minmax_element(
        policy, c.begin(), c.end(), std::less<int>{}, proj);

    HPX_TEST(result.min != c.end());
    HPX_TEST(result.max != c.end());
    HPX_TEST_EQ(result.min->second, ref_min->second);
    HPX_TEST_EQ(result.max->second, ref_max->second);
}

///////////////////////////////////////////////////////////////////////////////
void minmax_parallel_projection_test()
{
    using namespace hpx::execution;

    // Small dataset: tests correct iterator return value
    test_min_element_parallel_projection(seq);
    test_min_element_parallel_projection(par);
    test_min_element_parallel_projection(par_unseq);

    test_max_element_parallel_projection(seq);
    test_max_element_parallel_projection(par);
    test_max_element_parallel_projection(par_unseq);

    test_minmax_element_parallel_projection(seq);
    test_minmax_element_parallel_projection(par);
    test_minmax_element_parallel_projection(par_unseq);

    // Large dataset: triggers actual parallel partitioning and reduction
    test_min_element_parallel_projection_large(seq);
    test_min_element_parallel_projection_large(par);

    test_max_element_parallel_projection_large(seq);
    test_max_element_parallel_projection_large(par);

    test_minmax_element_parallel_projection_large(seq);
    test_minmax_element_parallel_projection_large(par);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    minmax_parallel_projection_test();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
