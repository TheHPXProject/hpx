//  Copyright (c) 2016 Hartmut Kaiser
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
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(1, 10006);

template <typename ExPolicy, typename IteratorTag>
void test_for_loop_n_strided(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::for_each(std::begin(c), std::end(c), [](std::size_t& v) -> void {
        if (v == 42)
            v = 43;
    });

    int stride = dis(gen);    //-V103

    // the loop body is invoked exactly n times, pick the number of iterations
    // that keeps the last one inside the sequence
    std::size_t const step = static_cast<std::size_t>(stride);
    std::size_t const n = (c.size() + step - 1) / step;

    hpx::experimental::for_loop_n_strided(std::forward<ExPolicy>(policy),
        iterator(std::begin(c)), n, stride, [](iterator it) { *it = 42; });

    // verify values
    std::size_t count = 0;
    for (std::size_t i = 0; i != c.size(); ++i)
    {
        if (i % step == 0)
        {
            HPX_TEST_EQ(c[i], std::size_t(42));
            ++count;
        }
        else
        {
            HPX_TEST_NEQ(c[i], std::size_t(42));
        }
    }
    HPX_TEST_EQ(count, n);
}

template <typename ExPolicy, typename IteratorTag>
void test_for_loop_n_strided_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::for_each(std::begin(c), std::end(c), [](std::size_t& v) -> void {
        if (v == 42)
            v = 43;
    });

    int stride = dis(gen);    //-V103

    std::size_t const step = static_cast<std::size_t>(stride);
    std::size_t const n = (c.size() + step - 1) / step;

    auto f = hpx::experimental::for_loop_n_strided(std::forward<ExPolicy>(p),
        iterator(std::begin(c)), n, stride, [](iterator it) { *it = 42; });
    f.wait();

    // verify values
    std::size_t count = 0;
    for (std::size_t i = 0; i != c.size(); ++i)
    {
        if (i % step == 0)
        {
            HPX_TEST_EQ(c[i], std::size_t(42));
            ++count;
        }
        else
        {
            HPX_TEST_NEQ(c[i], std::size_t(42));
        }
    }
    HPX_TEST_EQ(count, n);
}

template <typename IteratorTag>
void test_for_loop_n_strided()
{
    using namespace hpx::execution;

    test_for_loop_n_strided(seq, IteratorTag());
    test_for_loop_n_strided(par, IteratorTag());
    test_for_loop_n_strided(par_unseq, IteratorTag());

    test_for_loop_n_strided_async(seq(task), IteratorTag());
    test_for_loop_n_strided_async(par(task), IteratorTag());
}

void for_loop_n_strided_test()
{
    test_for_loop_n_strided<std::random_access_iterator_tag>();
    test_for_loop_n_strided<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_for_loop_n_strided_idx(ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::for_each(std::begin(c), std::end(c), [](std::size_t& v) -> void {
        if (v == 42)
            v = 43;
    });

    int stride = dis(gen);    //-V103

    std::size_t const step = static_cast<std::size_t>(stride);
    std::size_t const n = (c.size() + step - 1) / step;

    hpx::experimental::for_loop_n_strided(std::forward<ExPolicy>(policy), 0, n,
        stride, [&c](std::size_t i) { c[i] = 42; });

    // verify values
    std::size_t count = 0;
    for (std::size_t i = 0; i != c.size(); ++i)
    {
        if (i % step == 0)
        {
            HPX_TEST_EQ(c[i], std::size_t(42));
            ++count;
        }
        else
        {
            HPX_TEST_NEQ(c[i], std::size_t(42));
        }
    }
    HPX_TEST_EQ(count, n);
}

template <typename ExPolicy>
void test_for_loop_n_strided_idx_async(ExPolicy&& p)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::for_each(std::begin(c), std::end(c), [](std::size_t& v) -> void {
        if (v == 42)
            v = 43;
    });

    int stride = dis(gen);    //-V103

    std::size_t const step = static_cast<std::size_t>(stride);
    std::size_t const n = (c.size() + step - 1) / step;

    auto f = hpx::experimental::for_loop_n_strided(std::forward<ExPolicy>(p), 0,
        n, stride, [&c](std::size_t i) { c[i] = 42; });
    f.wait();

    // verify values
    std::size_t count = 0;
    for (std::size_t i = 0; i != c.size(); ++i)
    {
        if (i % step == 0)
        {
            HPX_TEST_EQ(c[i], std::size_t(42));
            ++count;
        }
        else
        {
            HPX_TEST_NEQ(c[i], std::size_t(42));
        }
    }
    HPX_TEST_EQ(count, n);
}

void for_loop_n_strided_test_idx()
{
    using namespace hpx::execution;

    test_for_loop_n_strided_idx(seq);
    test_for_loop_n_strided_idx(par);
    test_for_loop_n_strided_idx(par_unseq);

    test_for_loop_n_strided_idx_async(seq(task));
    test_for_loop_n_strided_idx_async(par(task));
}

///////////////////////////////////////////////////////////////////////////////
// the second argument is the number of iterations to perform, not the distance
// those iterations cover (see N4755, 7.2.4, paragraph 2.1)
template <typename ExPolicy>
void test_for_loop_n_strided_count(ExPolicy&& policy)
{
    std::vector<std::size_t> c(100, 0);

    hpx::experimental::for_loop_n_strided(std::forward<ExPolicy>(policy), 0,
        std::size_t(10), 3, [&c](std::size_t i) { ++c[i]; });

    // the body should have run for 0, 3, 6, ... 27
    std::size_t count = 0;
    for (std::size_t i = 0; i != c.size(); ++i)
    {
        if (i < 30 && i % 3 == 0)
        {
            HPX_TEST_EQ(c[i], std::size_t(1));
            ++count;
        }
        else
        {
            HPX_TEST_EQ(c[i], std::size_t(0));
        }
    }
    HPX_TEST_EQ(count, std::size_t(10));
}

void for_loop_n_strided_test_count()
{
    using namespace hpx::execution;

    test_for_loop_n_strided_count(seq);
    test_for_loop_n_strided_count(par);
    test_for_loop_n_strided_count(par_unseq);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    for_loop_n_strided_test();
    for_loop_n_strided_test_idx();
    for_loop_n_strided_test_count();

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
