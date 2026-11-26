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
// hpx::ranges::search_n
struct item
{
    int id;
    int payload;
};

template <typename ExPolicy>
void ranges_search_n_test(ExPolicy&& policy)
{
    std::vector<std::size_t> c(10007, 5);
    std::size_t const mid = c.size() / 2;
    c[mid] = 1;
    c[mid + 1] = 1;
    c[mid + 2] = 1;

    auto const expected = c.begin() + static_cast<std::ptrdiff_t>(mid);

    // iterator/sentinel and range overloads
    HPX_TEST(hpx::ranges::search_n(
                 policy, c.begin(), c.end(), 3, std::size_t{1}) == expected);
    HPX_TEST(hpx::ranges::search_n(policy, c, 3, std::size_t{1}) == expected);

    // not found / count too large / zero count
    HPX_TEST(hpx::ranges::search_n(policy, c, 4, std::size_t{1}) == c.end());
    HPX_TEST(
        hpx::ranges::search_n(policy, c, 100000, std::size_t{5}) == c.end());
    HPX_TEST(hpx::ranges::search_n(policy, c, 0, std::size_t{42}) == c.begin());
    HPX_TEST(hpx::ranges::search_n(policy, c, -1, std::size_t{5}) == c.begin());

    // predicate and projection
    auto const equal = [](std::size_t a, std::size_t b) { return a == b; };
    auto const twice = [](std::size_t a) { return 2 * a; };
    HPX_TEST(hpx::ranges::search_n(
                 policy, c, 3, std::size_t{2}, equal, twice) == expected);
    HPX_TEST(hpx::ranges::search_n(policy, c.begin(), c.end(), 3,
                 std::size_t{2}, equal, twice) == expected);
}

template <typename ExPolicy>
void ranges_search_n_task_test(ExPolicy&& policy)
{
    std::vector<std::size_t> c(10007, 5);
    std::size_t const mid = c.size() / 2;
    c[mid] = 1;
    c[mid + 1] = 1;

    auto const expected = c.begin() + static_cast<std::ptrdiff_t>(mid);

    auto f1 = hpx::ranges::search_n(policy, c, 2, std::size_t{1});
    HPX_TEST(f1.get() == expected);

    auto f2 =
        hpx::ranges::search_n(policy, c.begin(), c.end(), 2, std::size_t{1});
    HPX_TEST(f2.get() == expected);
}

void ranges_search_n_projection_test()
{
    // the projection yields a type other than the element type
    std::vector<item> v{{1, 0}, {2, 0}, {2, 0}, {2, 0}, {3, 0}};

    auto it =
        hpx::ranges::search_n(v, 2, 2, hpx::ranges::equal_to{}, &item::id);
    HPX_TEST(it == v.begin() + 1);

    it = hpx::ranges::search_n(hpx::execution::seq, v.begin(), v.end(), 4, 2,
        hpx::ranges::equal_to{}, &item::id);
    HPX_TEST(it == v.end());
}

void ranges_search_n_test_all()
{
    ranges_search_n_test(hpx::execution::seq);
    ranges_search_n_test(hpx::execution::par);
    ranges_search_n_task_test(hpx::execution::seq(hpx::execution::task));
    ranges_search_n_task_test(hpx::execution::par(hpx::execution::task));

    std::vector<std::size_t> c(100, 5);
    c[10] = 7;
    c[11] = 7;
    HPX_TEST(hpx::ranges::search_n(c, 2, std::size_t{7}) == c.begin() + 10);
    HPX_TEST(hpx::ranges::search_n(c.begin(), c.end(), 2, std::size_t{7}) ==
        c.begin() + 10);
    HPX_TEST(hpx::ranges::search_n(c, 3, std::size_t{7}) == c.end());

    ranges_search_n_projection_test();
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

    ranges_search_n_test_all();

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
