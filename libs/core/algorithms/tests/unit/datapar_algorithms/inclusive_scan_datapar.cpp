//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/datapar.hpp>
#include <hpx/init.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "../algorithms/inclusive_scan_tests.hpp"

template <typename IteratorTag>
void test_inclusive_scan1()
{
    using namespace hpx::execution;

    test_inclusive_scan1(simd, IteratorTag());
    test_inclusive_scan1(par_simd, IteratorTag());

    test_inclusive_scan1_async(simd(task), IteratorTag());
    test_inclusive_scan1_async(par_simd(task), IteratorTag());
}

void inclusive_scan_test1()
{
    test_inclusive_scan1<std::random_access_iterator_tag>();
    test_inclusive_scan1<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_inclusive_scan2()
{
    using namespace hpx::execution;

    test_inclusive_scan2(simd, IteratorTag());
    test_inclusive_scan2(par_simd, IteratorTag());

    test_inclusive_scan2_async(simd(task), IteratorTag());
    test_inclusive_scan2_async(par_simd(task), IteratorTag());
}

void inclusive_scan_test2()
{
    test_inclusive_scan2<std::random_access_iterator_tag>();
    test_inclusive_scan2<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_inclusive_scan3()
{
    using namespace hpx::execution;

    test_inclusive_scan3(simd, IteratorTag());
    test_inclusive_scan3(par_simd, IteratorTag());

    test_inclusive_scan3_async(simd(task), IteratorTag());
    test_inclusive_scan3_async(par_simd(task), IteratorTag());
}

void inclusive_scan_test3()
{
    test_inclusive_scan3<std::random_access_iterator_tag>();
    test_inclusive_scan3<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_inclusive_scan_contiguous(ExPolicy policy, std::size_t size)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(size);
    std::vector<std::size_t> d(size);
    std::fill(std::begin(c), std::end(c), std::size_t(1));

    std::size_t const val(0);
    auto op = [](std::size_t v1, std::size_t v2) { return v1 + v2; };

    hpx::inclusive_scan(
        policy, std::begin(c), std::end(c), std::begin(d), op, val);

    std::vector<std::size_t> e(size);
    hpx::parallel::detail::sequential_inclusive_scan(
        std::begin(c), std::end(c), std::begin(e), val, op);

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(e)));

    // in-place
    std::vector<std::size_t> in_place = c;
    hpx::inclusive_scan(policy, std::begin(in_place), std::end(in_place),
        std::begin(in_place), op, val);
    HPX_TEST(
        std::equal(std::begin(in_place), std::end(in_place), std::begin(e)));
}

template <typename ExPolicy>
void test_inclusive_scan_contiguous_async(ExPolicy p, std::size_t size)
{
    std::vector<std::size_t> c(size);
    std::vector<std::size_t> d(size);
    std::fill(std::begin(c), std::end(c), std::size_t(1));

    std::size_t const val(0);
    auto op = [](std::size_t v1, std::size_t v2) { return v1 + v2; };

    hpx::future<void> fut = hpx::inclusive_scan(
        p, std::begin(c), std::end(c), std::begin(d), op, val);
    fut.wait();

    std::vector<std::size_t> e(size);
    hpx::parallel::detail::sequential_inclusive_scan(
        std::begin(c), std::end(c), std::begin(e), val, op);

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(e)));

    // in-place
    std::vector<std::size_t> in_place = c;
    hpx::future<void> fut_in_place =
        hpx::inclusive_scan(p, std::begin(in_place), std::end(in_place),
            std::begin(in_place), op, val);
    fut_in_place.wait();
    HPX_TEST(
        std::equal(std::begin(in_place), std::end(in_place), std::begin(e)));
}

void inclusive_scan_test_contiguous()
{
    using namespace hpx::execution;

    for (std::size_t size : {3, 10007})
    {
        test_inclusive_scan_contiguous(simd, size);
        test_inclusive_scan_contiguous(par_simd, size);

        test_inclusive_scan_contiguous_async(simd(task), size);
        test_inclusive_scan_contiguous_async(par_simd(task), size);
    }
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    inclusive_scan_test1();
    inclusive_scan_test2();
    inclusive_scan_test3();
    inclusive_scan_test_contiguous();

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
