//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/memory.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
struct default_constructable
{
    default_constructable()
      : value_(42)
    {
    }

    explicit default_constructable(std::int32_t val)
    {
        value_ = val;
    }

    std::int32_t value_;
};

std::size_t const data_size = 10007;

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_uninitialized_default_construct_n(IteratorTag)
{
    using base_iterator = std::vector<default_constructable>;

    base_iterator c(data_size, default_constructable(10));
    auto end_size = rand() % data_size;
    hpx::ranges::uninitialized_default_construct_n(std::begin(c), end_size);

    std::size_t count42 = 0;
    std::size_t count10 = 0;
    std::for_each(std::begin(c), std::begin(c) + data_size,
        [&count42, &count10](default_constructable v1) {
            if (v1.value_ == 42)
            {
                count42++;
            }
            else if (v1.value_ == 10)
            {
                count10++;
            }
        });

    HPX_TEST_EQ(count42, end_size);
    HPX_TEST_EQ(count10, data_size - end_size);
}

template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_n(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<default_constructable>;

    base_iterator c(data_size, default_constructable(10));
    auto end_size = rand() % data_size;
    hpx::ranges::uninitialized_default_construct_n(
        policy, std::begin(c), end_size);

    std::size_t count42 = 0;
    std::size_t count10 = 0;
    std::for_each(std::begin(c), std::begin(c) + data_size,
        [&count42, &count10](default_constructable v1) {
            if (v1.value_ == 42)
            {
                count42++;
            }
            else if (v1.value_ == 10)
            {
                count10++;
            }
        });

    HPX_TEST_EQ(count42, end_size);
    HPX_TEST_EQ(count10, data_size - end_size);
}

template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_default_construct_n_async(ExPolicy&& p, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<default_constructable>;

    base_iterator c(data_size, default_constructable(10));
    auto end_size = rand() % data_size;
    auto f = hpx::ranges::uninitialized_default_construct_n(
        p, std::begin(c), end_size);
    f.wait();

    std::size_t count42 = 0;
    std::size_t count10 = 0;
    std::for_each(std::begin(c), std::begin(c) + data_size,
        [&count42, &count10](default_constructable v1) {
            if (v1.value_ == 42)
            {
                count42++;
            }
            else if (v1.value_ == 10)
            {
                count10++;
            }
        });

    HPX_TEST_EQ(count42, end_size);
    HPX_TEST_EQ(count10, data_size - end_size);
}

template <typename IteratorTag>
void test_uninitialized_default_construct_n()
{
    using namespace hpx::execution;

    test_uninitialized_default_construct_n(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_uninitialized_default_construct_n(seq, IteratorTag());
        test_uninitialized_default_construct_n(par, IteratorTag());
        test_uninitialized_default_construct_n(par_unseq, IteratorTag());

        test_uninitialized_default_construct_n_async(seq(task), IteratorTag());
        test_uninitialized_default_construct_n_async(par(task), IteratorTag());
    }
}

void uninitialized_default_construct_n_test()
{
    test_uninitialized_default_construct_n<std::random_access_iterator_tag>();
    test_uninitialized_default_construct_n<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    uninitialized_default_construct_n_test();
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
