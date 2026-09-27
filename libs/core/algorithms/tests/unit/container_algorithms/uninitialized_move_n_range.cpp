//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2015-2025 Hartmut Kaiser
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
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

#include <hpx/iterator_support/tests/iter_sent.hpp>
#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_uninitialized_move_n_sent(IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::rbegin(d));
    std::size_t sent_len = (std::rand() % 10007) + 1;
    hpx::ranges::uninitialized_move_n(std::begin(c), sent_len, std::begin(d),
        sentinel<std::size_t>{
            *(std::begin(d) + static_cast<std::ptrdiff_t>(sent_len))});

    std::size_t count = 0;
    // loop till for sent_len since either the sentinel for the input or output iterator
    // will be reached by then
    HPX_TEST(std::equal(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(sent_len), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST_EQ(count, sent_len);
}

template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_move_n_sent(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::rbegin(d));
    std::size_t sent_len = (std::rand() % 10007) + 1;
    hpx::ranges::uninitialized_move_n(policy, std::begin(c), sent_len,
        std::begin(d),
        test::make_sized_sentinel(std::begin(d),
            sentinel<std::size_t>{
                *(std::begin(d) + static_cast<std::ptrdiff_t>(sent_len))}));

    std::size_t count = 0;
    // loop till for sent_len since either the sentinel for the input or output iterator
    // will be reached by then
    HPX_TEST(std::equal(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(sent_len), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST_EQ(count, sent_len);
}

template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_move_n_sent_async(ExPolicy&& p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::rbegin(d));
    std::size_t sent_len = (std::rand() % 10007) + 1;
    auto f = hpx::ranges::uninitialized_move_n(p, std::begin(c), sent_len,
        std::begin(d),
        test::make_sized_sentinel(std::begin(d),
            sentinel<std::size_t>{
                *(std::begin(d) + static_cast<std::ptrdiff_t>(sent_len))}));
    f.wait();

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(sent_len), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, sent_len);
}

template <typename IteratorTag>
void test_uninitialized_move_n_sent()
{
    using namespace hpx::execution;

    test_uninitialized_move_n_sent(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_uninitialized_move_n_sent(seq, IteratorTag());
        test_uninitialized_move_n_sent(par, IteratorTag());
        test_uninitialized_move_n_sent(par_unseq, IteratorTag());

        test_uninitialized_move_n_sent_async(seq(task), IteratorTag());
        test_uninitialized_move_n_sent_async(par(task), IteratorTag());
    }
}

void uninitialized_move_n_sent_test()
{
    test_uninitialized_move_n_sent<std::random_access_iterator_tag>();
    test_uninitialized_move_n_sent<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    uninitialized_move_n_sent_test();
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
