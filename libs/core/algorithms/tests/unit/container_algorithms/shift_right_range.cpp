//  Copyright (c) 2018 Christopher Ogle
//  Copyright (c) 2020-2025 Hartmut Kaiser
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
#include <vector>

#include "test_utils.hpp"

#define ARR_SIZE 100007

unsigned int seed;

template <typename IteratorTag>
void test_shift_right_sent(IteratorTag)
{
    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    hpx::ranges::shift_right(
        std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}, 0);
    HPX_TEST(std::equal(std::begin(c), std::end(c) - 1, std::begin(d)));

    // shift by a negative number should have no effect
    hpx::ranges::shift_right(
        std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}, -4);
    HPX_TEST(std::equal(std::begin(c), std::end(c) - 1, std::begin(d)));

    std::size_t n = (std::rand() % (std::size_t) ARR_SIZE) + 1;
    hpx::ranges::shift_right(
        std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}, n);

    std::move_backward(std::begin(d),
        std::end(d) - static_cast<std::ptrdiff_t>(n + 1), std::end(d) - 1);

    // verify values
    HPX_TEST(std::equal(std::begin(c) + static_cast<std::ptrdiff_t>(n),
        std::end(c) - 1, std::begin(d) + static_cast<std::ptrdiff_t>(n)));

    // ensure shift by more than n does not crash
    hpx::ranges::shift_right(std::begin(c),
        sentinel<std::size_t>{*std::rbegin(c)}, (std::size_t) (ARR_SIZE + 1));
}

template <typename ExPolicy, typename IteratorTag>
void test_shift_right_sent(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    test::subrange_begin(hpx::ranges::shift_right(policy, std::begin(c),
        test::make_sized_sentinel(
            std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}),
        0));
    HPX_TEST(std::equal(std::begin(c), std::end(c) - 1, std::begin(d)));

    // shift by a negative number should have no effect
    test::subrange_begin(hpx::ranges::shift_right(policy, std::begin(c),
        test::make_sized_sentinel(
            std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}),
        -4));
    HPX_TEST(std::equal(std::begin(c), std::end(c) - 1, std::begin(d)));

    std::size_t n = (std::rand() % (std::size_t) ARR_SIZE) + 1;
    test::subrange_begin(hpx::ranges::shift_right(policy, std::begin(c),
        test::make_sized_sentinel(
            std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}),
        n));

    std::move_backward(std::begin(d),
        std::end(d) - static_cast<std::ptrdiff_t>(n + 1), std::end(d) - 1);

    // verify values
    HPX_TEST(std::equal(std::begin(c) + static_cast<std::ptrdiff_t>(n),
        std::end(c) - 1, std::begin(d) + n));

    // ensure shift by more than n does not crash
    test::subrange_begin(hpx::ranges::shift_right(policy, std::begin(c),
        test::make_sized_sentinel(
            std::begin(c), sentinel<std::size_t>{*std::rbegin(c)}),
        (std::size_t) (ARR_SIZE + 1)));
}

template <typename IteratorTag>
void test_shift_right(IteratorTag)
{
    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    hpx::ranges::shift_right(c, 0);
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    // shift by a negative number should have no effect
    hpx::ranges::shift_right(c, -4);
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % (std::size_t) ARR_SIZE) + 1;
    hpx::ranges::shift_right(c, n);

    std::move_backward(std::begin(d),
        std::end(d) - static_cast<std::ptrdiff_t>(n), std::end(d));

    // verify values
    HPX_TEST(std::equal(std::begin(c) + static_cast<std::ptrdiff_t>(n),
        std::end(c), std::begin(d) + static_cast<std::ptrdiff_t>(n)));

    // ensure shift by more than n does not crash
    hpx::ranges::shift_right(c, (std::size_t) (ARR_SIZE + 1));
}

template <typename ExPolicy, typename IteratorTag>
void test_shift_right(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    test::subrange_begin(hpx::ranges::shift_right(policy, c, 0));
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    // shift by a negative number should have no effect
    test::subrange_begin(hpx::ranges::shift_right(policy, c, -4));
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % (std::size_t) ARR_SIZE) + 1;
    test::subrange_begin(hpx::ranges::shift_right(policy, c, n));

    std::move_backward(std::begin(d),
        std::end(d) - static_cast<std::ptrdiff_t>(n), std::end(d));

    // verify values
    HPX_TEST(std::equal(std::begin(c) + static_cast<std::ptrdiff_t>(n),
        std::end(c), std::begin(d) + static_cast<std::ptrdiff_t>(n)));

    // ensure shift by more than n does not crash
    test::subrange_begin(
        hpx::ranges::shift_right(policy, c, (std::size_t) (ARR_SIZE + 1)));
}

template <typename ExPolicy, typename IteratorTag>
void test_shift_right_async(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(ARR_SIZE);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    // shift by zero should have no effect
    auto fut1 = test::subrange_begin(hpx::ranges::shift_right(policy, c, 0));
    fut1.wait();
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    // shift by a negative number should have no effect
    auto fut2 = test::subrange_begin(hpx::ranges::shift_right(policy, c, -4));
    fut2.wait();
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));

    std::size_t n = (std::rand() % (std::size_t) ARR_SIZE) + 1;
    auto fut3 = test::subrange_begin(hpx::ranges::shift_right(policy, c, n));
    fut3.wait();

    std::move_backward(std::begin(d),
        std::end(d) - static_cast<std::ptrdiff_t>(n), std::end(d));

    // verify values
    HPX_TEST(std::equal(std::begin(c) + static_cast<std::ptrdiff_t>(n),
        std::end(c), std::begin(d) + static_cast<std::ptrdiff_t>(n)));

    // ensure shift by more than n does not crash
    auto fut4 = test::subrange_begin(
        hpx::ranges::shift_right(policy, c, (std::size_t) (ARR_SIZE + 1)));
    fut4.wait();
}

template <typename IteratorTag>
void test_shift_right()
{
    using namespace hpx::execution;

    test_shift_right(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_shift_right(seq, IteratorTag());
        test_shift_right(par, IteratorTag());
        test_shift_right(par_unseq, IteratorTag());

        test_shift_right_async(seq(task), IteratorTag());
        test_shift_right_async(par(task), IteratorTag());
    }

    test_shift_right_sent(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_shift_right_sent(seq, IteratorTag());
        test_shift_right_sent(par, IteratorTag());
        test_shift_right_sent(par_unseq, IteratorTag());
    }
}

void shift_right_test()
{
    test_shift_right<std::random_access_iterator_tag>();
    test_shift_right<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed1 = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed1 = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed1 << std::endl;
    std::srand(seed1);

    seed = seed1;

    shift_right_test();
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
