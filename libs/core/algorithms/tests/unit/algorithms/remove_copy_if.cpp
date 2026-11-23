//  Copyright (c) 2015 Daniel Bourgeois
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
void test_remove_copy_if(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    std::uniform_int_distribution<> dis(
        0, static_cast<int>((c.size() >> 1) - 1));
    std::uniform_int_distribution<> dist(0, static_cast<int>(c.size() - 1));

    std::size_t middle_idx = dis(gen);
    auto middle = std::begin(c) + static_cast<std::ptrdiff_t>(middle_idx);
    std::iota(std::begin(c), middle, static_cast<int>(dist(gen)));
    std::fill(middle, std::end(c), -1);

    hpx::remove_copy_if(iterator(std::begin(c)), iterator(std::end(c)),
        std::begin(d), [](int i) { return i < 0; });

    std::size_t count = 0;
    HPX_TEST(std::equal(
        std::begin(c), middle, std::begin(d), [&count](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST(std::equal(middle, std::end(c), std::begin(d) + middle_idx,
        [&count](int v1, int v2) -> bool {
            HPX_TEST_NEQ(v1, v2);
            ++count;
            return v1 != v2;
        }));

    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    std::uniform_int_distribution<> dis(
        0, static_cast<int>((c.size() >> 1) - 1));
    std::uniform_int_distribution<> dist(0, static_cast<int>(c.size() - 1));

    std::size_t middle_idx = dis(gen);
    auto middle = std::begin(c) + static_cast<std::ptrdiff_t>(middle_idx);
    std::iota(std::begin(c), middle, static_cast<int>(dist(gen)));
    std::fill(middle, std::end(c), -1);

    hpx::remove_copy_if(policy, iterator(std::begin(c)), iterator(std::end(c)),
        std::begin(d), [](int i) { return i < 0; });

    std::size_t count = 0;
    HPX_TEST(std::equal(
        std::begin(c), middle, std::begin(d), [&count](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST(std::equal(middle, std::end(c), std::begin(d) + middle_idx,
        [&count](int v1, int v2) -> bool {
            HPX_TEST_NEQ(v1, v2);
            ++count;
            return v1 != v2;
        }));

    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    std::uniform_int_distribution<> dis(
        0, static_cast<int>((c.size() >> 1) - 1));
    std::uniform_int_distribution<> dist(0, static_cast<int>(c.size() - 1));

    std::size_t middle_idx = dis(gen);
    auto middle = std::begin(c) + static_cast<std::ptrdiff_t>(middle_idx);
    std::iota(std::begin(c), middle, static_cast<int>(dist(gen)));
    std::fill(middle, std::end(c), -1);

    auto f = hpx::remove_copy_if(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), [](int i) { return i < 0; });
    f.wait();

    std::size_t count = 0;
    HPX_TEST(std::equal(
        std::begin(c), middle, std::begin(d), [&count](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST(std::equal(middle, std::end(c), std::begin(d) + middle_idx,
        [&count](int v1, int v2) -> bool {
            HPX_TEST_NEQ(v1, v2);
            ++count;
            return v1 != v2;
        }));

    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_outiter(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::vector<int> d(0);
    std::uniform_int_distribution<> dis(
        0, static_cast<int>((c.size() >> 1) - 1));
    std::uniform_int_distribution<> dist(0, static_cast<int>(c.size() - 1));

    std::size_t middle_idx = dis(gen);
    auto middle = std::begin(c) + static_cast<std::ptrdiff_t>(middle_idx);
    std::iota(std::begin(c), middle, static_cast<int>(dist(gen)));
    std::fill(middle, std::end(c), -1);

    hpx::remove_copy_if(policy, iterator(std::begin(c)), iterator(std::end(c)),
        std::back_inserter(d), [](int i) { return i < 0; });

    HPX_TEST(std::equal(
        std::begin(c), middle, std::begin(d), [](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            return v1 == v2;
        }));

    HPX_TEST_EQ(middle_idx, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_outiter_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    std::vector<int> d(0);
    std::uniform_int_distribution<> dis(
        0, static_cast<int>((c.size() >> 1) - 1));
    std::uniform_int_distribution<> dist(0, static_cast<int>(c.size() - 1));

    std::size_t middle_idx = dis(gen);
    auto middle = std::begin(c) + static_cast<std::ptrdiff_t>(middle_idx);
    std::iota(std::begin(c), middle, static_cast<int>(dist(gen)));
    std::fill(middle, std::end(c), -1);

    auto f =
        hpx::remove_copy_if(p, iterator(std::begin(c)), iterator(std::end(c)),
            std::back_inserter(d), [](int i) { return i < 0; });
    f.wait();

    HPX_TEST(std::equal(
        std::begin(c), middle, std::begin(d), [](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            return v1 == v2;
        }));

    HPX_TEST_EQ(middle_idx, d.size());
}

template <typename IteratorTag>
void test_remove_copy_if()
{
    using namespace hpx::execution;

    test_remove_copy_if(IteratorTag());
    test_remove_copy_if(seq, IteratorTag());
    test_remove_copy_if(par, IteratorTag());
    test_remove_copy_if(par_unseq, IteratorTag());

    test_remove_copy_if_async(seq(task), IteratorTag());
    test_remove_copy_if_async(par(task), IteratorTag());
}

void remove_copy_if_test()
{
    test_remove_copy_if<std::random_access_iterator_tag>();
    test_remove_copy_if<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_exception = false;
    try
    {
        hpx::remove_copy_if(policy, iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(d), [](std::size_t v) {
                return throw std::runtime_error("test"), v == 0;
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
void test_remove_copy_if_exception_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::remove_copy_if(p, iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(d), [](std::size_t v) {
                return throw std::runtime_error("test"), v == 0;
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
void test_remove_copy_if_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_remove_copy_if_exception(seq, IteratorTag());
    test_remove_copy_if_exception(par, IteratorTag());

    test_remove_copy_if_exception_async(seq(task), IteratorTag());
    test_remove_copy_if_exception_async(par(task), IteratorTag());
}

void remove_copy_if_exception_test()
{
    test_remove_copy_if_exception<std::random_access_iterator_tag>();
    test_remove_copy_if_exception<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_bad_alloc = false;
    try
    {
        hpx::remove_copy_if(policy, iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(d),
            [](std::size_t v) { return throw std::bad_alloc(), v; });

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
void test_remove_copy_if_bad_alloc_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), gen());

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::remove_copy_if(p, iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(d),
            [](std::size_t v) { return throw std::bad_alloc(), v; });

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
void test_remove_copy_if_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_remove_copy_if_bad_alloc(seq, IteratorTag());
    test_remove_copy_if_bad_alloc(par, IteratorTag());

    test_remove_copy_if_bad_alloc_async(seq(task), IteratorTag());
    test_remove_copy_if_bad_alloc_async(par(task), IteratorTag());
}

void remove_copy_if_bad_alloc_test()
{
    test_remove_copy_if_bad_alloc<std::random_access_iterator_tag>();
    test_remove_copy_if_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = std::random_device{}();
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    remove_copy_if_test();
    remove_copy_if_exception_test();
    remove_copy_if_bad_alloc_test();
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
