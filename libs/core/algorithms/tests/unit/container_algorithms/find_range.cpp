//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
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

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(2, 101);

template <typename IteratorTag>
void test_find(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    //fill vector with random values about 1
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(c.size() / 2) = 1;

    iterator index = hpx::ranges::find(
        iterator(std::begin(c)), iterator(std::end(c)), std::size_t(1));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_find(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    //fill vector with random values about 1
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(c.size() / 2) = 1;

    iterator index = hpx::ranges::find(
        policy, iterator(std::begin(c)), iterator(std::end(c)), std::size_t(1));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_find_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    //fill vector with random values above 1
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(c.size() / 2) = 1;

    hpx::future<iterator> f = hpx::ranges::find(
        p, iterator(std::begin(c)), iterator(std::end(c)), std::size_t(1));
    f.wait();

    //create iterator at position of value to be found
    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(f.get() == iterator(test_index));
}

template <typename IteratorTag>
void test_find()
{
    using namespace hpx::execution;

    test_find(IteratorTag());

    test_find(seq, IteratorTag());
    test_find(par, IteratorTag());
    test_find(par_unseq, IteratorTag());

    test_find_async(seq(task), IteratorTag());
    test_find_async(par(task), IteratorTag());
}

void find_test()
{
    test_find<std::random_access_iterator_tag>();
    test_find<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_find_exception(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 0;

    bool caught_exception = false;
    try
    {
        hpx::ranges::find(decorated_iterator(std::begin(c),
                              []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), std::size_t(0));
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
void test_find_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 0;

    bool caught_exception = false;
    try
    {
        hpx::ranges::find(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), std::size_t(0));
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
void test_find_exception_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 0;

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<decorated_iterator> f = hpx::ranges::find(p,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), std::size_t(0));
        returned_from_algorithm = true;
        f.get();

        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(p, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
}

template <typename IteratorTag>
void test_find_exception()
{
    using namespace hpx::execution;

    test_find_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_find_exception(seq, IteratorTag());
    test_find_exception(par, IteratorTag());

    test_find_exception_async(seq(task), IteratorTag());
    test_find_exception_async(par(task), IteratorTag());
}

void find_exception_test()
{
    test_find_exception<std::random_access_iterator_tag>();
    test_find_exception<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_find_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(100007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 0;

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::find(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), std::size_t(0));
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
void test_find_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 0;

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<decorated_iterator> f = hpx::ranges::find(p,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), std::size_t(0));
        returned_from_algorithm = true;
        f.get();

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
void test_find_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_find_bad_alloc(seq, IteratorTag());
    test_find_bad_alloc(par, IteratorTag());

    test_find_bad_alloc_async(seq(task), IteratorTag());
    test_find_bad_alloc_async(par(task), IteratorTag());
}

void find_bad_alloc_test()
{
    test_find_bad_alloc<std::random_access_iterator_tag>();
    test_find_bad_alloc<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_last_sentinel(IteratorTag)
{
    using namespace hpx::execution;
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;
    using test_vector =
        test::test_sentinel_container<std::vector<std::size_t>, IteratorTag>;

    test_vector c(1007);
    std::fill(std::begin(c.base()), std::end(c.base()), dis(gen));
    c.base().at(c.size() / 2) = 1;
    c.base().at(c.size() - 50) = 1;

    auto pred = [](std::size_t val) { return val == 1; };

    // find_last
    {
        auto result = hpx::ranges::find_last(c, std::size_t(1));
        base_iterator test_index =
            std::begin(c.base()) + static_cast<std::ptrdiff_t>(c.size() - 50);
        HPX_TEST(result.begin() == iterator(test_index));
    }
    {
        auto result = hpx::ranges::find_last(seq, c, std::size_t(1));
        base_iterator test_index =
            std::begin(c.base()) + static_cast<std::ptrdiff_t>(c.size() - 50);
        HPX_TEST(result.begin() == iterator(test_index));
    }
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        auto result = hpx::ranges::find_last(par, c, std::size_t(1));
        base_iterator test_index =
            std::begin(c.base()) + static_cast<std::ptrdiff_t>(c.size() - 50);
        HPX_TEST(result.begin() == iterator(test_index));
    }

    // find_last_if
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        auto result = hpx::ranges::find_last_if(par, c, pred);
        base_iterator test_index =
            std::begin(c.base()) + static_cast<std::ptrdiff_t>(c.size() - 50);
        HPX_TEST(result.begin() == iterator(test_index));
    }

    // find_last_if_not
    {
        auto result = hpx::ranges::find_last_if_not(c, pred);
        base_iterator test_index =
            std::begin(c.base()) + static_cast<std::ptrdiff_t>(c.size() - 50);
        c.base().back() = 5;
        auto result2 = hpx::ranges::find_last_if_not(c, pred);
        HPX_TEST(
            result2.begin() == iterator(std::begin(c.base()) + c.size() - 1));
    }
    {
        c.base().back() = 5;
        auto result = hpx::ranges::find_last_if_not(seq, c, pred);
        HPX_TEST(
            result.begin() == iterator(std::begin(c.base()) + c.size() - 1));
    }
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        c.base().back() = 5;
        auto result = hpx::ranges::find_last_if_not(par, c, pred);
        HPX_TEST(
            result.begin() == iterator(std::begin(c.base()) + c.size() - 1));
    }
}

void find_last_range_test()
{
    test_find_last_sentinel(std::random_access_iterator_tag());
    test_find_last_sentinel(std::bidirectional_iterator_tag());
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    find_test();
    find_exception_test();
    find_bad_alloc_test();
    find_last_range_test();
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
