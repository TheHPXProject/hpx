//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2020 Francisco Jose Tapia (fjtapia@gmail.com )
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

#define TEST_SIZE 10007

template <typename IteratorTag>
void test_nth_element(IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(TEST_SIZE);
    std::generate(
        std::begin(c), std::end(c), []() { return std::rand() % TEST_SIZE; });
    std::vector<std::size_t> d = c;

    auto rand_index = std::rand() % TEST_SIZE;

    hpx::nth_element(iterator(std::begin(c)),
        iterator(std::begin(c) + rand_index), iterator(std::end(c)));

    std::nth_element(std::begin(d), std::begin(d) + rand_index, std::end(d));

    HPX_TEST(*(std::begin(c) + rand_index) == *(std::begin(d) + rand_index));

    for (int k = 0; k < rand_index; k++)
    {
        HPX_TEST(c[k] <= c[rand_index]);
    }

    for (int k = rand_index + 1; k < TEST_SIZE; k++)
    {
        HPX_TEST(c[k] >= c[rand_index]);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_nth_element(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(TEST_SIZE);
    std::generate(
        std::begin(c), std::end(c), []() { return std::rand() % TEST_SIZE; });
    std::vector<std::size_t> d = c;

    auto rand_index = std::rand() % TEST_SIZE;

    hpx::nth_element(policy, iterator(std::begin(c)),
        iterator(std::begin(c) + rand_index), iterator(std::end(c)));

    std::nth_element(std::begin(d), std::begin(d) + rand_index, std::end(d));

    HPX_TEST(*(std::begin(c) + rand_index) == *(std::begin(d) + rand_index));

    for (int k = 0; k < rand_index; k++)
    {
        HPX_TEST(c[k] <= c[rand_index]);
    }

    for (int k = rand_index + 1; k < TEST_SIZE; k++)
    {
        HPX_TEST(c[k] >= c[rand_index]);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_nth_element_async(ExPolicy p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(TEST_SIZE);
    std::generate(
        std::begin(c), std::end(c), []() { return std::rand() % TEST_SIZE; });
    std::vector<std::size_t> d = c;

    auto rand_index = std::rand() % TEST_SIZE;

    auto actual = hpx::nth_element(p, iterator(std::begin(c)),
        iterator(std::begin(c) + rand_index), iterator(std::end(c)));

    std::nth_element(std::begin(d), std::begin(d) + rand_index, std::end(d));

    actual.wait();
    HPX_TEST(*(std::begin(c) + rand_index) == *(std::begin(d) + rand_index));

    for (int k = 0; k < rand_index; k++)
    {
        HPX_TEST(c[k] <= c[rand_index]);
    }

    for (int k = rand_index + 1; k < TEST_SIZE; k++)
    {
        HPX_TEST(c[k] >= c[rand_index]);
    }
}

template <typename IteratorTag>
void test_nth_element()
{
    using namespace hpx::execution;
    test_nth_element(IteratorTag());
    test_nth_element(seq, IteratorTag());
    test_nth_element(par, IteratorTag());
    test_nth_element(par_unseq, IteratorTag());

    test_nth_element_async(seq(task), IteratorTag());
    test_nth_element_async(par(task), IteratorTag());
}

void nth_element_test()
{
    test_nth_element<std::random_access_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_nth_element_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(TEST_SIZE);
    std::generate(
        std::begin(c), std::end(c), []() { return std::rand() % TEST_SIZE; });

    auto rand_index = std::rand() % TEST_SIZE;

    bool caught_exception = false;
    try
    {
        hpx::nth_element(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::begin(c) + rand_index,
                []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c), []() { throw std::runtime_error("test"); }));
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
void test_nth_element_async_exception(ExPolicy p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(TEST_SIZE);
    std::generate(
        std::begin(c), std::end(c), []() { return std::rand() % TEST_SIZE; });

    auto rand_index = std::rand() % TEST_SIZE;

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::nth_element(p,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::begin(c) + rand_index,
                []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c), []() { throw std::runtime_error("test"); }));
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
void test_nth_element_exception()
{
    using namespace hpx::execution;
    //If the execution policy object is of type vector_execution_policy,
    //  std::terminate shall be called. therefore we do not test exceptions
    //  with a vector execution policy
    test_nth_element_exception(seq, IteratorTag());
    test_nth_element_exception(par, IteratorTag());

    test_nth_element_async_exception(seq(task), IteratorTag());
    test_nth_element_async_exception(par(task), IteratorTag());
}

void nth_element_exception_test()
{
    test_nth_element_exception<std::random_access_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_nth_element_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(TEST_SIZE);
    std::generate(
        std::begin(c), std::end(c), []() { return std::rand() % TEST_SIZE; });

    auto rand_index = std::rand() % TEST_SIZE;

    bool caught_bad_alloc = false;
    try
    {
        hpx::nth_element(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(
                std::begin(c) + rand_index, []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c), []() { throw std::bad_alloc(); }));
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
void test_nth_element_async_bad_alloc(ExPolicy p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(TEST_SIZE);
    std::generate(
        std::begin(c), std::end(c), []() { return std::rand() % TEST_SIZE; });

    auto rand_index = std::rand() % TEST_SIZE;

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::nth_element(p,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(
                std::begin(c) + rand_index, []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c), []() { throw std::bad_alloc(); }));
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
void test_nth_element_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_nth_element_bad_alloc(par, IteratorTag());
    test_nth_element_bad_alloc(seq, IteratorTag());

    test_nth_element_async_bad_alloc(seq(task), IteratorTag());
    test_nth_element_async_bad_alloc(par(task), IteratorTag());
}

void nth_element_bad_alloc_test()
{
    test_nth_element_bad_alloc<std::random_access_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    nth_element_test();
    nth_element_exception_test();
    nth_element_bad_alloc_test();
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
