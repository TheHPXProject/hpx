//  Copyright (c) 2014 Grant Mercer
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
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

template <typename IteratorTag>
void test_lexicographical_compare1(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);

    //d is lexicographical less than c
    std::vector<std::size_t> d(10006);
    std::iota(std::begin(d), std::end(d), 0);

    bool res = hpx::lexicographical_compare(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), std::end(d));

    HPX_TEST(!res);
}

template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare1(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);

    //d is lexicographical less than c
    std::vector<std::size_t> d(10006);
    std::iota(std::begin(d), std::end(d), 0);

    bool res = hpx::lexicographical_compare(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), std::end(d));

    HPX_TEST(!res);
}

template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare1_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);

    // d is lexicographical less than c
    std::vector<std::size_t> d(10006);
    std::iota(std::begin(d), std::end(d), 0);

    hpx::future<bool> f =
        hpx::lexicographical_compare(p, iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(d), std::end(d));

    f.wait();

    bool res = f.get();

    HPX_TEST(!res);
}

template <typename IteratorTag>
void test_lexicographical_compare1()
{
    using namespace hpx::execution;
    test_lexicographical_compare1(IteratorTag());
    test_lexicographical_compare1(seq, IteratorTag());
    test_lexicographical_compare1(par, IteratorTag());
    test_lexicographical_compare1(par_unseq, IteratorTag());

    test_lexicographical_compare1_async(seq(task), IteratorTag());
    test_lexicographical_compare1_async(par(task), IteratorTag());
}

void lexicographical_compare_test1()
{
    test_lexicographical_compare1<std::random_access_iterator_tag>();
    test_lexicographical_compare1<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_lexicographical_compare2(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // lexicographically equal, so result is false
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);

    std::vector<std::size_t> d(10007);
    std::iota(std::begin(d), std::end(d), 0);

    bool res = hpx::lexicographical_compare(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), std::end(d));

    HPX_TEST(!res);
}

template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare2(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // lexicographically equal, so result is false
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);

    std::vector<std::size_t> d(10007);
    std::iota(std::begin(d), std::end(d), 0);

    bool res = hpx::lexicographical_compare(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), std::end(d));

    HPX_TEST(!res);
}

template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare2_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // lexicographically equal, so result is false
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);

    std::vector<std::size_t> d(10007);
    std::iota(std::begin(d), std::end(d), 0);

    hpx::future<bool> f =
        hpx::lexicographical_compare(p, iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(d), std::end(d));

    f.wait();

    HPX_TEST(!f.get());
}

template <typename IteratorTag>
void test_lexicographical_compare2()
{
    using namespace hpx::execution;
    test_lexicographical_compare2(IteratorTag());
    test_lexicographical_compare2(seq, IteratorTag());
    test_lexicographical_compare2(par, IteratorTag());
    test_lexicographical_compare2(par_unseq, IteratorTag());

    test_lexicographical_compare2_async(seq(task), IteratorTag());
    test_lexicographical_compare2_async(par(task), IteratorTag());
}

void lexicographical_compare_test2()
{
    test_lexicographical_compare2<std::random_access_iterator_tag>();
    test_lexicographical_compare2<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_lexicographical_compare3(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // C is lexicographically less due to the (gen() % size + 1)th
    // element being less than D
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);
    std::uniform_int_distribution<> dis(1, 5000);
    c[dis(gen)] = 0;    //-V108

    std::vector<std::size_t> d(10007);
    std::iota(std::begin(d), std::end(d), 0);

    bool res = hpx::lexicographical_compare(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), std::end(d));

    HPX_TEST(res);
}

template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare3(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // C is lexicographically less due to the (gen() % size + 1)th
    // element being less than D
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);
    std::uniform_int_distribution<> dis(1, 5000);
    c[dis(gen)] = 0;    //-V108

    std::vector<std::size_t> d(10007);
    std::iota(std::begin(d), std::end(d), 0);

    bool res = hpx::lexicographical_compare(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), std::end(d));

    HPX_TEST(res);
}

template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare3_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);
    std::uniform_int_distribution<> dis(1, 10006);
    c[dis(gen)] = 0;    //-V108

    std::vector<std::size_t> d(10007);
    std::iota(std::begin(d), std::end(d), 0);

    hpx::future<bool> f =
        hpx::lexicographical_compare(p, iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(d), std::end(d));

    f.wait();

    HPX_TEST(f.get());
}

template <typename IteratorTag>
void test_lexicographical_compare3()
{
    using namespace hpx::execution;
    test_lexicographical_compare3(IteratorTag());
    test_lexicographical_compare3(seq, IteratorTag());
    test_lexicographical_compare3(par, IteratorTag());
    test_lexicographical_compare3(par_unseq, IteratorTag());

    test_lexicographical_compare3_async(seq(task), IteratorTag());
    test_lexicographical_compare3_async(par(task), IteratorTag());
}

void lexicographical_compare_test3()
{
    test_lexicographical_compare3<std::random_access_iterator_tag>();
    test_lexicographical_compare3<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), 0);

    std::vector<std::size_t> h(10007);
    std::iota(std::begin(h), std::end(h), 0);

    bool caught_exception = false;
    try
    {
        hpx::lexicographical_compare(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c), []() { throw std::runtime_error("test"); }),
            std::begin(h), std::end(h));
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
void test_lexicographical_compare_async_exception(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), gen() + 1);

    std::vector<std::size_t> h(10006);
    std::fill(std::begin(h), std::end(h), gen() + 1);

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<bool> f = hpx::lexicographical_compare(p,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c), []() { throw std::runtime_error("test"); }),
            std::begin(h), std::end(h));
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
void test_lexicographical_compare_exception()
{
    using namespace hpx::execution;
    //If the execution policy object is of type vector_execution_policy,
    //  std::terminate shall be called. therefore we do not test exceptions
    //  with a vector execution policy
    test_lexicographical_compare_exception(seq, IteratorTag());
    test_lexicographical_compare_exception(par, IteratorTag());

    test_lexicographical_compare_async_exception(seq(task), IteratorTag());
    test_lexicographical_compare_async_exception(par(task), IteratorTag());
}

void lexicographical_compare_exception_test()
{
    test_lexicographical_compare_exception<std::random_access_iterator_tag>();
    test_lexicographical_compare_exception<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_lexicographical_compare_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), gen() + 1);

    std::vector<std::size_t> h(10006);
    std::fill(std::begin(h), std::end(h), gen() + 1);

    bool caught_bad_alloc = false;
    try
    {
        hpx::lexicographical_compare(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c), []() { throw std::bad_alloc(); }),
            std::begin(h), std::end(h));
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
void test_lexicographical_compare_async_bad_alloc(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), gen() + 1);

    std::vector<std::size_t> h(10006);
    std::fill(std::begin(h), std::end(h), gen() + 1);

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<bool> f = hpx::lexicographical_compare(p,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c), []() { throw std::bad_alloc(); }),
            std::begin(h), std::end(h));
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
void test_lexicographical_compare_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_lexicographical_compare_bad_alloc(par, IteratorTag());
    test_lexicographical_compare_bad_alloc(seq, IteratorTag());

    test_lexicographical_compare_async_bad_alloc(seq(task), IteratorTag());
    test_lexicographical_compare_async_bad_alloc(par(task), IteratorTag());
}

void lexicographical_compare_bad_alloc_test()
{
    test_lexicographical_compare_bad_alloc<std::random_access_iterator_tag>();
    test_lexicographical_compare_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = std::random_device{}();
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    lexicographical_compare_test1();
    lexicographical_compare_test2();
    lexicographical_compare_test3();
    lexicographical_compare_exception_test();
    lexicographical_compare_bad_alloc_test();
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
