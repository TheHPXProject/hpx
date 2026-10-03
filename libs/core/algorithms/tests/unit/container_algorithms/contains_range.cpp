//  Copyright (c) 2024 Zakaria Abdi
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "test_utils.hpp"

//////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(2, 101);

template <typename IteratorTag>
void test_contains(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    std::size_t const n = c.size();
    c.at(n / 2) = 1;

    bool result1 = hpx::ranges::contains(
        iterator(std::begin(c)), iterator(std::end(c)), int(1));
    HPX_TEST_EQ(result1, true);
}

template <typename ExPolicy, typename IteratorTag>
void test_contains(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    std::size_t const n = c.size();
    c.at(n / 2) = 1;

    bool result1 = hpx::ranges::contains(
        policy, iterator(std::begin(c)), iterator(std::end(c)), int(1));
    HPX_TEST_EQ(result1, true);

    bool result2 = hpx::ranges::contains(
        policy, iterator(std::begin(c)), iterator(std::end(c)), int(110));
    HPX_TEST_EQ(result2, false);
}

template <typename ExPolicy, typename IteratorTag>
void test_contains_async(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::size_t const n = c.size();
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(n / 2) = 1;

    hpx::future<bool> result1 = hpx::ranges::contains(
        policy, iterator(std::begin(c)), iterator(std::end(c)), int(1));
    result1.wait();
    HPX_TEST_EQ(result1.get(), true);

    hpx::future<bool> result2 = hpx::ranges::contains(
        policy, iterator(std::begin(c)), iterator(std::end(c)), int(110));
    result2.wait();
    HPX_TEST_EQ(result2.get(), false);
}

template <typename IteratorTag>
void test_contains()
{
    using namespace hpx::execution;

    test_contains(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_contains(seq, IteratorTag());
        test_contains(par, IteratorTag());
        test_contains(par_unseq, IteratorTag());

        test_contains_async(seq(task), IteratorTag());
        test_contains_async(par(task), IteratorTag());
        test_contains_async(par_unseq(task), IteratorTag());
    }
}

void contains_test()
{
    test_contains<std::random_access_iterator_tag>();
    test_contains<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_contains_exception(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    std::size_t const n = c.size();
    c.at(n / 2) = 0;
    bool caught_exception = false;
    try
    {
        hpx::ranges::contains(decorated_iterator(std::begin(c),
                                  []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), int(0));

        HPX_TEST(false);
    }

    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<hpx::execution::sequenced_policy,
            IteratorTag>::call(hpx::execution::seq, e);
    }

    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_contains_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::size_t const n = c.size();
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c.at(n / 2) = 0;

    bool caught_exception = false;
    try
    {
        hpx::ranges::contains(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), int(0));

        HPX_TEST(false);
    }

    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
    }

    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_contains_exception_async(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::size_t const n = c.size();
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c.at(n / 2) = 0;

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<bool> result = hpx::ranges::contains(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), int(0));
        returned_from_algorithm = true;
        result.get();
        HPX_TEST(false);
    }

    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
    }

    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
}

template <typename IteratorTag>
void test_contains_exception()
{
    using namespace hpx::execution;

    test_contains_exception(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_contains_exception(seq, IteratorTag());
        test_contains_exception(par, IteratorTag());

        test_contains_exception_async(seq(task), IteratorTag());
        test_contains_exception_async(par(task), IteratorTag());
    }
}

void contains_exception_test()
{
    test_contains_exception<std::random_access_iterator_tag>();
    test_contains_exception<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////

template <typename ExPolicy, typename IteratorTag>
void test_contains_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_b<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::size_t const n = c.size();
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c.at(n / 2) = 0;
    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::contains(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), int(0));
        HPX_TEST(false);
    }

    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }

    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
}

template <typename ExPolicy, typename IteratorTag>
void test_contains_bad_alloc_async(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::size_t const n = c.size();
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c.at(n / 2) = 0;
    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<bool> result = hpx::ranges::contains(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), int(0));
        returned_from_algorithm = true;
        result.get();
        HPX_TEST(false);
    }

    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }

    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
    HPX_TEST(returned_from_algorithm);
}

template <typename IteratorTag>
void test_contains_bad_alloc()
{
    using namespace hpx::execution;

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_contains_bad_alloc(seq, IteratorTag());
        test_contains_bad_alloc(par, IteratorTag());

        test_contains_bad_alloc_async(seq(task), IteratorTag());
        test_contains_bad_alloc_async(par(task), IteratorTag());
    }
}

void contains_bad_alloc_test()
{
    test_contains_bad_alloc<std::random_access_iterator_tag>();
    test_contains_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "Using seed as: " << seed << std::endl;
    gen.seed(seed);

    contains_test();
    contains_exception_test();
    contains_bad_alloc_test();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage" HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        " the random number generator to use for this run");

    std::vector<std::string> cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
