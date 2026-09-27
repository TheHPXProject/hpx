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
void test_uninitialized_copy_sent(IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::rbegin(d));
    std::size_t sent_len = (std::rand() % 10007) + 1;
    hpx::ranges::uninitialized_copy(std::begin(c),
        sentinel<std::size_t>{
            *(std::begin(c) + static_cast<std::ptrdiff_t>(sent_len))},
        std::begin(d),
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
void test_uninitialized_copy_sent(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::rbegin(d));
    std::size_t sent_len = (std::rand() % 10007) + 1;
    hpx::ranges::uninitialized_copy(policy, std::begin(c),
        test::make_sized_sentinel(std::begin(c),
            sentinel<std::size_t>{
                *(std::begin(c) + static_cast<std::ptrdiff_t>(sent_len))}),
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
void test_uninitialized_copy_sent_async(ExPolicy&& p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    std::copy(std::begin(c), std::end(c), std::rbegin(d));
    std::size_t sent_len = (std::rand() % 10007) + 1;
    auto f = hpx::ranges::uninitialized_copy(p, std::begin(c),
        test::make_sized_sentinel(std::begin(c),
            sentinel<std::size_t>{
                *(std::begin(c) + static_cast<std::ptrdiff_t>(sent_len))}),
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
void test_uninitialized_copy_sent()
{
    using namespace hpx::execution;

    test_uninitialized_copy_sent(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_uninitialized_copy_sent(seq, IteratorTag());
        test_uninitialized_copy_sent(par, IteratorTag());
        test_uninitialized_copy_sent(par_unseq, IteratorTag());

        test_uninitialized_copy_sent_async(seq(task), IteratorTag());
        test_uninitialized_copy_sent_async(par(task), IteratorTag());
    }
}

void uninitialized_copy_sent_test()
{
    test_uninitialized_copy_sent<std::random_access_iterator_tag>();
    test_uninitialized_copy_sent<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_uninitialized_copy(IteratorTag)
{
    using test_vector =
        typename test::test_container<std::vector<std::size_t>, IteratorTag>;

    test_vector c(10007);
    test_vector d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    hpx::ranges::uninitialized_copy(c, d);

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
void test_uninitialized_copy(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using test_vector =
        typename test::test_container<std::vector<std::size_t>, IteratorTag>;

    test_vector c(10007);
    test_vector d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());
    hpx::ranges::uninitialized_copy(policy, c, d);

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
void test_uninitialized_copy_async(ExPolicy&& p, IteratorTag)
{
    using test_vector =
        typename test::test_container<std::vector<std::size_t>, IteratorTag>;

    test_vector c(10007);
    test_vector d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    auto f = hpx::ranges::uninitialized_copy(p, c, d);
    f.wait();

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
void test_uninitialized_copy()
{
    using namespace hpx::execution;

    test_uninitialized_copy(IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_uninitialized_copy(seq, IteratorTag());
        test_uninitialized_copy(par, IteratorTag());
        test_uninitialized_copy(par_unseq, IteratorTag());

        test_uninitialized_copy_async(seq(task), IteratorTag());
        test_uninitialized_copy_async(par(task), IteratorTag());
    }
}

void uninitialized_copy_test()
{
    test_uninitialized_copy<std::random_access_iterator_tag>();
    test_uninitialized_copy<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_uninitialized_copy_exception(IteratorTag)
{
    using base_iterator = typename std::vector<std::size_t>::iterator;
    using decorated_iterator =
        typename test::decorated_iterator<base_iterator, IteratorTag>;
    using test_vector =
        typename test::test_container<std::vector<std::size_t>, IteratorTag>;

    std::vector<std::size_t> c(10007);
    test_vector d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    try
    {
        hpx::ranges::uninitialized_copy(
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            d);
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
void test_uninitialized_copy_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = typename std::vector<std::size_t>::iterator;
    using decorated_iterator =
        typename test::decorated_iterator<base_iterator, IteratorTag>;
    using test_vector =
        typename test::test_container<std::vector<std::size_t>, IteratorTag>;

    std::vector<std::size_t> c(10007);
    test_vector d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    try
    {
        hpx::ranges::uninitialized_copy(policy,
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            d);
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
void test_uninitialized_copy_exception_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = typename std::vector<std::size_t>::iterator;
    using decorated_iterator =
        typename test::decorated_iterator<base_iterator, IteratorTag>;
    using test_vector =
        typename test::test_container<std::vector<std::size_t>, IteratorTag>;

    std::vector<std::size_t> c(10007);
    test_vector d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::uninitialized_copy(p,
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            d);
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
void test_uninitialized_copy_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_uninitialized_copy_exception(seq, IteratorTag());
        test_uninitialized_copy_exception(par, IteratorTag());

        test_uninitialized_copy_exception_async(seq(task), IteratorTag());
        test_uninitialized_copy_exception_async(par(task), IteratorTag());
    }
}

void uninitialized_copy_exception_test()
{
    test_uninitialized_copy_exception<std::random_access_iterator_tag>();
    test_uninitialized_copy_exception<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_uninitialized_copy_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = typename std::vector<std::size_t>::iterator;
    using decorated_iterator =
        typename test::decorated_iterator<base_iterator, IteratorTag>;
    using test_vector =
        typename test::test_container<std::vector<std::size_t>, IteratorTag>;

    std::vector<std::size_t> c(10007);
    test_vector d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::uninitialized_copy(policy,
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            d);
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
void test_uninitialized_copy_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    using base_iterator = typename std::vector<std::size_t>::iterator;
    using decorated_iterator =
        typename test::decorated_iterator<base_iterator, IteratorTag>;
    using test_vector =
        typename test::test_container<std::vector<std::size_t>, IteratorTag>;

    std::vector<std::size_t> c(10007);
    test_vector d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::uninitialized_copy(p,
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            d);
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
void test_uninitialized_copy_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_uninitialized_copy_bad_alloc(seq, IteratorTag());
        test_uninitialized_copy_bad_alloc(par, IteratorTag());

        test_uninitialized_copy_bad_alloc_async(seq(task), IteratorTag());
        test_uninitialized_copy_bad_alloc_async(par(task), IteratorTag());
    }
}

void uninitialized_copy_bad_alloc_test()
{
    test_uninitialized_copy_bad_alloc<std::random_access_iterator_tag>();
    test_uninitialized_copy_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    uninitialized_copy_test();
    uninitialized_copy_sent_test();
    uninitialized_copy_exception_test();
    uninitialized_copy_bad_alloc_test();
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
