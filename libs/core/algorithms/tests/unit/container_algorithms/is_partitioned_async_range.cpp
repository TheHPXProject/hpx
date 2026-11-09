//  Copyright (c) 2025 Hartmut Kaiser
//  Copyright (c) 2020 ETH Zurich
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
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(0, 99);

template <typename ExPolicy, typename IteratorTag>
void test_partitioned1_async(ExPolicy p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);

    // fill first half of array with even numbers and second half
    // with odd numbers
    std::fill_n(std::begin(c), c.size() / 2, 2 * (dis(gen)));
    std::fill(std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        std::end(c), 2 * (dis(gen)) + 1);

    hpx::future<bool> f =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c)),
            iterator(std::end(c)), [](std::size_t n) { return n % 2 == 0; });
    f.wait();

    HPX_TEST(f.get());
}

template <typename ExPolicy>
void test_partitioned1_async(ExPolicy p)
{
    std::vector<std::size_t> c(10007);

    // fill first half of array with even numbers and second half
    // with odd numbers
    std::fill_n(std::begin(c), c.size() / 2, 2 * (dis(gen)));
    std::fill(std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        std::end(c), 2 * (dis(gen)) + 1);

    hpx::future<bool> f = hpx::ranges::is_partitioned(
        p, c, [](std::size_t n) { return n % 2 == 0; });
    f.wait();

    HPX_TEST(f.get());
}

template <typename IteratorTag>
void test_partitioned1()
{
    using namespace hpx::execution;
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_partitioned1_async(seq(task), IteratorTag());
        test_partitioned1_async(par(task), IteratorTag());
    }
}

void partitioned_test1()
{
    using namespace hpx::execution;
    test_partitioned1_async(seq(task));
    test_partitioned1_async(par(task));
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_partitioned2_async(ExPolicy p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c_odd(10007);
    // fill all of the array with odds
    std::fill(std::begin(c_odd), std::end(c_odd), 2 * (dis(gen)) + 1);
    std::vector<std::size_t> c_even(10007);
    // fill all of the array with evens
    std::fill(std::begin(c_even), std::end(c_even), 2 * (dis(gen)));

    hpx::future<bool> f_odd = hpx::ranges::is_partitioned(p,
        iterator(std::begin(c_odd)), iterator(std::end(c_odd)),
        [](std::size_t n) { return n % 2 == 0; });
    hpx::future<bool> f_even = hpx::ranges::is_partitioned(p,
        iterator(std::begin(c_even)), iterator(std::end(c_even)),
        [](std::size_t n) { return n % 2 == 0; });

    f_odd.wait();
    HPX_TEST(f_odd.get());
    f_even.wait();
    HPX_TEST(f_even.get());
}

template <typename ExPolicy>
void test_partitioned2_async(ExPolicy p)
{
    std::vector<std::size_t> c_odd(10007);
    // fill all of the array with odds
    std::fill(std::begin(c_odd), std::end(c_odd), 2 * (dis(gen)) + 1);
    std::vector<std::size_t> c_even(10007);
    // fill all of the array with evens
    std::fill(std::begin(c_odd), std::end(c_odd), 2 * (dis(gen)));

    hpx::future<bool> f_odd = hpx::ranges::is_partitioned(
        p, c_odd, [](std::size_t n) { return n % 2 == 0; });
    hpx::future<bool> f_even = hpx::ranges::is_partitioned(
        p, c_even, [](std::size_t n) { return n % 2 == 0; });

    f_odd.wait();
    HPX_TEST(f_odd.get());
    f_even.wait();
    HPX_TEST(f_even.get());
}

template <typename IteratorTag>
void test_partitioned2()
{
    using namespace hpx::execution;
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_partitioned2_async(seq(task), IteratorTag());
        test_partitioned2_async(par(task), IteratorTag());
    }
}

void partitioned_test2()
{
    using namespace hpx::execution;
    test_partitioned2_async(seq(task));
    test_partitioned2_async(par(task));
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_partitioned3_async(ExPolicy p, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c_beg(10007);

    //  fill first half of array with even numbers and second half
    // with odd numbers
    std::fill_n(std::begin(c_beg), c_beg.size() / 2, 2 * (dis(gen)));
    std::fill(std::begin(c_beg) + static_cast<std::ptrdiff_t>(c_beg.size() / 2),
        std::end(c_beg), 2 * (dis(gen)) + 1);
    std::vector<size_t> c_end = c_beg;
    // add odd number to the beginning
    c_beg[0] -= 1;
    // add even number to end
    c_end[c_end.size() - 1] -= 1;

    hpx::future<bool> f_beg = hpx::ranges::is_partitioned(p,
        iterator(std::begin(c_beg)), iterator(std::end(c_beg)),
        [](std::size_t n) { return n % 2 == 0; });
    hpx::future<bool> f_end = hpx::ranges::is_partitioned(p,
        iterator(std::begin(c_end)), iterator(std::end(c_end)),
        [](std::size_t n) { return n % 2 == 0; });

    f_beg.wait();
    HPX_TEST(!f_beg.get());
    f_end.wait();
    HPX_TEST(!f_end.get());
}

template <typename ExPolicy>
void test_partitioned3_async(ExPolicy p)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c_beg(10007);

    // fill first half of array with even numbers and second half
    // with odd numbers
    std::fill_n(std::begin(c_beg), c_beg.size() / 2, 2 * (dis(gen)));
    std::fill(std::begin(c_beg) + static_cast<std::ptrdiff_t>(c_beg.size() / 2),
        std::end(c_beg), 2 * (dis(gen)) + 1);
    std::vector<size_t> c_end = c_beg;
    // add odd number to the beginning
    c_beg[0] -= 1;
    // add even number to end
    c_end[c_end.size() - 1] -= 1;

    hpx::future<bool> f_beg = hpx::ranges::is_partitioned(
        p, c_beg, [](std::size_t n) { return n % 2 == 0; });
    hpx::future<bool> f_end = hpx::ranges::is_partitioned(
        p, c_end, [](std::size_t n) { return n % 2 == 0; });

    f_beg.wait();
    HPX_TEST(!f_beg.get());
    f_end.wait();
    HPX_TEST(!f_end.get());
}

template <typename IteratorTag>
void test_partitioned3()
{
    using namespace hpx::execution;
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_partitioned3_async(seq(task), IteratorTag());
        test_partitioned3_async(par(task), IteratorTag());
    }
}

void partitioned_test3()
{
    using namespace hpx::execution;
    test_partitioned3_async(seq(task));
    test_partitioned3_async(par(task));
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_partitioned_async_exception(ExPolicy p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    // fill first half of array with even numbers and second half
    // with odd numbers
    std::fill_n(std::begin(c), c.size() / 2, 2 * (dis(gen)));
    std::fill(std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        std::end(c), 2 * (dis(gen)) + 1);

    bool caught_exception = false;
    try
    {
        hpx::future<bool> f = hpx::ranges::is_partitioned(p,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c), []() { throw std::runtime_error("test"); }),
            [](std::size_t n) { return n % 2 == 0; });
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
}

template <typename IteratorTag>
void test_partitioned_exception()
{
    using namespace hpx::execution;
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_partitioned_async_exception(seq(task), IteratorTag());
        test_partitioned_async_exception(par(task), IteratorTag());
    }
}

void partitioned_exception_test()
{
    test_partitioned_exception<std::random_access_iterator_tag>();
    test_partitioned_exception<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_partitioned_async_bad_alloc(ExPolicy p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    // fill first half of array with even numbers and second half
    // with odd numbers
    std::fill_n(std::begin(c), c.size() / 2, 2 * (dis(gen)));
    std::fill(std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        std::end(c), 2 * (dis(gen)) + 1);

    bool caught_bad_alloc = false;
    try
    {
        hpx::future<bool> f = hpx::ranges::is_partitioned(p,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c), []() { throw std::bad_alloc(); }),
            [](std::size_t n) { return n % 2 == 0; });

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
}

template <typename IteratorTag>
void test_partitioned_bad_alloc()
{
    using namespace hpx::execution;
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_partitioned_async_bad_alloc(seq(task), IteratorTag());
        test_partitioned_async_bad_alloc(par(task), IteratorTag());
    }
}

void partitioned_bad_alloc_test()
{
    test_partitioned_bad_alloc<std::random_access_iterator_tag>();
    test_partitioned_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    partitioned_test1();
    partitioned_test2();
    partitioned_test3();
    partitioned_exception_test();
    partitioned_bad_alloc_test();

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
