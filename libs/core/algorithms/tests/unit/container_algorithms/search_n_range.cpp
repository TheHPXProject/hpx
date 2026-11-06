//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

struct user_defined_type
{
    user_defined_type() = default;
    explicit user_defined_type(std::size_t v)
      : val(v)
    {
    }

    std::size_t val{};
};

void search_test_n_basic()
{
    std::vector<std::size_t> c(10007);

    std::fill(c.begin(), c.end(), std::size_t{5});

    std::size_t mid = c.size() / 2;
    c[mid] = 1;
    c[mid + 1] = 1;

    auto it = hpx::search_n(c.begin(), c.end(), 2, std::size_t{1});
    auto expected = c.begin() + static_cast<std::ptrdiff_t>(mid);

    HPX_TEST(it == expected);
}

void search_test_n_begin()
{
    std::vector<std::size_t> c(10007);

    std::fill(c.begin(), c.end(), std::size_t{5});

    c[0] = 2;
    c[1] = 2;

    auto it = hpx::search_n(c.begin(), c.end(), 2, std::size_t{2});

    HPX_TEST(it == c.begin());
}

void search_test_n_predicate()
{
    std::vector<std::size_t> c(10007);
    std::fill(c.begin(), c.end(), 5);

    std::size_t mid = c.size() / 2;
    c[mid] = 3;
    c[mid + 1] = 3;

    auto pred = [](std::size_t a, std::size_t b) { return a == b; };

    auto it = hpx::search_n(c.begin(), c.end(), 2, std::size_t{3}, pred);

    auto expected = c.begin() + static_cast<std::ptrdiff_t>(mid);

    HPX_TEST(it == expected);
}

void search_test_n_not_found()
{
    std::vector<std::size_t> c(100, 5);

    auto it = hpx::search_n(c.begin(), c.end(), 2, std::size_t{1});

    HPX_TEST(it == c.end());
}

void search_test_n_zero_count()
{
    std::vector<std::size_t> c(100, 5);

    auto it = hpx::search_n(c.begin(), c.end(), 0, std::size_t{42});

    HPX_TEST(it == c.begin());
}

void search_test_n_count_too_large()
{
    std::vector<std::size_t> c(5, 1);

    auto it = hpx::search_n(c.begin(), c.end(), 10, std::size_t{1});

    HPX_TEST(it == c.end());
}

///////////////////////////////////////////////////////////////////////////////
// The tests above exercise the non-ranges hpx::search_n. The tests below
// exercise hpx::ranges::search_n itself (which used to be ill-formed in
// all of its overloads, so no call to it would compile).

void ranges_search_n_test_basic()
{
    std::vector<std::size_t> c(10007);
    std::fill(c.begin(), c.end(), std::size_t{5});

    std::size_t mid = c.size() / 2;
    c[mid] = 1;
    c[mid + 1] = 1;

    auto expected = c.begin() + static_cast<std::ptrdiff_t>(mid);

    // iterator overload, no execution policy
    HPX_TEST(hpx::ranges::search_n(c.begin(), c.end(), 2, std::size_t{1}) ==
        expected);

    // iterator overloads with execution policies
    HPX_TEST(hpx::ranges::search_n(hpx::execution::seq, c.begin(), c.end(), 2,
                 std::size_t{1}) == expected);
    HPX_TEST(hpx::ranges::search_n(hpx::execution::par, c.begin(), c.end(), 2,
                 std::size_t{1}) == expected);

    // range overload, no execution policy
    HPX_TEST(hpx::ranges::search_n(c, 2, std::size_t{1}) == expected);

    // range overload with an execution policy
    HPX_TEST(hpx::ranges::search_n(hpx::execution::par, c, 2, std::size_t{1}) ==
        expected);
}

void ranges_search_n_test_predicate()
{
    std::vector<std::size_t> c(10007);
    std::fill(c.begin(), c.end(), std::size_t{5});

    std::size_t mid = c.size() / 2;
    c[mid] = 3;
    c[mid + 1] = 3;

    auto expected = c.begin() + static_cast<std::ptrdiff_t>(mid);

    auto pred = [](std::size_t a, std::size_t b) { return a == b; };

    HPX_TEST(hpx::ranges::search_n(
                 c.begin(), c.end(), 2, std::size_t{3}, pred) == expected);
    HPX_TEST(hpx::ranges::search_n(
                 hpx::execution::par, c, 2, std::size_t{3}, pred) == expected);
}

void ranges_search_n_test_projection()
{
    std::vector<user_defined_type> c(10007, user_defined_type(5));

    std::size_t mid = c.size() / 2;
    c[mid] = user_defined_type(1);
    c[mid + 1] = user_defined_type(1);

    auto expected = c.begin() + static_cast<std::ptrdiff_t>(mid);

    auto proj = [](user_defined_type const& e) { return e.val; };

    HPX_TEST(hpx::ranges::search_n(c.begin(), c.end(), 2, user_defined_type(1),
                 hpx::ranges::equal_to{}, proj) == expected);
    HPX_TEST(
        hpx::ranges::search_n(hpx::execution::par, c, 2, user_defined_type(1),
            hpx::ranges::equal_to{}, proj) == expected);
}

void ranges_search_n_test_sentinel()
{
    std::vector<std::size_t> c(100, 5);
    c[10] = 1;
    c[11] = 1;

    // a range whose sentinel type differs from its iterator type
    auto rng = std::ranges::subrange(
        std::counted_iterator(c.begin(), c.size()), std::default_sentinel);

    auto it = hpx::ranges::search_n(rng, 2, std::size_t{1});

    HPX_TEST(it.base() == c.begin() + 10);
}

void ranges_search_n_test_not_found()
{
    std::vector<std::size_t> c(100, 5);

    HPX_TEST(hpx::ranges::search_n(c.begin(), c.end(), 2, std::size_t{1}) ==
        c.end());
    HPX_TEST(hpx::ranges::search_n(hpx::execution::par, c, 2, std::size_t{1}) ==
        c.end());
}

void ranges_search_n_test_zero_count()
{
    std::vector<std::size_t> c(100, 5);

    HPX_TEST(hpx::ranges::search_n(c, 0, std::size_t{42}) == c.begin());
    HPX_TEST(hpx::ranges::search_n(
                 hpx::execution::par, c, 0, std::size_t{42}) == c.begin());
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    search_test_n_basic();
    search_test_n_begin();
    search_test_n_predicate();
    search_test_n_not_found();
    search_test_n_zero_count();
    search_test_n_count_too_large();

    ranges_search_n_test_basic();
    ranges_search_n_test_predicate();
    ranges_search_n_test_projection();
    ranges_search_n_test_sentinel();
    ranges_search_n_test_not_found();
    ranges_search_n_test_zero_count();

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
