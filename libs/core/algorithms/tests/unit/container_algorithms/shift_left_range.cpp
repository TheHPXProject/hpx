//  Copyright (c) 2018 Christopher Ogle
//  Copyright (c) 2020 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/iterator_support/tests/iter_sent.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "test_utils.hpp"

constexpr std::size_t ARR_SIZE = 100007;

unsigned int seed;

template <typename IteratorTag>
void test_shift_left_sent(IteratorTag)
{
    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    hpx::ranges::shift_left(
        std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}, 0);
    HPX_TEST(std::equal(std::begin(c), std::end(c) - 1, std::begin(d)));

    // shift by a negative number should have no effect
    hpx::ranges::shift_left(
        std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}, -4);
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % (ARR_SIZE - 1)) + 1;
    hpx::ranges::shift_left(
        std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}, n);

    std::move(std::begin(d) + static_cast<std::ptrdiff_t>(n), std::end(d) - 1,
        std::begin(d));

    // verify values
    HPX_TEST(std::equal(
        std::begin(c), std::begin(c) + (ARR_SIZE - n - 1), std::begin(d)));

    // ensure shift by more than n does not crash
    hpx::ranges::shift_left(
        std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}, ARR_SIZE + 1);
}

template <typename ExPolicy, typename IteratorTag>
void test_shift_left_sent(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    test::subrange_end(hpx::ranges::shift_left(policy, std::begin(c),
        test::make_sized_sentinel(
            std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}),
        0));
    HPX_TEST(std::equal(std::begin(c), std::end(c) - 1, std::begin(d)));

    // shift by a negative number should have no effect
    test::subrange_end(hpx::ranges::shift_left(policy, std::begin(c),
        test::make_sized_sentinel(
            std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}),
        -4));
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % (ARR_SIZE - 1)) + 1;
    test::subrange_end(hpx::ranges::shift_left(policy, std::begin(c),
        test::make_sized_sentinel(
            std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}),
        n));

    std::move(std::begin(d) + static_cast<std::ptrdiff_t>(n), std::end(d) - 1,
        std::begin(d));

    // verify values
    HPX_TEST(std::equal(
        std::begin(c), std::begin(c) + (ARR_SIZE - n - 1), std::begin(d)));

    // ensure shift by more than n does not crash
    test::subrange_end(hpx::ranges::shift_left(policy, std::begin(c),
        test::make_sized_sentinel(
            std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}),
        ARR_SIZE + 1));
}

template <typename IteratorTag>
void test_shift_left(IteratorTag)
{
    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    hpx::ranges::shift_left(c, 0);
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    // shift by a negative number should have no effect
    hpx::ranges::shift_left(c, -4);
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % ARR_SIZE) + 1;
    hpx::ranges::shift_left(c, n);

    std::move(std::begin(d) + static_cast<std::ptrdiff_t>(n), std::end(d),
        std::begin(d));

    // verify values
    HPX_TEST(std::equal(
        std::begin(c), std::begin(c) + (ARR_SIZE - n), std::begin(d)));

    // ensure shift by more than n does not crash
    hpx::ranges::shift_left(c, ARR_SIZE + 1);
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
    test::subrange_end(hpx::ranges::shift_left(policy, c, 0));
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    // shift by a negative number should have no effect
    test::subrange_end(hpx::ranges::shift_left(policy, c, -4));
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % ARR_SIZE) + 1;
    test::subrange_end(hpx::ranges::shift_left(policy, c, n));

    std::move(std::begin(d) + static_cast<std::ptrdiff_t>(n), std::end(d),
        std::begin(d));

    // verify values
    HPX_TEST(std::equal(
        std::begin(c), std::begin(c) + (ARR_SIZE - n), std::begin(d)));

    // ensure shift by more than n does not crash
    test::subrange_end(hpx::ranges::shift_left(policy, c, ARR_SIZE + 1));
}

template <typename ExPolicy, typename IteratorTag>
void test_shift_left_async(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    auto fut1 = test::subrange_end(hpx::ranges::shift_left(policy, c, 0));
    fut1.wait();
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    // shift by a negative number should have no effect
    auto fut2 = test::subrange_end(hpx::ranges::shift_left(policy, c, -4));
    fut2.wait();
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % ARR_SIZE) + 1;
    auto fut3 = test::subrange_end(hpx::ranges::shift_left(policy, c, n));
    fut3.wait();

    std::move(std::begin(d) + static_cast<std::ptrdiff_t>(n), std::end(d),
        std::begin(d));

    // verify values
    HPX_TEST(std::equal(
        std::begin(c), std::begin(c) + (ARR_SIZE - n), std::begin(d)));

    // ensure shift by more than n does not crash
    auto fut4 =
        test::subrange_end(hpx::ranges::shift_left(policy, c, ARR_SIZE + 1));
    fut4.wait();
}

template <typename IteratorTag>
void test_shift_left()
{
    using namespace hpx::execution;

    test_shift_left(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_shift_left(seq, IteratorTag());
        test_shift_left(par, IteratorTag());
        test_shift_left(par_unseq, IteratorTag());

        test_shift_left_async(seq(task), IteratorTag());
        test_shift_left_async(par(task), IteratorTag());
    }

    test_shift_left_sent(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_shift_left_sent(seq, IteratorTag());
        test_shift_left_sent(par, IteratorTag());
        test_shift_left_sent(par_unseq, IteratorTag());
    }
}

void shift_left_test()
{
    test_shift_left<std::random_access_iterator_tag>();
    test_shift_left<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed1 = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed1 = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed1 << std::endl;
    std::srand(seed1);

    seed = seed1;

    shift_left_test();
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
