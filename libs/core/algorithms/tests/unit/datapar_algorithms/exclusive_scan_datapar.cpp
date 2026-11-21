//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/datapar.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "../algorithms/test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_exclusive_scan1(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::fill(std::begin(c), std::end(c), std::size_t(1));

    std::size_t const val(0);
    auto op = [](std::size_t v1, std::size_t v2) { return v1 + v2; };

    hpx::exclusive_scan(policy, iterator(std::begin(c)), iterator(std::end(c)),
        std::begin(d), val, op);

    // verify values
    std::vector<std::size_t> e(c.size());
    hpx::parallel::detail::sequential_exclusive_scan(
        std::begin(c), std::end(c), std::begin(e), val, op);

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(e)));
}

template <typename ExPolicy, typename IteratorTag>
void test_exclusive_scan1_async(ExPolicy p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::fill(std::begin(c), std::end(c), std::size_t(1));

    std::size_t const val(0);
    auto op = [](std::size_t v1, std::size_t v2) { return v1 + v2; };

    hpx::future<void> fut = hpx::exclusive_scan(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(d), val, op);
    fut.wait();

    // verify values
    std::vector<std::size_t> e(c.size());
    hpx::parallel::detail::sequential_exclusive_scan(
        std::begin(c), std::end(c), std::begin(e), val, op);

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(e)));
}

template <typename IteratorTag>
void test_exclusive_scan1()
{
    using namespace hpx::execution;

    test_exclusive_scan1(simd, IteratorTag());
    test_exclusive_scan1(par_simd, IteratorTag());

    test_exclusive_scan1_async(simd(task), IteratorTag());
    test_exclusive_scan1_async(par_simd(task), IteratorTag());
}

void exclusive_scan_test1()
{
    test_exclusive_scan1<std::random_access_iterator_tag>();
    test_exclusive_scan1<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_exclusive_scan2(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::fill(std::begin(c), std::end(c), std::size_t(1));

    std::size_t const val(0);
    hpx::exclusive_scan(policy, iterator(std::begin(c)), iterator(std::end(c)),
        std::begin(d), val);

    // verify values
    std::vector<std::size_t> e(c.size());
    hpx::parallel::detail::sequential_exclusive_scan(std::begin(c), std::end(c),
        std::begin(e), val, std::plus<std::size_t>());

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(e)));
}

template <typename ExPolicy, typename IteratorTag>
void test_exclusive_scan2_async(ExPolicy p, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::fill(std::begin(c), std::end(c), std::size_t(1));

    std::size_t const val(0);
    hpx::future<void> f = hpx::exclusive_scan(
        p, iterator(std::begin(c)), iterator(std::end(c)), std::begin(d), val);
    f.wait();

    // verify values
    std::vector<std::size_t> e(c.size());
    hpx::parallel::detail::sequential_exclusive_scan(std::begin(c), std::end(c),
        std::begin(e), val, std::plus<std::size_t>());

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(e)));
}

template <typename IteratorTag>
void test_exclusive_scan2()
{
    using namespace hpx::execution;

    test_exclusive_scan2(simd, IteratorTag());
    test_exclusive_scan2(par_simd, IteratorTag());

    test_exclusive_scan2_async(simd(task), IteratorTag());
    test_exclusive_scan2_async(par_simd(task), IteratorTag());
}

void exclusive_scan_test2()
{
    test_exclusive_scan2<std::random_access_iterator_tag>();
    test_exclusive_scan2<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_exclusive_scan_contiguous(ExPolicy policy, std::size_t size)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(size);
    std::vector<std::size_t> d(size);
    std::fill(std::begin(c), std::end(c), std::size_t(1));

    std::size_t const val(0);
    auto op = [](std::size_t v1, std::size_t v2) { return v1 + v2; };

    hpx::exclusive_scan(
        policy, std::begin(c), std::end(c), std::begin(d), val, op);

    std::vector<std::size_t> e(size);
    hpx::parallel::detail::sequential_exclusive_scan(
        std::begin(c), std::end(c), std::begin(e), val, op);

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(e)));

    // in-place
    std::vector<std::size_t> in_place = c;
    hpx::exclusive_scan(policy, std::begin(in_place), std::end(in_place),
        std::begin(in_place), val, op);
    HPX_TEST(
        std::equal(std::begin(in_place), std::end(in_place), std::begin(e)));
}

template <typename ExPolicy>
void test_exclusive_scan_contiguous_async(ExPolicy p, std::size_t size)
{
    std::vector<std::size_t> c(size);
    std::vector<std::size_t> d(size);
    std::fill(std::begin(c), std::end(c), std::size_t(1));

    std::size_t const val(0);
    auto op = [](std::size_t v1, std::size_t v2) { return v1 + v2; };

    hpx::future<void> fut = hpx::exclusive_scan(
        p, std::begin(c), std::end(c), std::begin(d), val, op);
    fut.wait();

    std::vector<std::size_t> e(size);
    hpx::parallel::detail::sequential_exclusive_scan(
        std::begin(c), std::end(c), std::begin(e), val, op);

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(e)));

    // in-place
    std::vector<std::size_t> in_place = c;
    hpx::future<void> fut_in_place =
        hpx::exclusive_scan(p, std::begin(in_place), std::end(in_place),
            std::begin(in_place), val, op);
    fut_in_place.wait();
    HPX_TEST(
        std::equal(std::begin(in_place), std::end(in_place), std::begin(e)));
}

void exclusive_scan_test_contiguous()
{
    using namespace hpx::execution;

    for (std::size_t size : {3, 10007})
    {
        test_exclusive_scan_contiguous(simd, size);
        test_exclusive_scan_contiguous(par_simd, size);

        test_exclusive_scan_contiguous_async(simd(task), size);
        test_exclusive_scan_contiguous_async(par_simd(task), size);
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    exclusive_scan_test1();
    exclusive_scan_test2();
    exclusive_scan_test_contiguous();

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
