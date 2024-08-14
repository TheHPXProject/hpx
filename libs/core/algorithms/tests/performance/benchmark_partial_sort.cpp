//  Copyright (c) 2020 Francisco Jose Tapia (fjtapia@gmail.com )
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

// Compare the speed with the implementation of the compiler
void function01(void)
{
    typedef std::less<std::uint64_t> compare_t;
#if defined(HPX_DEBUG)
    constexpr std::uint32_t NELEM = 100;
#else
    constexpr std::uint32_t NELEM = 10000;
#endif

    std::vector<uint64_t> A, B;
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
    }
    std::shuffle(A.begin(), A.end(), gen);

    auto start = std::chrono::high_resolution_clock::now();
    for (std::uint64_t i = 0; i <= NELEM; ++i)
    {
        B = A;
        hpx::partial_sort(B.begin(), B.begin() + static_cast<std::ptrdiff_t>(i),
            B.end(), compare_t());
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<long unsigned, std::nano> nanotime1 = end - start;
    std::cout << "hpx::partial_sort :" << (nanotime1.count() / 1000000)
              << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (std::uint64_t i = 0; i <= NELEM; ++i)
    {
        B = A;
        std::partial_sort(B.begin(), B.begin() + static_cast<std::ptrdiff_t>(i),
            B.end(), compare_t());
    }
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<long unsigned, std::nano> nanotime2 = end - start;
    std::cout << "std::partial_sort           :"
              << (nanotime2.count() / 1000000) << std::endl;
}

void function02(void)
{
#if defined(HPX_DEBUG)
    constexpr std::uint32_t NELEM = 100000;
#else
    constexpr std::uint32_t NELEM = 10000000;
#endif

    std::less<std::uint64_t> comp;
    std::vector<std::uint64_t> A, B;
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
    }
    std::shuffle(A.begin(), A.end(), gen);

    // std::sort
    B = A;
    auto start = std::chrono::high_resolution_clock::now();
    std::sort(B.begin(), B.end(), comp);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<long unsigned, std::nano> nanotime1 = end - start;
    std::cout << "std::sort                    :"
              << (nanotime1.count() / 1000000) << std::endl;

    // heap sort
    B = A;
    start = std::chrono::high_resolution_clock::now();
    std::make_heap(B.begin(), B.end(), comp);
    std::sort_heap(B.begin(), B.end(), comp);
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<long unsigned, std::nano> nanotime2 = end - start;
    std::cout << "std::heap_sort               :"
              << (nanotime2.count() / 1000000) << std::endl;

    // hpx::partial_sort
    B = A;
    start = std::chrono::high_resolution_clock::now();
    hpx::partial_sort(B.begin(), B.end(), B.end(), comp);
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<long unsigned, std::nano> nanotime4 = end - start;
    std::cout << "hpx::partial_sort  :" << (nanotime4.count() / 1000000)
              << std::endl;

    // std::partial_sort
    B = A;
    start = std::chrono::high_resolution_clock::now();
    std::partial_sort(B.begin(), B.end(), B.end(), comp);
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<long unsigned, std::nano> nanotime5 = end - start;
    std::cout << "std::partial_sort            :"
              << (nanotime5.count() / 1000000) << std::endl;

    std::cout << "\n";
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    hpx::util::perftests_init(vm, "benchmark_partial_sort");

    typedef std::less<std::uint64_t> compare_t;

    std::uint32_t NELEM = 1000;

    std::vector<uint64_t> A, B;
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
    }
    std::shuffle(A.begin(), A.end(), gen);
    
    hpx::util::perftests_report("hpx::partial_sort, size: " + std::to_string(NELEM) + ", step: " + std::to_string(1) , "seq", 100, [&] {
        for (uint32_t i = 0; i < NELEM; i++)
        {
            B = A;
            hpx::partial_sort(B.begin(), B.begin() + i, B.end(), compare_t());
        }
    });

    NELEM = 100000;

    A.clear();
    B.clear();
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
    }
    std::shuffle(A.begin(), A.end(), gen);

    hpx::util::perftests_report("hpx::partial_sort, size: " + std::to_string(NELEM), "seq", 100, [&] {
        B = A;
        hpx::partial_sort(B.begin(), B.end(), B.end(), compare_t());
    });

    hpx::util::perftests_print_times();

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

    hpx::util::perftests_cfg(desc_commandline);

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
