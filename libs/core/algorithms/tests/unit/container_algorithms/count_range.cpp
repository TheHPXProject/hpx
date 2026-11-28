//  Copyright (c) 2018 Christopher Ogle

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
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

struct user_defined_struct
{
    user_defined_struct() = default;
    user_defined_struct(int val)
      : val(val)
    {
    }

    ~user_defined_struct() = default;

    bool operator==(user_defined_struct const& rhs) const
    {
        return val == rhs.val;
    }

    int val;
};

struct random_fill
{
    random_fill(int rand_base, int range)
      : gen(std::rand())
      , base(static_cast<unsigned int>(rand_base))
      , dist(-(range / 2), range / 2)
    {
    }

    int operator()()
    {
        // add a random offset to rand_base modulo 2^32: rand_base +/- the
        // range may not fit into an int, which would make the range of the
        // distribution invalid
        return static_cast<int>(base + static_cast<unsigned int>(dist(gen)));
    }

    std::mt19937 gen;
    unsigned int base = 0;
    std::uniform_int_distribution<> dist;
};

template <typename IteratorTag, typename DataType>
void test_count(IteratorTag, DataType)
{
    std::vector<DataType> c{10007};
    std::generate(std::begin(c), std::end(c), random_fill(0, 20));

    auto const search_val = DataType(10);

    auto result = hpx::ranges::count(c, search_val);
    auto expected = std::count(std::begin(c), std::end(c), search_val);

    HPX_TEST_EQ(expected, result);
}

template <typename ExPolicy, typename IteratorTag, typename DataType>
void test_count(ExPolicy&& policy, IteratorTag, DataType)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<DataType> c{10007};
    std::generate(std::begin(c), std::end(c), random_fill(0, 20));

    auto const search_val = DataType(10);

    auto result = hpx::ranges::count(policy, c, search_val);
    auto expected = std::count(std::begin(c), std::end(c), search_val);

    HPX_TEST_EQ(expected, result);
}

template <typename ExPolicy, typename IteratorTag, typename DataType>
void test_count_async(ExPolicy&& policy, IteratorTag, DataType)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<DataType> c{10007};
    std::generate(std::begin(c), std::end(c), random_fill(0, 20));

    auto const search_val = DataType(10);

    auto f = hpx::ranges::count(policy, c, search_val);
    auto result = f.get();

    auto expected = std::count(std::begin(c), std::end(c), search_val);

    HPX_TEST_EQ(expected, result);
}

template <typename IteratorTag, typename DataType>
void test_count()
{
    using namespace hpx::execution;

    test_count(IteratorTag(), DataType());

    test_count(seq, IteratorTag(), DataType());
    test_count(par, IteratorTag(), DataType());
    test_count(par_unseq, IteratorTag(), DataType());

    test_count_async(seq(task), IteratorTag(), DataType());
    test_count_async(par(task), IteratorTag(), DataType());
}

void count_test()
{
    test_count<std::random_access_iterator_tag, int>();
    test_count<std::random_access_iterator_tag, user_defined_struct>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    count_test();
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
