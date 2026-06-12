//  Copyright (c) 2017-2018 Taeguk Kwon
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
#include <cstdint>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
struct user_defined_type
{
    user_defined_type() = default;
    user_defined_type(int rand_no)
      : val(rand_no)
      , name(name_list[std::rand() % name_list.size()])
    {
    }

    bool operator==(int rand_no) const
    {
        return this->val == rand_no;
    }

    bool operator==(user_defined_type const& t) const
    {
        return this->name == t.name && this->val == t.val;
    }

    static std::vector<std::string> const name_list;

    int val;
    std::string name;
};

std::vector<std::string> const user_defined_type::name_list{
    "ABB", "ABC", "ACB", "BASE", "CAA", "CAAA", "CAAB"};

struct random_fill
{
    random_fill(int rand_base, int range)
      : gen(std::rand())
      , dist(rand_base - range / 2, rand_base + range / 2)
    {
    }

    int operator()()
    {
        return dist(gen);
    }

    std::mt19937 gen;
    std::uniform_int_distribution<> dist;
};

////////////////////////////////////////////////////////////////////////////
// test case for iterator - sentinel_value
void test_remove_if_sent()
{
    using hpx::get;

    std::size_t const size = 100;
    std::vector<std::int16_t> c(size);
    std::iota(std::begin(c), std::end(c), 1);

    auto pred = [](std::int16_t const& a) -> bool { return a % 42 == 0; };

    auto pre_result = std::count_if(std::begin(c), std::end(c), pred);
    hpx::ranges::remove_if(std::begin(c), sentinel<std::int16_t>{50}, pred);
    auto post_result = std::count_if(std::begin(c), std::end(c), pred);

    HPX_TEST(pre_result == 2 && post_result == 1);
}

template <typename ExPolicy>
void test_remove_if_sent(ExPolicy policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using hpx::get;

    std::size_t const size = 100;
    std::vector<std::int16_t> c(size);
    std::iota(std::begin(c), std::end(c), 1);

    auto pred = [](std::int16_t const& a) -> bool { return a % 42 == 0; };

    auto pre_result = std::count_if(std::begin(c), std::end(c), pred);
    hpx::ranges::remove_if(
        policy, std::begin(c), sentinel<std::int16_t>{50}, pred);
    auto post_result = std::count_if(std::begin(c), std::end(c), pred);

    HPX_TEST(pre_result == 2 && post_result == 1);
}

template <typename DataType>
void test_remove_if(DataType)
{
    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(0, 6));
    d = c;

    auto pred = [](DataType const& a) -> bool { return a == 0; };

    auto result = hpx::ranges::remove_if(c, pred);
    auto solution = std::remove_if(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), result.begin(), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename DataType>
void test_remove_if(ExPolicy policy, DataType)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(0, 6));
    d = c;

    auto pred = [](DataType const& a) -> bool { return a == 0; };

    auto result = hpx::ranges::remove_if(policy, c, pred);
    auto solution = std::remove_if(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), result.begin(), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename DataType>
void test_remove_if_async(ExPolicy policy, DataType)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using hpx::get;

    std::size_t const size = 10007;
    std::vector<DataType> c(size), d;
    std::generate(std::begin(c), std::end(c), random_fill(0, 6));
    d = c;

    auto pred = [](DataType const& a) -> bool { return a == 0; };

    auto f = hpx::ranges::remove_if(policy, c, pred);
    auto result = f.get();
    auto solution = std::remove_if(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), std::begin(result), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename DataType>
void test_remove_if_sentinel(DataType)
{
    using hpx::get;

    using test_vector = test::test_sentinel_container<std::vector<DataType>,
        std::forward_iterator_tag>;

    std::size_t const size = 10007;
    test_vector c(size);
    std::vector<DataType> d;
    std::generate(std::begin(c.base()), std::end(c.base()), random_fill(0, 6));
    d = c.base();

    auto pred = [](DataType const& a) -> bool { return a == 0; };

    auto result = hpx::ranges::remove_if(c, pred);
    auto solution = std::remove_if(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), result.begin(), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename ExPolicy, typename DataType>
void test_remove_if_sentinel(ExPolicy policy, DataType)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using hpx::get;

    using test_vector = test::test_sentinel_container<std::vector<DataType>,
        std::forward_iterator_tag>;

    std::size_t const size = 10007;
    test_vector c(size);
    std::vector<DataType> d;
    std::generate(std::begin(c.base()), std::end(c.base()), random_fill(0, 6));
    d = c.base();

    auto pred = [](DataType const& a) -> bool { return a == 0; };

    auto result = hpx::ranges::remove_if(policy, c, pred);
    auto solution = std::remove_if(std::begin(d), std::end(d), pred);

    bool equality =
        test::equal(std::begin(c), result.begin(), std::begin(d), solution);

    HPX_TEST(equality);
}

template <typename DataType>
void test_remove_if()
{
    using namespace hpx::execution;

    test_remove_if(DataType());
    test_remove_if(seq, DataType());
    test_remove_if(par, DataType());
    test_remove_if(par_unseq, DataType());

    test_remove_if_sentinel(DataType());
    test_remove_if_sentinel(seq, DataType());
    test_remove_if_sentinel(par, DataType());
    test_remove_if_sentinel(par_unseq, DataType());

    test_remove_if_async(seq(task), DataType());
    test_remove_if_async(par(task), DataType());

    test_remove_if_sent();
    test_remove_if_sent(par);
    test_remove_if_sent(par_unseq);
}

void test_remove_if()
{
    test_remove_if<int>();
    test_remove_if<user_defined_type>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    test_remove_if();
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
