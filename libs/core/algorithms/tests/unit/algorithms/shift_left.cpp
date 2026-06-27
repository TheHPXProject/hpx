//  Copyright (c) 2014 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "test_utils.hpp"

#define ARR_SIZE 100007

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_shift_left_nonbidir(IteratorTag)
{
    std::forward_list<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d;

    for (auto elem : c)
    {
        d.push_back(elem);
    }

    // shift by zero should have no effect
    hpx::shift_left(std::begin(c), std::end(c), 0);
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    // shift by a negative number should have no effect
    hpx::shift_left(std::begin(c), std::end(c), -4);
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % (std::size_t) ARR_SIZE) + 1;
    hpx::shift_left(std::begin(c), std::end(c), n);

    std::move(std::begin(d) + static_cast<std::ptrdiff_t>(n), std::end(d),
        std::begin(d));

    // verify values
    HPX_TEST(std::equal(std::begin(c),
        std::next(std::begin(c), ((std::size_t) ARR_SIZE - n)), std::begin(d)));

    // ensure shift by more than n does not crash
    hpx::shift_left(std::begin(c), std::end(c), (std::size_t) (ARR_SIZE + 1));
}

template <typename IteratorTag>
void test_shift_left(IteratorTag)
{
    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    hpx::shift_left(std::begin(c), std::end(c), 0);
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    // shift by a negative number should have no effect
    hpx::shift_left(std::begin(c), std::end(c), -4);
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % (std::size_t) ARR_SIZE) + 1;
    hpx::shift_left(std::begin(c), std::end(c), n);

    std::move(std::begin(d) + static_cast<std::ptrdiff_t>(n), std::end(d),
        std::begin(d));

    // verify values
    HPX_TEST(std::equal(std::begin(c),
        std::begin(c) + ((std::size_t) ARR_SIZE - n), std::begin(d)));

    // ensure shift by more than n does not crash
    hpx::shift_left(std::begin(c), std::end(c), (std::size_t) (ARR_SIZE + 1));
}

template <typename ExPolicy, typename IteratorTag>
void test_shift_left(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    hpx::shift_left(policy, std::begin(c), std::end(c), 0);
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    // shift by a negative number should have no effect
    hpx::shift_left(policy, std::begin(c), std::end(c), -4);
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % (std::size_t) ARR_SIZE) + 1;
    hpx::shift_left(policy, std::begin(c), std::end(c), n);

    std::move(std::begin(d) + static_cast<std::ptrdiff_t>(n), std::end(d),
        std::begin(d));

    // verify values
    HPX_TEST(std::equal(std::begin(c),
        std::begin(c) + ((std::size_t) ARR_SIZE - n), std::begin(d)));

    // ensure shift by more than n does not crash
    hpx::shift_left(
        policy, std::begin(c), std::end(c), (std::size_t) (ARR_SIZE + 1));
}

template <typename ExPolicy, typename IteratorTag>
void test_shift_left_async(ExPolicy p, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    auto f = hpx::shift_left(p, std::begin(c), std::end(c), 0);
    f.wait();
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    // shift by a negative number should have no effect
    auto f1 = hpx::shift_left(p, std::begin(c), std::end(c), -4);
    f1.wait();
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % (std::size_t) ARR_SIZE) + 1;
    auto f2 = hpx::shift_left(p, std::begin(c), std::end(c), n);
    f2.wait();

    std::move(std::begin(d) + static_cast<std::ptrdiff_t>(n), std::end(d),
        std::begin(d));

    // verify values
    HPX_TEST(std::equal(std::begin(c),
        std::begin(c) + ((std::size_t) ARR_SIZE - n), std::begin(d)));

    // ensure shift by more than n does not crash
    auto f3 = hpx::shift_left(
        p, std::begin(c), std::end(c), (std::size_t) (ARR_SIZE + 1));
    f3.wait();
}

template <typename IteratorTag>
void test_shift_left()
{
    using namespace hpx::execution;
    test_shift_left_nonbidir(IteratorTag());
    test_shift_left(IteratorTag());
    test_shift_left(seq, IteratorTag());
    test_shift_left(par, IteratorTag());
    test_shift_left(par_unseq, IteratorTag());

    test_shift_left_async(seq(task), IteratorTag());
    test_shift_left_async(par(task), IteratorTag());
}

void shift_left_test()
{
    test_shift_left<std::random_access_iterator_tag>();
    test_shift_left<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
// Return-iterator tests for hpx::shift_left
template <typename ExPolicy>
void test_shift_left_return_iterator(ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    // n == 0: no-op, returns last
    {
        std::vector<std::size_t> c(100);
        std::iota(std::begin(c), std::end(c), std::size_t(1));

        auto result = hpx::shift_left(policy, c.begin(), c.end(), 0);
        HPX_TEST(result == c.end());
    }

    // n >= dist: no-op, returns first
    {
        std::vector<std::size_t> c(100);
        std::iota(std::begin(c), std::end(c), std::size_t(1));

        auto r1 = hpx::shift_left(policy, c.begin(), c.end(), 100);
        HPX_TEST(r1 == c.begin());

        auto r2 = hpx::shift_left(policy, c.begin(), c.end(), 9999);
        HPX_TEST(r2 == c.begin());
    }

    // 0 < n < dist: returns first + (dist - n)
    {
        std::vector<std::size_t> c(100);
        std::iota(std::begin(c), std::end(c), std::size_t(1));

        constexpr std::size_t n = 25;
        auto result = hpx::shift_left(policy, c.begin(), c.end(), n);
        auto expected = c.begin() + static_cast<std::ptrdiff_t>(100 - n);
        HPX_TEST(result == expected);

        bool values_ok = true;
        for (std::size_t i = 0; i != 75; ++i)
        {
            if (c[i] != i + 26)
            {
                values_ok = false;
                break;
            }
        }
        HPX_TEST(values_ok);
    }

    // n == dist: returns first
    {
        std::vector<std::size_t> c = {42};
        auto result = hpx::shift_left(policy, c.begin(), c.end(), 1);
        HPX_TEST(result == c.begin());
    }

    // empty range: returns last
    {
        std::vector<std::size_t> empty;
        auto result = hpx::shift_left(policy, empty.begin(), empty.end(), 0);
        HPX_TEST(result == empty.end());
    }
}

template <typename IteratorTag>
void test_shift_left_return_iterator()
{
    using namespace hpx::execution;
    test_shift_left_return_iterator(seq);
    test_shift_left_return_iterator(par);
    test_shift_left_return_iterator(par_unseq);
}

// Cross-policy consistency: seq, par, par_unseq must all return the SAME
// iterator for each case (relative to the range begin).
void test_shift_left_cross_policy()
{
    using namespace hpx::execution;

    auto verify_consistency = [](std::vector<std::size_t> c, int n) {
        std::vector<std::size_t> c_par = c;
        std::vector<std::size_t> c_pu = c;

        auto rs = hpx::shift_left(seq, c.begin(), c.end(), n);
        auto rp = hpx::shift_left(par, c_par.begin(), c_par.end(), n);
        auto ru = hpx::shift_left(par_unseq, c_pu.begin(), c_pu.end(), n);

        auto ds = std::distance(c.begin(), rs);
        auto dp = std::distance(c_par.begin(), rp);
        auto du = std::distance(c_pu.begin(), ru);

        HPX_TEST(ds == dp);
        HPX_TEST(ds == du);
    };

    std::vector<std::size_t> base(50);
    std::iota(base.begin(), base.end(), std::size_t(1));

    verify_consistency(base, 0);
    verify_consistency(base, 50);
    verify_consistency(base, 99);
    verify_consistency(base, 25);
    verify_consistency(base, 1);
    verify_consistency(base, 49);
}

void shift_left_return_iterator_test()
{
    test_shift_left_return_iterator<std::random_access_iterator_tag>();
    test_shift_left_cross_policy();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    shift_left_test();
    shift_left_return_iterator_test();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
