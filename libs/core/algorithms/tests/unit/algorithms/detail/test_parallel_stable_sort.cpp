//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#if defined(HPX_DEBUG)
#define NUMELEMS 50000
#else
#define NUMELEMS 500000
#endif

using namespace hpx::parallel::detail;
using hpx::execution::parallel_executor;
using hpx::parallel::util::range;

struct xk
{
    unsigned tail : 3;
    unsigned num : 24;

    bool operator<(xk A) const
    {
        return (unsigned) num < (unsigned) A.num;
    }
};

void test3()
{
    std::mt19937_64 my_rand(std::rand());

    constexpr std::uint32_t NMAX = NUMELEMS;

    std::vector<xk> V1, V2;
    V1.reserve(NMAX);
    for (std::uint32_t i = 0; i < 8; ++i)
    {
        for (std::uint32_t k = 0; k < NMAX; ++k)
        {
            auto const NM = static_cast<std::uint32_t>(my_rand());
            xk G;
            G.num = NM >> 3;
            G.tail = i;
            V1.push_back(G);
        }
    }
    V2 = V1;
    parallel_stable_sort(parallel_executor{}, V1.begin(), V1.end());
    std::stable_sort(V2.begin(), V2.end());

    HPX_TEST(V1.size() == V2.size());
    for (std::uint32_t i = 0; i < V1.size(); ++i)
    {
        HPX_TEST(V1[i].num == V2[i].num && V1[i].tail == V2[i].tail);
    }
}

void test4()
{
    constexpr std::uint32_t NElem = NUMELEMS;

    std::vector<std::uint64_t> V1;
    std::mt19937_64 my_rand(std::rand());

    for (std::uint32_t i = 0; i < NElem; ++i)
        V1.push_back(my_rand() % NElem);

    parallel_stable_sort(parallel_executor{}, V1.begin(), V1.end());
    for (unsigned i = 1; i < NElem; i++)
    {
        HPX_TEST(V1[i - 1] <= V1[i]);
    }

    V1.clear();
    for (std::uint32_t i = 0; i < NElem; ++i)
        V1.push_back(i);

    parallel_stable_sort(parallel_executor{}, V1.begin(), V1.end());
    for (unsigned i = 1; i < NElem; i++)
    {
        HPX_TEST(V1[i - 1] <= V1[i]);
    }

    V1.clear();
    for (std::uint32_t i = 0; i < NElem; ++i)
        V1.push_back(NElem - i);

    parallel_stable_sort(parallel_executor{}, V1.begin(), V1.end());
    for (unsigned i = 1; i < NElem; i++)
    {
        HPX_TEST(V1[i - 1] <= V1[i]);
    }

    V1.clear();
    for (std::uint32_t i = 0; i < NElem; ++i)
        V1.push_back(1000);

    parallel_stable_sort(parallel_executor{}, V1.begin(), V1.end());
    for (unsigned i = 1; i < NElem; i++)
    {
        HPX_TEST(V1[i - 1] == V1[i]);
    }
}

void test5()
{
    constexpr std::uint32_t NELEM = NUMELEMS;
    std::mt19937_64 my_rand(std::rand());

    std::vector<std::uint64_t> A, B;
    A.reserve(NELEM);

    for (unsigned i = 0; i < NELEM; i++)
        A.push_back(my_rand());
    B = A;

    parallel_stable_sort(parallel_executor{}, A.begin(), A.end());
    for (unsigned i = 0; i < (NELEM - 1); i++)
    {
        HPX_TEST(A[i] <= A[i + 1]);
    }
    std::stable_sort(B.begin(), B.end());
    HPX_TEST(A.size() == B.size());

    for (std::uint32_t i = 0; i < A.size(); ++i)
        HPX_TEST(A[i] == B[i]);
}

void test6(void)
{
    constexpr std::uint32_t NELEM = NUMELEMS;
    std::vector<std::uint64_t> A;
    A.reserve(NELEM);

    for (unsigned i = 0; i < NELEM; i++)
        A.push_back(NELEM - i);

    parallel_stable_sort(parallel_executor{}, A.begin(), A.end());
    for (unsigned i = 1; i < NELEM; i++)
    {
        HPX_TEST(A[i - 1] <= A[i]);
    }
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    test3();
    test4();
    test5();
    test6();

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

    // Initialize && run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
