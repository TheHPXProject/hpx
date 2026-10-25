//  Copyright (c) 2014-2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>

#include <climits>
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "reduce_tests.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce1()
{
    using namespace hpx::execution;

    test_reduce1(IteratorTag());
    test_reduce1(seq, IteratorTag());
    test_reduce1(par, IteratorTag());

    test_reduce1_async(seq(task), IteratorTag());
    test_reduce1_async(par(task), IteratorTag());
}

void reduce_test1()
{
    test_reduce1<std::random_access_iterator_tag>();
    test_reduce1<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce2()
{
    using namespace hpx::execution;

    test_reduce2(IteratorTag());
    test_reduce2(seq, IteratorTag());
    test_reduce2(par, IteratorTag());

    test_reduce2_async(seq(task), IteratorTag());
    test_reduce2_async(par(task), IteratorTag());
}

void reduce_test2()
{
    test_reduce2<std::random_access_iterator_tag>();
    test_reduce2<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce3()
{
    using namespace hpx::execution;

    test_reduce3(IteratorTag());
    test_reduce3(seq, IteratorTag());
    test_reduce3(par, IteratorTag());

    test_reduce3_async(seq(task), IteratorTag());
    test_reduce3_async(par(task), IteratorTag());
}

void reduce_test3()
{
    test_reduce3<std::random_access_iterator_tag>();
    test_reduce3<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_reduce_exception(seq, IteratorTag());
    test_reduce_exception(par, IteratorTag());

    test_reduce_exception_async(seq(task), IteratorTag());
    test_reduce_exception_async(par(task), IteratorTag());
}

void reduce_exception_test()
{
    test_reduce_exception<std::random_access_iterator_tag>();
    test_reduce_exception<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_reduce_bad_alloc(seq, IteratorTag());
    test_reduce_bad_alloc(par, IteratorTag());

    test_reduce_bad_alloc_async(seq(task), IteratorTag());
    test_reduce_bad_alloc_async(par(task), IteratorTag());
}

void reduce_bad_alloc_test()
{
    test_reduce_bad_alloc<std::random_access_iterator_tag>();
    test_reduce_bad_alloc<std::forward_iterator_tag>();
}

struct reduce_minmax_6647
{
    std::pair<int, int> operator()(
        std::pair<int, int> lhs, std::pair<int, int> rhs) const
    {
        return {lhs.first < rhs.first ? lhs.first : rhs.first,
            lhs.second < rhs.second ? rhs.second : lhs.second};
    }
    std::pair<int, int> operator()(std::pair<int, int> lhs, int rhs) const
    {
        return (*this)(lhs, std::pair<int, int>{rhs, rhs});
    }
    std::pair<int, int> operator()(int lhs, std::pair<int, int> rhs) const
    {
        return (*this)(std::pair<int, int>{lhs, lhs}, rhs);
    }
    std::pair<int, int> operator()(int lhs, int rhs) const
    {
        return (*this)(
            std::pair<int, int>{lhs, lhs}, std::pair<int, int>{rhs, rhs});
    }
};

void test_reduce_minmax_not_convertible_to_T()
{
    for (int i = 0; i != 4; ++i)
    {
        std::vector<int> c{3, 1, 4, 1, 5, 9, 2, 6};
        auto const r = hpx::reduce(hpx::execution::par, c.begin(), c.end(),
            std::pair<int, int>{INT_MAX, INT_MIN}, reduce_minmax_6647{});
        HPX_TEST_EQ(r.first, 1);
        HPX_TEST_EQ(r.second, 9);

        std::vector<int> one{7};
        auto const s = hpx::reduce(hpx::execution::par, one.begin(), one.end(),
            std::pair<int, int>{INT_MAX, INT_MIN}, reduce_minmax_6647{});
        HPX_TEST_EQ(s.first, 7);
        HPX_TEST_EQ(s.second, 7);

        std::vector<int> empty;
        auto const e =
            hpx::reduce(hpx::execution::par, empty.begin(), empty.end(),
                std::pair<int, int>{INT_MAX, INT_MIN}, reduce_minmax_6647{});
        HPX_TEST_EQ(e.first, INT_MAX);
        HPX_TEST_EQ(e.second, INT_MIN);
    }
}

void test_reduce_init_wider_than_value_type()
{
    for (int i = 0; i != 4; ++i)
    {
        std::vector<int> c(4, INT_MAX);
        auto const r =
            hpx::reduce(hpx::execution::par, c.begin(), c.end(), 0LL);
        HPX_TEST_EQ(r, 4LL * INT_MAX);

        std::vector<int> big(64, INT_MAX);
        auto const s =
            hpx::reduce(hpx::execution::par, big.begin(), big.end(), 0LL);
        HPX_TEST_EQ(s, 64LL * INT_MAX);
    }
}

struct reduce_non_const_op
{
    int operator()(int lhs, int rhs)
    {
        return lhs + rhs;
    }
};

void test_reduce_non_const_op()
{
    for (int i = 0; i != 4; ++i)
    {
        std::vector<int> c{1, 2, 3, 4};
        auto const r = hpx::reduce(
            hpx::execution::par, c.begin(), c.end(), 0, reduce_non_const_op{});
        HPX_TEST_EQ(r, 10);
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    reduce_test1();
    reduce_test2();
    reduce_test3();
    test_reduce_minmax_not_convertible_to_T();
    test_reduce_init_wider_than_value_type();
    test_reduce_non_const_op();

    reduce_exception_test();
    reduce_bad_alloc_test();
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
