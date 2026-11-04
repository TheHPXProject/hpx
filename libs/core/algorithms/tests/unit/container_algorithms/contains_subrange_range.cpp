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

unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(2, 101);

template <typename IteratorTag>
void test_contains_subrange(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007), c2 = {1, 2, 3};
    std::fill(std::begin(c1), std::end(c1), dis(gen));
    std::size_t const n = c1.size();
    std::size_t const mid = n / 2;

    c1.at(mid) = 1;
    c1.at(mid + 1) = 2;
    c1.at(mid + 2) = 3;

    bool result = hpx::ranges::contains_subrange(iterator(std::begin(c1)),
        iterator(std::end(c1)), iterator(std::begin(c2)),
        iterator(std::end(c2)));
    HPX_TEST_EQ(result, true);

    std::vector<int> empty;
    HPX_TEST(!hpx::ranges::contains_subrange(iterator(empty.begin()),
        iterator(empty.end()), iterator(c2.begin()), iterator(c2.end())));
    HPX_TEST(!hpx::ranges::contains_subrange(iterator(c2.begin()),
        iterator(c2.end()), iterator(c1.begin()), iterator(c1.end())));
}

template <typename ExPolicy, typename IteratorTag>
void test_contains_subrange(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007), c2 = {1, 2, 3};
    std::fill(std::begin(c1), std::end(c1), dis(gen));
    std::size_t const n = c1.size();
    std::size_t const mid = n / 2;

    c1.at(mid) = 1;
    c1.at(mid + 1) = 2;
    c1.at(mid + 2) = 3;

    bool result1 = hpx::ranges::contains_subrange(policy,
        iterator(std::begin(c1)), iterator(std::end(c1)),
        iterator(std::begin(c2)), iterator(std::end(c2)));
    HPX_TEST_EQ(result1, true);

    std::vector<int> empty;
    HPX_TEST(!hpx::ranges::contains_subrange(policy, iterator(empty.begin()),
        iterator(empty.end()), iterator(c2.begin()), iterator(c2.end())));
    HPX_TEST(!hpx::ranges::contains_subrange(policy, iterator(c2.begin()),
        iterator(c2.end()), iterator(c1.begin()), iterator(c1.end())));
}

template <typename ExPolicy, typename IteratorTag>
void test_contains_subrange_async(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007), c2 = {1, 2, 3};
    std::fill(std::begin(c1), std::end(c1), dis(gen));
    std::size_t const n = c1.size();
    std::size_t const mid = n / 2;

    c1.at(mid) = 1;
    c1.at(mid + 1) = 2;
    c1.at(mid + 2) = 3;

    hpx::future<bool> result = hpx::ranges::contains_subrange(policy,
        iterator(std::begin(c1)), iterator(std::end(c1)),
        iterator(std::begin(c2)), iterator(std::end(c2)));
    result.wait();
    HPX_TEST_EQ(result.get(), true);

    std::vector<int> empty;
    hpx::future<bool> empty_result =
        hpx::ranges::contains_subrange(policy, iterator(empty.begin()),
            iterator(empty.end()), iterator(c2.begin()), iterator(c2.end()));
    HPX_TEST(!empty_result.get());
    hpx::future<bool> longer_result =
        hpx::ranges::contains_subrange(policy, iterator(c2.begin()),
            iterator(c2.end()), iterator(c1.begin()), iterator(c1.end()));
    HPX_TEST(!longer_result.get());
}

template <typename IteratorTag>
void test_contains_subrange()
{
    using namespace hpx::execution;

    test_contains_subrange(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_contains_subrange(seq, IteratorTag());
        test_contains_subrange(par, IteratorTag());
        test_contains_subrange(par_unseq, IteratorTag());

        test_contains_subrange_async(seq(task), IteratorTag());
        test_contains_subrange_async(par(task), IteratorTag());
        test_contains_subrange_async(par_unseq(task), IteratorTag());
    }
}

void contains_subrange_test()
{
    test_contains_subrange<std::random_access_iterator_tag>();
    test_contains_subrange<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_contains_subrange_exception(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c1(10007), c2 = {1, 2, 3};
    std::iota(std::begin(c1), std::end(c1), gen() + 1);
    std::size_t const n = c1.size();
    std::size_t const mid = n / 2;

    c1.at(mid) = 1;
    c1.at(mid + 1) = 2;
    c1.at(mid + 2) = 3;

    bool caught_exception = false;
    try
    {
        hpx::ranges::contains_subrange(
            decorated_iterator(
                std::begin(c1), []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c1), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::begin(c2)),
            decorated_iterator(std::end(c2)));
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
void test_contains_subrange_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c1(10007), c2 = {1, 2, 3};
    std::iota(std::begin(c1), std::end(c1), gen() + 1);
    std::size_t const n = c1.size();
    std::size_t const mid = n / 2;

    c1.at(mid) = 1;
    c1.at(mid + 1) = 2;
    c1.at(mid + 2) = 3;

    bool caught_exception = false;
    try
    {
        hpx::ranges::contains_subrange(policy,
            decorated_iterator(
                std::begin(c1), []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c1), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::begin(c2)),
            decorated_iterator(std::end(c2)));

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
void test_contains_subrange_exception_async(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c1(10007), c2 = {1, 2, 3};
    std::iota(std::begin(c1), std::end(c1), gen() + 1);
    std::size_t const n = c1.size();
    std::size_t const mid = n / 2;

    c1.at(mid) = 1;
    c1.at(mid + 1) = 2;
    c1.at(mid + 2) = 3;

    bool caught_exception = false;
    try
    {
        hpx::future<bool> result = hpx::ranges::contains_subrange(policy,
            decorated_iterator(
                std::begin(c1), []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c1), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::begin(c2)),
            decorated_iterator(std::end(c2)));
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
}

template <typename IteratorTag>
void test_contains_subrange_exception()
{
    using namespace hpx::execution;

    test_contains_subrange_exception(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_contains_subrange_exception(seq, IteratorTag());
        test_contains_subrange_exception(par, IteratorTag());

        test_contains_subrange_exception_async(seq(task), IteratorTag());
        test_contains_subrange_exception_async(par(task), IteratorTag());
    }
}

void contains_subrange_exception_test()
{
    test_contains_subrange_exception<std::random_access_iterator_tag>();
    test_contains_subrange_exception<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_contains_subrange_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<int> c1(10007), c2 = {1, 2, 3};
    std::iota(std::begin(c1), std::end(c1), gen() + 1);
    std::size_t const n = c1.size();
    std::size_t const mid = n / 2;

    c1.at(mid) = 1;
    c1.at(mid + 1) = 2;
    c1.at(mid + 2) = 3;

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::contains_subrange(policy,
            decorated_iterator(
                std::begin(c1), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c1), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::begin(c2)),
            decorated_iterator(std::end(c2)));
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
void test_contains_subrange_bad_alloc_async(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<int> c1(10007), c2 = {1, 2, 3};
    std::iota(std::begin(c1), std::end(c1), gen() + 1);
    std::size_t const n = c1.size();
    std::size_t const mid = n / 2;

    c1.at(mid) = 1;
    c1.at(mid + 1) = 2;
    c1.at(mid + 2) = 3;

    bool caught_bad_alloc = false;
    try
    {
        hpx::future<bool> result = hpx::ranges::contains_subrange(policy,
            decorated_iterator(
                std::begin(c1), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c1), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::begin(c2)),
            decorated_iterator(std::end(c2)));
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
}

template <typename IteratorTag>
void test_contains_subrange_bad_alloc()
{
    using namespace hpx::execution;

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_contains_subrange_bad_alloc(seq, IteratorTag());
        test_contains_subrange_bad_alloc(par, IteratorTag());

        test_contains_subrange_bad_alloc_async(seq(task), IteratorTag());
        test_contains_subrange_bad_alloc_async(par(task), IteratorTag());
    }
}

void contains_subrange_bad_alloc_test()
{
    test_contains_subrange_bad_alloc<std::random_access_iterator_tag>();
    test_contains_subrange_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "Using seed as " << seed << std::endl;
    gen.seed(seed);
    contains_subrange_test();
    contains_subrange_exception_test();
    contains_subrange_bad_alloc_test();

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
