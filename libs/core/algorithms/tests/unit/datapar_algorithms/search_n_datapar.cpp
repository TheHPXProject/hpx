//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/datapar.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "../algorithms/test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();

template <typename ExPolicy, typename IteratorTag>
void test_search_n1(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007, 2);
    std::size_t mid = c.size() / 2;
    c[mid] = 1;
    c[mid + 1] = 1;
    c[mid + 2] = 1;

    iterator index = hpx::search_n(
        policy, iterator(std::begin(c)), iterator(std::end(c)), 3, 1);

    iterator expected =
        std::search_n(iterator(std::begin(c)), iterator(std::end(c)), 3, 1);

    HPX_TEST(index == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n1_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007, 2);
    std::size_t mid = c.size() / 2;
    c[mid] = 1;
    c[mid + 1] = 1;
    c[mid + 2] = 1;

    hpx::future<iterator> f =
        hpx::search_n(p, iterator(std::begin(c)), iterator(std::end(c)), 3, 1);

    iterator expected =
        std::search_n(iterator(std::begin(c)), iterator(std::end(c)), 3, 1);

    HPX_TEST(f.get() == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n2(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007, 2);

    iterator index = hpx::search_n(
        policy, iterator(std::begin(c)), iterator(std::end(c)), 3, 99);

    HPX_TEST(index == iterator(std::end(c)));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n2_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007, 2);

    hpx::future<iterator> f =
        hpx::search_n(p, iterator(std::begin(c)), iterator(std::end(c)), 3, 99);

    HPX_TEST(f.get() == iterator(std::end(c)));
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n3(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007, 2);
    c[0] = 1;
    c[1] = 1;
    c[2] = 1;

    iterator index = hpx::search_n(
        policy, iterator(std::begin(c)), iterator(std::end(c)), 3, 1);

    iterator expected =
        std::search_n(iterator(std::begin(c)), iterator(std::end(c)), 3, 1);

    HPX_TEST(index == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search_n3_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007, 2);
    c[0] = 1;
    c[1] = 1;
    c[2] = 1;

    hpx::future<iterator> f =
        hpx::search_n(p, iterator(std::begin(c)), iterator(std::end(c)), 3, 1);

    iterator expected =
        std::search_n(iterator(std::begin(c)), iterator(std::end(c)), 3, 1);

    HPX_TEST(f.get() == expected);
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_search_n1()
{
    using namespace hpx::execution;

    test_search_n1(simd, IteratorTag());
    test_search_n1(par_simd, IteratorTag());

    test_search_n1_async(simd(task), IteratorTag());
    test_search_n1_async(par_simd(task), IteratorTag());
}

template <typename IteratorTag>
void test_search_n2()
{
    using namespace hpx::execution;

    test_search_n2(simd, IteratorTag());
    test_search_n2(par_simd, IteratorTag());

    test_search_n2_async(simd(task), IteratorTag());
    test_search_n2_async(par_simd(task), IteratorTag());
}

template <typename IteratorTag>
void test_search_n3()
{
    using namespace hpx::execution;

    test_search_n3(simd, IteratorTag());
    test_search_n3(par_simd, IteratorTag());

    test_search_n3_async(simd(task), IteratorTag());
    test_search_n3_async(par_simd(task), IteratorTag());
}

void search_n_test1()
{
    test_search_n1<std::random_access_iterator_tag>();
    test_search_n1<std::forward_iterator_tag>();
}

void search_n_test2()
{
    test_search_n2<std::random_access_iterator_tag>();
    test_search_n2<std::forward_iterator_tag>();
}

void search_n_test3()
{
    test_search_n3<std::random_access_iterator_tag>();
    test_search_n3<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;

    search_n_test1();
    search_n_test2();
    search_n_test3();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
