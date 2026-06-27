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
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "../algorithms/test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<int> dis(3, 102);

template <typename ExPolicy, typename IteratorTag>
void test_search1(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    int h[] = {1, 2};

    iterator index = hpx::search(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search1_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    int h[] = {1, 2};

    hpx::future<iterator> f = hpx::search(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(f.get() == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search2(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c[0] = 1;
    c[1] = 2;

    int h[] = {1, 2};

    iterator index = hpx::search(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search2_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c[0] = 1;
    c[1] = 2;

    int h[] = {1, 2};

    hpx::future<iterator> f = hpx::search(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(f.get() == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search3(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    int h[] = {1, 2};

    iterator index = hpx::search(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == expected);
}

template <typename ExPolicy, typename IteratorTag>
void test_search3_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    int h[] = {1, 2};

    hpx::future<iterator> f = hpx::search(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator expected = std::search(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(f.get() == expected);
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_search1()
{
    using namespace hpx::execution;

    test_search1(simd, IteratorTag());
    test_search1(par_simd, IteratorTag());

    test_search1_async(simd(task), IteratorTag());
    test_search1_async(par_simd(task), IteratorTag());
}

template <typename IteratorTag>
void test_search2()
{
    using namespace hpx::execution;

    test_search2(simd, IteratorTag());
    test_search2(par_simd, IteratorTag());

    test_search2_async(simd(task), IteratorTag());
    test_search2_async(par_simd(task), IteratorTag());
}

template <typename IteratorTag>
void test_search3()
{
    using namespace hpx::execution;

    test_search3(simd, IteratorTag());
    test_search3(par_simd, IteratorTag());

    test_search3_async(simd(task), IteratorTag());
    test_search3_async(par_simd(task), IteratorTag());
}

void search_test1()
{
    test_search1<std::random_access_iterator_tag>();
    test_search1<std::forward_iterator_tag>();
}

void search_test2()
{
    test_search2<std::random_access_iterator_tag>();
    test_search2<std::forward_iterator_tag>();
}

void search_test3()
{
    test_search3<std::random_access_iterator_tag>();
    test_search3<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    search_test1();
    search_test2();
    search_test3();
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
