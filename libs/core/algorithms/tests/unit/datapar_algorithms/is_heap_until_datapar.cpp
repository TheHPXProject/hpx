//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/datapar.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "../algorithms/is_heap_tests.hpp"
#include "../algorithms/test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag, typename DataType>
void test_is_heap_until()
{
    using namespace hpx::execution;

    test_is_heap(simd, IteratorTag(), DataType(), false);
    test_is_heap(par_simd, IteratorTag(), DataType(), false);

    test_is_heap_async(simd(task), IteratorTag(), DataType(), false);
    test_is_heap_async(par_simd(task), IteratorTag(), DataType(), false);
}

void is_heap_until_test()
{
    test_is_heap_until<std::random_access_iterator_tag, int>();
    test_is_heap_until<std::random_access_iterator_tag, double>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_is_heap_until_contiguous(ExPolicy policy, std::size_t size)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    // Valid heap
    {
        std::vector<int> c(size);
        std::iota(std::begin(c), std::end(c), 0);
        std::make_heap(std::begin(c), std::end(c));

        auto result = hpx::is_heap_until(policy, std::begin(c), std::end(c));
        auto solution = std::is_heap_until(std::begin(c), std::end(c));

        HPX_TEST(result == solution);
        HPX_TEST(result == std::end(c));
    }

    // Invalid heap (break at various positions if size >= 3)
    if (size >= 3)
    {
        for (std::size_t break_pos : {1ul, size / 2, size - 1})
        {
            std::vector<int> c(size);
            std::iota(std::begin(c), std::end(c), 0);
            std::make_heap(std::begin(c), std::end(c));

            // Break heap property by making child greater than parent
            c[break_pos] = 999999;

            auto result =
                hpx::is_heap_until(policy, std::begin(c), std::end(c));
            auto solution = std::is_heap_until(std::begin(c), std::end(c));

            HPX_TEST(result == solution);
            HPX_TEST(result != std::end(c));
        }
    }

    // Custom predicate (min-heap using std::greater<int>)
    {
        std::vector<int> c(size);
        std::iota(std::begin(c), std::end(c), 0);
        std::make_heap(std::begin(c), std::end(c), std::greater<int>());

        auto result = hpx::is_heap_until(
            policy, std::begin(c), std::end(c), std::greater<int>());
        auto solution =
            std::is_heap_until(std::begin(c), std::end(c), std::greater<int>());

        HPX_TEST(result == solution);
        HPX_TEST(result == std::end(c));
    }
}

template <typename ExPolicy>
void test_is_heap_until_contiguous_async(ExPolicy p, std::size_t size)
{
    using iterator = std::vector<int>::iterator;

    // Valid heap
    {
        std::vector<int> c(size);
        std::iota(std::begin(c), std::end(c), 0);
        std::make_heap(std::begin(c), std::end(c));

        hpx::future<iterator> fut =
            hpx::is_heap_until(p, std::begin(c), std::end(c));
        iterator result = fut.get();
        auto solution = std::is_heap_until(std::begin(c), std::end(c));

        HPX_TEST(result == solution);
        HPX_TEST(result == std::end(c));
    }

    // Invalid heap
    if (size >= 3)
    {
        std::vector<int> c(size);
        std::iota(std::begin(c), std::end(c), 0);
        std::make_heap(std::begin(c), std::end(c));
        c[size / 2] = 999999;

        hpx::future<iterator> fut =
            hpx::is_heap_until(p, std::begin(c), std::end(c));
        iterator result = fut.get();
        auto solution = std::is_heap_until(std::begin(c), std::end(c));

        HPX_TEST(result == solution);
        HPX_TEST(result != std::end(c));
    }
}

void is_heap_until_test_contiguous()
{
    using namespace hpx::execution;

    for (std::size_t size : {0ul, 1ul, 3ul, 7ul, 16ul, 10007ul})
    {
        test_is_heap_until_contiguous(simd, size);
        test_is_heap_until_contiguous(par_simd, size);

        test_is_heap_until_contiguous_async(simd(task), size);
        test_is_heap_until_contiguous_async(par_simd(task), size);
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

    is_heap_until_test();
    is_heap_until_test_contiguous();

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
