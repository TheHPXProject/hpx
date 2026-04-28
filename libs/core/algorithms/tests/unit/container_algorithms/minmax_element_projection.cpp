//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

void test_min_element_projection()
{
    using namespace hpx::execution;
    using element = std::pair<int, int>;
    std::vector<element> c = {{1, 10}, {2, 5}, {3, 15}};

    auto proj = [](element const& p) { return p.second; };

    // Test sequential
    auto r_seq = hpx::ranges::min_element(
        seq, c.begin(), c.end(), std::less<int>{}, proj);
    HPX_TEST(r_seq != c.end());
    HPX_TEST_EQ(r_seq->second, 5);

    // Test parallel
    auto r_par = hpx::ranges::min_element(
        par, c.begin(), c.end(), std::less<int>{}, proj);
    HPX_TEST(r_par != c.end());
    HPX_TEST_EQ(r_par->second, 5);
}

void test_max_element_projection()
{
    using namespace hpx::execution;
    using element = std::pair<int, int>;
    std::vector<element> c = {{1, 10}, {2, 5}, {3, 15}};

    auto proj = [](element const& p) { return p.second; };

    // Test sequential
    auto r_seq = hpx::ranges::max_element(
        seq, c.begin(), c.end(), std::less<int>{}, proj);
    HPX_TEST(r_seq != c.end());
    HPX_TEST_EQ(r_seq->second, 15);

    // Test parallel
    auto r_par = hpx::ranges::max_element(
        par, c.begin(), c.end(), std::less<int>{}, proj);
    HPX_TEST(r_par != c.end());
    HPX_TEST_EQ(r_par->second, 15);
}

void test_minmax_element_projection()
{
    using namespace hpx::execution;
    using element = std::pair<int, int>;
    std::vector<element> c = {{1, 10}, {2, 5}, {3, 15}};

    auto proj = [](element const& p) { return p.second; };

    // Test sequential
    auto r_seq = hpx::ranges::minmax_element(
        seq, c.begin(), c.end(), std::less<int>{}, proj);
    HPX_TEST(r_seq.min != c.end());
    HPX_TEST(r_seq.max != c.end());
    HPX_TEST_EQ(r_seq.min->second, 5);
    HPX_TEST_EQ(r_seq.max->second, 15);

    // Test parallel
    auto r_par = hpx::ranges::minmax_element(
        par, c.begin(), c.end(), std::less<int>{}, proj);
    HPX_TEST(r_par.min != c.end());
    HPX_TEST(r_par.max != c.end());
    HPX_TEST_EQ(r_par.min->second, 5);
    HPX_TEST_EQ(r_par.max->second, 15);
}

int hpx_main(hpx::program_options::variables_map&)
{
    test_min_element_projection();
    test_max_element_projection();
    test_minmax_element_projection();

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
