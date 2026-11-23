//  Copyright (c) 2014-2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/iterator_support/tests/iter_sent.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

template <typename IteratorTag>
void test_reduce_sent(IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t val(42);
    auto op = [val](std::size_t v1, std::size_t v2) { return v1 + v2 + val; };

    std::size_t r1 = hpx::ranges::reduce(
        std::begin(c), sentinel<std::size_t>{*(std::begin(c) + 100)}, val, op);

    // verify values
    std::size_t r2 =
        std::accumulate(std::begin(c), std::begin(c) + 100, val, op);
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce_sent(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t val(42);
    auto op = [val](std::size_t v1, std::size_t v2) { return v1 + v2 + val; };

    std::size_t r1 = hpx::ranges::reduce(policy, std::begin(c),
        sentinel<std::size_t>{*(std::begin(c) + 100)}, val, op);

    // verify values
    std::size_t r2 =
        std::accumulate(std::begin(c), std::begin(c) + 100, val, op);
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce_sent_async(ExPolicy p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t val(42);
    auto op = [val](std::size_t v1, std::size_t v2) { return v1 + v2 + val; };

    hpx::future<std::size_t> f = hpx::ranges::reduce(p, std::begin(c),
        sentinel<std::size_t>{*(std::begin(c) + 100)}, val, op);
    f.wait();

    // verify values
    std::size_t r2 =
        std::accumulate(std::begin(c), std::begin(c) + 100, val, op);
    HPX_TEST_EQ(f.get(), r2);
}

template <typename IteratorTag>
void test_reduce_sent()
{
    using namespace hpx::execution;

    test_reduce_sent(IteratorTag());
    test_reduce_sent(seq, IteratorTag());
    test_reduce_sent(par, IteratorTag());
    test_reduce_sent(par_unseq, IteratorTag());

    test_reduce_sent_async(seq(task), IteratorTag());
    test_reduce_sent_async(par(task), IteratorTag());
}

void reduce_test_sent()
{
    test_reduce_sent<std::random_access_iterator_tag>();
    test_reduce_sent<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce1(IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t val(42);
    auto op = [val](std::size_t v1, std::size_t v2) { return v1 + v2 + val; };

    std::size_t r1 = hpx::ranges::reduce(c, val, op);

    // verify values
    std::size_t r2 = std::accumulate(std::begin(c), std::end(c), val, op);
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce1(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t val(42);
    auto op = [val](std::size_t v1, std::size_t v2) { return v1 + v2 + val; };

    std::size_t r1 = hpx::ranges::reduce(policy, c, val, op);

    // verify values
    std::size_t r2 = std::accumulate(std::begin(c), std::end(c), val, op);
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce1_async(ExPolicy p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t val(42);
    auto op = [val](std::size_t v1, std::size_t v2) { return v1 + v2 + val; };

    hpx::future<std::size_t> f = hpx::ranges::reduce(p, c, val, op);
    f.wait();

    // verify values
    std::size_t r2 = std::accumulate(std::begin(c), std::end(c), val, op);
    HPX_TEST_EQ(f.get(), r2);
}

template <typename IteratorTag>
void test_reduce1()
{
    using namespace hpx::execution;

    test_reduce1(IteratorTag());
    test_reduce1(seq, IteratorTag());
    test_reduce1(par, IteratorTag());
    test_reduce1(par_unseq, IteratorTag());

    test_reduce1_async(seq(task), IteratorTag());
    test_reduce1_async(par(task), IteratorTag());
}

void reduce_test1()
{
    test_reduce1<std::random_access_iterator_tag>();
    test_reduce1<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce2(IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t const val(42);
    std::size_t r1 = hpx::ranges::reduce(c, val);

    // verify values
    std::size_t r2 = std::accumulate(std::begin(c), std::end(c), val);
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce2(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t const val(42);
    std::size_t r1 = hpx::ranges::reduce(policy, c, val);

    // verify values
    std::size_t r2 = std::accumulate(std::begin(c), std::end(c), val);
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce2_async(ExPolicy p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t const val(42);
    hpx::future<std::size_t> f = hpx::ranges::reduce(p, c, val);
    f.wait();

    // verify values
    std::size_t r2 = std::accumulate(std::begin(c), std::end(c), val);
    HPX_TEST_EQ(f.get(), r2);
}

template <typename IteratorTag>
void test_reduce2()
{
    using namespace hpx::execution;

    test_reduce2(IteratorTag());
    test_reduce2(seq, IteratorTag());
    test_reduce2(par, IteratorTag());
    test_reduce2(par_unseq, IteratorTag());

    test_reduce2_async(seq(task), IteratorTag());
    test_reduce2_async(par(task), IteratorTag());
}

void reduce_test2()
{
    test_reduce2<std::random_access_iterator_tag>();
    test_reduce2<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce3(IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t r1 = hpx::ranges::reduce(c);

    // verify values
    std::size_t r2 =
        std::accumulate(std::begin(c), std::end(c), std::size_t(0));
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce3(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t r1 = hpx::ranges::reduce(policy, c);

    // verify values
    std::size_t r2 =
        std::accumulate(std::begin(c), std::end(c), std::size_t(0));
    HPX_TEST_EQ(r1, r2);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce3_async(ExPolicy p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    hpx::future<std::size_t> f = hpx::ranges::reduce(p, c);
    f.wait();

    // verify values
    std::size_t r2 =
        std::accumulate(std::begin(c), std::end(c), std::size_t(0));
    HPX_TEST_EQ(f.get(), r2);
}

template <typename IteratorTag>
void test_reduce3()
{
    using namespace hpx::execution;

    test_reduce3(IteratorTag());
    test_reduce3(seq, IteratorTag());
    test_reduce3(par, IteratorTag());
    test_reduce3(par_unseq, IteratorTag());

    test_reduce3_async(seq(task), IteratorTag());
    test_reduce3_async(par(task), IteratorTag());
}

void reduce_test3()
{
    test_reduce3<std::random_access_iterator_tag>();
    test_reduce3<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_reduce_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_exception = false;
    try
    {
        hpx::ranges::reduce(
            policy, c, std::size_t(42), [](std::size_t v1, std::size_t v2) {
                return throw std::runtime_error("test"), v1 + v2;
            });

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
void test_reduce_exception_async(ExPolicy p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<void> f = hpx::ranges::reduce(
            p, c, std::size_t(42), [](std::size_t v1, std::size_t v2) {
                return throw std::runtime_error("test"), v1 + v2;
            });
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
void test_reduce_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_reduce_exception(seq, IteratorTag());
    test_reduce_exception(par, IteratorTag());

    test_reduce_exception_async(seq(task), IteratorTag());
    test_reduce_exception_async(par(task), IteratorTag());
}

void reduce_exception_test()
{
    test_reduce_exception<std::random_access_iterator_tag>();
    test_reduce_exception<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_reduce_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_exception = false;
    try
    {
        hpx::ranges::reduce(
            policy, c, std::size_t(42), [](std::size_t v1, std::size_t v2) {
                return throw std::bad_alloc(), v1 + v2;
            });

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_reduce_bad_alloc_async(ExPolicy p, IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<void> f = hpx::ranges::reduce(
            p, c, std::size_t(42), [](std::size_t v1, std::size_t v2) {
                return throw std::bad_alloc(), v1 + v2;
            });
        returned_from_algorithm = true;
        f.get();

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_exception = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
}

template <typename IteratorTag>
void test_reduce_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_reduce_bad_alloc(seq, IteratorTag());
    test_reduce_bad_alloc(par, IteratorTag());

    test_reduce_bad_alloc_async(seq(task), IteratorTag());
    test_reduce_bad_alloc_async(par(task), IteratorTag());
}

void reduce_bad_alloc_test()
{
    test_reduce_bad_alloc<std::random_access_iterator_tag>();
    test_reduce_bad_alloc<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = std::random_device{}();
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    reduce_test_sent();
    reduce_test1();
    reduce_test2();
    reduce_test3();

    reduce_exception_test();
    reduce_bad_alloc_test();
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
