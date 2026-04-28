//  Copyright (c) 2018 Christopher Ogle
//  Copyright (c) 2020 Hartmut Kaiser
//  Copyright (c) 2020 Francisco Jose Tapia (fjtapia@gmail.com )
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
#include <unordered_set>
#include <vector>

#include "test_utils.hpp"

struct S
{
    std::size_t val;
};

////////////////////////////////////////////////////////////////////////////
#define SIZE 10007

unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

template <typename IteratorTag>
void test_nth_element_sent(IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using sentinel = test::sentinel_from_iterator<iterator>;

    std::vector<std::size_t> c(SIZE);
    std::iota(c.begin(), c.end(), 1);
    std::shuffle(c.begin(), c.end(), gen);
    std::vector<std::size_t> d = c;

    auto rand_index = std::rand() % SIZE;

    auto result = hpx::ranges::nth_element(iterator(std::begin(c)),
        iterator(std::begin(c) + rand_index),
        sentinel(iterator(std::end(c) - 1)));

    HPX_TEST(result == iterator(std::end(c) - 1));

    std::nth_element(
        std::begin(d), std::begin(d) + rand_index, std::end(d) - 1);

    HPX_TEST(*(std::begin(c) + rand_index) == *(std::begin(d) + rand_index));

    for (int k = 0; k < rand_index; k++)
    {
        HPX_TEST(c[k] <= c[rand_index]);
    }

    for (int k = rand_index + 1; k < SIZE - 1; k++)
    {
        HPX_TEST(c[k] >= c[rand_index]);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_nth_element_sent(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using sentinel = test::sentinel_from_iterator<iterator>;

    std::vector<std::size_t> c(SIZE);
    std::iota(c.begin(), c.end(), 1);
    std::shuffle(c.begin(), c.end(), gen);
    std::vector<std::size_t> d = c;

    auto rand_index = std::rand() % SIZE;

    auto result = hpx::ranges::nth_element(policy, iterator(std::begin(c)),
        iterator(std::begin(c) + rand_index),
        sentinel(iterator(std::end(c) - 1)));

    HPX_TEST(result == iterator(std::end(c) - 1));

    std::nth_element(
        std::begin(d), std::begin(d) + rand_index, std::end(d) - 1);

    HPX_TEST(*(std::begin(c) + rand_index) == *(std::begin(d) + rand_index));

    for (int k = 0; k < rand_index; k++)
    {
        HPX_TEST(c[k] <= c[rand_index]);
    }

    for (int k = rand_index + 1; k < SIZE - 1; k++)
    {
        HPX_TEST(c[k] >= c[rand_index]);
    }
}

template <typename IteratorTag>
void test_nth_element(IteratorTag)
{
    std::vector<std::size_t> c(SIZE);
    std::generate(
        std::begin(c), std::end(c), []() { return std::rand() % SIZE; });
    std::vector<std::size_t> d = c;

    auto rand_index = std::rand() % SIZE;

    auto result = hpx::ranges::nth_element(c, std::begin(c) + rand_index);
    HPX_TEST(result == std::end(c));

    std::nth_element(std::begin(d), std::begin(d) + rand_index, std::end(d));

    HPX_TEST(*(std::begin(c) + rand_index) == *(std::begin(d) + rand_index));

    for (int k = 0; k < rand_index; k++)
    {
        HPX_TEST(c[k] <= c[rand_index]);
    }

    for (int k = rand_index + 1; k < SIZE; k++)
    {
        HPX_TEST(c[k] >= c[rand_index]);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_nth_element(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(SIZE);
    std::generate(
        std::begin(c), std::end(c), []() { return std::rand() % SIZE; });
    std::vector<std::size_t> d = c;

    auto rand_index = std::rand() % SIZE;

    auto result =
        hpx::ranges::nth_element(policy, c, std::begin(c) + rand_index);
    HPX_TEST(result == std::end(c));

    std::nth_element(std::begin(d), std::begin(d) + rand_index, std::end(d));

    HPX_TEST(*(std::begin(c) + rand_index) == *(std::begin(d) + rand_index));

    for (int k = 0; k < rand_index; k++)
    {
        HPX_TEST(c[k] <= c[rand_index]);
    }

    for (int k = rand_index + 1; k < SIZE; k++)
    {
        HPX_TEST(c[k] >= c[rand_index]);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_nth_element_async(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(SIZE);
    std::generate(
        std::begin(c), std::end(c), []() { return std::rand() % SIZE; });
    std::vector<std::size_t> d = c;

    auto rand_index = std::rand() % SIZE;

    auto result =
        hpx::ranges::nth_element(policy, c, std::begin(c) + rand_index);
    result.wait();

    HPX_TEST(result.get() == std::end(c));

    std::nth_element(std::begin(d), std::begin(d) + rand_index, std::end(d));

    HPX_TEST(*(std::begin(c) + rand_index) == *(std::begin(d) + rand_index));

    for (int k = 0; k < rand_index; k++)
    {
        HPX_TEST(c[k] <= c[rand_index]);
    }

    for (int k = rand_index + 1; k < SIZE; k++)
    {
        HPX_TEST(c[k] >= c[rand_index]);
    }
}

template <typename ExPolicy>
void test_nth_element_projection(ExPolicy policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<S> c(SIZE);
    for (std::size_t i = 0; i < SIZE; ++i)
        c[i].val = SIZE - i;

    auto rand_index = std::rand() % SIZE;
    auto nth = std::begin(c) + rand_index;

    hpx::ranges::nth_element(policy, c, nth, std::less<std::size_t>{}, &S::val);

    for (int k = 0; k < rand_index; k++)
    {
        HPX_TEST(c[k].val <= c[rand_index].val);
    }

    for (int k = rand_index + 1; k < SIZE; k++)
    {
        HPX_TEST(c[k].val >= c[rand_index].val);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_nth_element_sent_projection(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<S>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using sentinel = test::sentinel_from_iterator<iterator>;

    std::vector<S> c(SIZE);
    for (std::size_t i = 0; i < SIZE; ++i)
        c[i].val = SIZE - i;

    auto rand_index = std::rand() % SIZE;

    auto result = hpx::ranges::nth_element(policy, iterator(std::begin(c)),
        iterator(std::begin(c) + rand_index),
        sentinel(iterator(std::end(c) - 1)), std::less<std::size_t>{}, &S::val);

    HPX_TEST(result == iterator(std::end(c) - 1));

    for (int k = 0; k < rand_index; k++)
    {
        HPX_TEST(c[k].val <= c[rand_index].val);
    }

    for (int k = rand_index + 1; k < SIZE - 1; k++)
    {
        HPX_TEST(c[k].val >= c[rand_index].val);
    }
}

template <typename IteratorTag>
void test_nth_element()
{
    using namespace hpx::execution;

    test_nth_element(IteratorTag());
    test_nth_element(seq, IteratorTag());
    test_nth_element(par, IteratorTag());
    test_nth_element(par_unseq, IteratorTag());

    test_nth_element_async(seq(task), IteratorTag());
    test_nth_element_async(par(task), IteratorTag());

    test_nth_element_sent(IteratorTag());
    test_nth_element_sent(seq, IteratorTag());
    test_nth_element_sent(par, IteratorTag());
    test_nth_element_sent(par_unseq, IteratorTag());

    test_nth_element_projection(seq);
    test_nth_element_projection(par);
    test_nth_element_projection(par_unseq);

    test_nth_element_sent_projection(seq, IteratorTag());
    test_nth_element_sent_projection(par, IteratorTag());
    test_nth_element_sent_projection(par_unseq, IteratorTag());
}

void nth_element_test()
{
    test_nth_element<std::random_access_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);
    gen.seed(seed);

    nth_element_test();
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
