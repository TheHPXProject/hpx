//  Copyright (c) 2017-2018 Taeguk Kwon
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
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
struct less_than
{
    explicit less_than(int partition_at)
      : partition_at_(partition_at)
    {
    }

    template <typename T>
    bool operator()(T const& val)
    {
        return val < partition_at_;
    }

    int partition_at_;
};

struct great_equal_than
{
    explicit great_equal_than(int partition_at)
      : partition_at_(partition_at)
    {
    }

    template <typename T>
    bool operator()(T const& val)
    {
        return val >= partition_at_;
    }

    int partition_at_;
};

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_stable_partition_sent(IteratorTag)
{
    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::begin(d));

    int partition_at = std::rand();

    auto result = hpx::ranges::stable_partition(std::begin(c),
        sentinel<int>{*std::next(
            std::begin(c), static_cast<std::ptrdiff_t>(c.size() - 1))},
        less_than(partition_at));

    auto partition_pt = std::find_if(
        std::begin(c), std::end(c) - 1, great_equal_than(partition_at));
    HPX_TEST(result.begin() == partition_pt);

    // verify values
    std::stable_partition(
        std::begin(d), std::end(d) - 1, less_than(partition_at));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c), std::end(c) - 1, std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size() - 1);
}

template <typename ExPolicy, typename IteratorTag>
void test_stable_partition_sent(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::begin(d));

    int partition_at = std::rand();

    auto result = hpx::ranges::stable_partition(policy, std::begin(c),
        sentinel<int>{*std::next(
            std::begin(c), static_cast<std::ptrdiff_t>(c.size() - 1))},
        less_than(partition_at));

    auto partition_pt = std::find_if(
        std::begin(c), std::end(c) - 1, great_equal_than(partition_at));
    HPX_TEST(result.begin() == partition_pt);

    // verify values
    std::stable_partition(
        std::begin(d), std::end(d) - 1, less_than(partition_at));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c), std::end(c) - 1, std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size() - 1);
}

template <typename IteratorTag>
void test_stable_partition(IteratorTag)
{
    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::begin(d));

    int partition_at = std::rand();

    auto result = hpx::ranges::stable_partition(c, less_than(partition_at));

    auto partition_pt = std::find_if(
        std::begin(c), std::end(c), great_equal_than(partition_at));
    HPX_TEST(result.begin() == partition_pt);

    // verify values
    std::stable_partition(std::begin(d), std::end(d), less_than(partition_at));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_stable_partition(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::begin(d));

    int partition_at = std::rand();

    auto result =
        hpx::ranges::stable_partition(policy, c, less_than(partition_at));

    auto partition_pt = std::find_if(
        std::begin(c), std::end(c), great_equal_than(partition_at));
    HPX_TEST(result.begin() == partition_pt);

    // verify values
    std::stable_partition(std::begin(d), std::end(d), less_than(partition_at));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_stable_partition_async(ExPolicy p, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::begin(d));

    int partition_at = std::rand();

    auto f = hpx::ranges::stable_partition(p, c, less_than(partition_at));

    auto result = f.get();
    auto partition_pt = std::find_if(
        std::begin(c), std::end(c), great_equal_than(partition_at));
    HPX_TEST(result.begin() == partition_pt);

    // verify values
    std::stable_partition(std::begin(d), std::end(d), less_than(partition_at));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size());
}

template <typename IteratorTag>
void test_stable_partition_sentinel(IteratorTag)
{
    using test_vector = test::test_sentinel_container<std::vector<int>,
        std::bidirectional_iterator_tag>;

    test_vector c(10007);
    std::vector<int> d(c.size());
    std::iota(std::begin(c.base()), std::end(c.base()), std::rand());
    std::copy(std::begin(c.base()), std::end(c.base()), std::begin(d));

    int partition_at = std::rand();

    auto result = hpx::ranges::stable_partition(c, less_than(partition_at));

    auto partition_pt = std::find_if(std::begin(c.base()), std::end(c.base()),
        great_equal_than(partition_at));
    HPX_TEST(result.begin().base() == partition_pt);

    // verify values
    std::stable_partition(std::begin(d), std::end(d), less_than(partition_at));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), std::end(c.base()), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_stable_partition_sentinel(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using test_vector = test::test_sentinel_container<std::vector<int>,
        std::bidirectional_iterator_tag>;

    test_vector c(10007);
    std::vector<int> d(c.size());
    std::iota(std::begin(c.base()), std::end(c.base()), std::rand());
    std::copy(std::begin(c.base()), std::end(c.base()), std::begin(d));

    int partition_at = std::rand();

    auto result =
        hpx::ranges::stable_partition(policy, c, less_than(partition_at));

    auto partition_pt = std::find_if(std::begin(c.base()), std::end(c.base()),
        great_equal_than(partition_at));
    HPX_TEST(result.begin().base() == partition_pt);

    // verify values
    std::stable_partition(std::begin(d), std::end(d), less_than(partition_at));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), std::end(c.base()), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size());
}

template <typename DataType>
void test_stable_partition()
{
    using namespace hpx::execution;

    test_stable_partition(DataType());
    test_stable_partition(seq, DataType());
    test_stable_partition(par, DataType());
    test_stable_partition(par_unseq, DataType());

    test_stable_partition_sentinel(DataType());
    test_stable_partition_sentinel(seq, DataType());
    test_stable_partition_sentinel(par, DataType());
    test_stable_partition_sentinel(par_unseq, DataType());

    test_stable_partition_async(seq(task), DataType());
    test_stable_partition_async(par(task), DataType());

    test_stable_partition_sent(DataType());
    test_stable_partition_sent(seq, DataType());
    test_stable_partition_sent(par, DataType());
    test_stable_partition_sent(par_unseq, DataType());
}

void test_stable_partition()
{
    test_stable_partition<int>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    test_stable_partition();
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
