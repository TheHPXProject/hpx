//  Copyright (c) 2015-2020 Hartmut Kaiser
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
#include <string>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_set_union1(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    std::sort(std::begin(c1), std::end(c1));
    std::sort(std::begin(c2), std::end(c2));

    std::vector<std::size_t> c3(2 * c1.size()), c4(2 * c1.size());    //-V656

    hpx::ranges::set_union(iterator(std::begin(c1)), iterator(std::end(c1)),
        std::begin(c2), std::end(c2), std::begin(c3));

    std::set_union(std::begin(c1), std::end(c1), std::begin(c2), std::end(c2),
        std::begin(c4));

    // verify values
    HPX_TEST(std::equal(std::begin(c3), std::end(c3), std::begin(c4)));
}

template <typename ExPolicy, typename IteratorTag>
void test_set_union1(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    std::sort(std::begin(c1), std::end(c1));
    std::sort(std::begin(c2), std::end(c2));

    std::vector<std::size_t> c3(2 * c1.size()), c4(2 * c1.size());    //-V656

    hpx::ranges::set_union(policy, iterator(std::begin(c1)),
        iterator(std::end(c1)), std::begin(c2), std::end(c2), std::begin(c3));

    std::set_union(std::begin(c1), std::end(c1), std::begin(c2), std::end(c2),
        std::begin(c4));

    // verify values
    HPX_TEST(std::equal(std::begin(c3), std::end(c3), std::begin(c4)));
}

template <typename ExPolicy, typename IteratorTag>
void test_set_union1_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    std::sort(std::begin(c1), std::end(c1));
    std::sort(std::begin(c2), std::end(c2));

    std::vector<std::size_t> c3(2 * c1.size()), c4(2 * c1.size());    //-V656

    hpx::future<void> result = hpx::ranges::set_union(p,
        iterator(std::begin(c1)), iterator(std::end(c1)), std::begin(c2),
        std::end(c2), std::begin(c3));
    result.wait();

    std::set_union(std::begin(c1), std::end(c1), std::begin(c2), std::end(c2),
        std::begin(c4));

    // verify values
    HPX_TEST(std::equal(std::begin(c3), std::end(c3), std::begin(c4)));
}

template <typename IteratorTag>
void test_set_union1()
{
    using namespace hpx::execution;

    test_set_union1(IteratorTag());

    test_set_union1(seq, IteratorTag());
    test_set_union1(par, IteratorTag());
    test_set_union1(par_unseq, IteratorTag());

    test_set_union1_async(seq(task), IteratorTag());
    test_set_union1_async(par(task), IteratorTag());
}

void set_union_test1()
{
    test_set_union1<std::random_access_iterator_tag>();
    test_set_union1<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_set_union2(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    auto comp = [](std::size_t l, std::size_t r) { return l > r; };

    std::sort(std::begin(c1), std::end(c1), comp);
    std::sort(std::begin(c2), std::end(c2), comp);

    std::vector<std::size_t> c3(2 * c1.size()), c4(2 * c1.size());    //-V656

    hpx::ranges::set_union(iterator(std::begin(c1)), iterator(std::end(c1)),
        std::begin(c2), std::end(c2), std::begin(c3), comp);

    std::set_union(std::begin(c1), std::end(c1), std::begin(c2), std::end(c2),
        std::begin(c4), comp);

    // verify values
    HPX_TEST(std::equal(std::begin(c3), std::end(c3), std::begin(c4)));
}

template <typename ExPolicy, typename IteratorTag>
void test_set_union2(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    auto comp = [](std::size_t l, std::size_t r) { return l > r; };

    std::sort(std::begin(c1), std::end(c1), comp);
    std::sort(std::begin(c2), std::end(c2), comp);

    std::vector<std::size_t> c3(2 * c1.size()), c4(2 * c1.size());    //-V656

    hpx::ranges::set_union(policy, iterator(std::begin(c1)),
        iterator(std::end(c1)), std::begin(c2), std::end(c2), std::begin(c3),
        comp);

    std::set_union(std::begin(c1), std::end(c1), std::begin(c2), std::end(c2),
        std::begin(c4), comp);

    // verify values
    HPX_TEST(std::equal(std::begin(c3), std::end(c3), std::begin(c4)));
}

template <typename ExPolicy, typename IteratorTag>
void test_set_union2_async(ExPolicy&& p, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    auto comp = [](std::size_t l, std::size_t r) { return l > r; };

    std::sort(std::begin(c1), std::end(c1), comp);
    std::sort(std::begin(c2), std::end(c2), comp);

    std::vector<std::size_t> c3(2 * c1.size()), c4(2 * c1.size());    //-V656

    hpx::future<void> result = hpx::ranges::set_union(p,
        iterator(std::begin(c1)), iterator(std::end(c1)), std::begin(c2),
        std::end(c2), std::begin(c3), comp);
    result.wait();

    std::set_union(std::begin(c1), std::end(c1), std::begin(c2), std::end(c2),
        std::begin(c4), comp);

    // verify values
    HPX_TEST(std::equal(std::begin(c3), std::end(c3), std::begin(c4)));
}

template <typename IteratorTag>
void test_set_union2()
{
    using namespace hpx::execution;

    test_set_union2(IteratorTag());

    test_set_union2(seq, IteratorTag());
    test_set_union2(par, IteratorTag());
    test_set_union2(par_unseq, IteratorTag());

    test_set_union2_async(seq(task), IteratorTag());
    test_set_union2_async(par(task), IteratorTag());
}

void set_union_test2()
{
    test_set_union2<std::random_access_iterator_tag>();
    test_set_union2<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_set_union_exception(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    std::sort(std::begin(c1), std::end(c1));
    std::sort(std::begin(c2), std::end(c2));

    std::vector<std::size_t> c3(2 * c1.size());

    bool caught_exception = false;
    try
    {
        hpx::ranges::set_union(decorated_iterator(std::begin(c1),
                                   []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c1)), std::begin(c2), std::end(c2),
            std::begin(c3));

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
void test_set_union_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    std::sort(std::begin(c1), std::end(c1));
    std::sort(std::begin(c2), std::end(c2));

    std::vector<std::size_t> c3(2 * c1.size());

    bool caught_exception = false;
    try
    {
        hpx::ranges::set_union(policy,
            decorated_iterator(
                std::begin(c1), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c1)), std::begin(c2), std::end(c2),
            std::begin(c3));

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
void test_set_union_exception_async(ExPolicy&& p, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    std::sort(std::begin(c1), std::end(c1));
    std::sort(std::begin(c2), std::end(c2));

    std::vector<std::size_t> c3(2 * c1.size());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<void> f = hpx::ranges::set_union(p,
            decorated_iterator(
                std::begin(c1), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c1)), std::begin(c2), std::end(c2),
            std::begin(c3));

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
void test_set_union_exception()
{
    using namespace hpx::execution;

    test_set_union_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_set_union_exception(seq, IteratorTag());
    test_set_union_exception(par, IteratorTag());

    test_set_union_exception_async(seq(task), IteratorTag());
    test_set_union_exception_async(par(task), IteratorTag());
}

void set_union_exception_test()
{
    test_set_union_exception<std::random_access_iterator_tag>();
    test_set_union_exception<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_set_union_bad_alloc(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    std::sort(std::begin(c1), std::end(c1));
    std::sort(std::begin(c2), std::end(c2));

    std::vector<std::size_t> c3(2 * c1.size());

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::set_union(decorated_iterator(std::begin(c1),
                                   []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c1)), std::begin(c2), std::end(c2),
            std::begin(c3));

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
void test_set_union_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    std::sort(std::begin(c1), std::end(c1));
    std::sort(std::begin(c2), std::end(c2));

    std::vector<std::size_t> c3(2 * c1.size());

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::set_union(policy,
            decorated_iterator(
                std::begin(c1), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c1)), std::begin(c2), std::end(c2),
            std::begin(c3));

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
void test_set_union_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());

    std::sort(std::begin(c1), std::end(c1));
    std::sort(std::begin(c2), std::end(c2));

    std::vector<std::size_t> c3(2 * c1.size());

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<void> f = hpx::ranges::set_union(p,
            decorated_iterator(
                std::begin(c1), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c1)), std::begin(c2), std::end(c2),
            std::begin(c3));

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
void test_set_union_bad_alloc()
{
    using namespace hpx::execution;

    test_set_union_bad_alloc(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_set_union_bad_alloc(seq, IteratorTag());
    test_set_union_bad_alloc(par, IteratorTag());

    test_set_union_bad_alloc_async(seq(task), IteratorTag());
    test_set_union_bad_alloc_async(par(task), IteratorTag());
}

void set_union_bad_alloc_test()
{
    test_set_union_bad_alloc<std::random_access_iterator_tag>();
    test_set_union_bad_alloc<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
void test_set_union_result_iterators()
{
    using namespace hpx::execution;

    std::vector<std::size_t> c1(100), c2(100);
    std::iota(std::begin(c1), std::end(c1), std::size_t(0));
    std::iota(std::begin(c2), std::end(c2), std::size_t(0));

    std::vector<std::size_t> c3(2 * c1.size());

    auto result = hpx::ranges::set_union(par, c1, c2, std::begin(c3));

    // the parallel algorithm must report both input ranges as fully
    // consumed, like std::ranges::set_union does
    HPX_TEST(result.in1 == std::end(c1));
    HPX_TEST(result.in2 == std::end(c2));
    HPX_TEST(result.out == std::begin(c3) + c1.size());
    HPX_TEST(std::equal(std::begin(c1), std::end(c1), std::begin(c3)));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    set_union_test1();
    set_union_test2();
    test_set_union_result_iterators();
    set_union_exception_test();
    set_union_bad_alloc_test();
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
