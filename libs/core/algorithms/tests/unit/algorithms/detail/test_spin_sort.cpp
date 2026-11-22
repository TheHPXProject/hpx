//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
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

void test2()
{
    std::uint64_t V1[300];
    std::less<std::uint64_t> comp;

    for (std::uint32_t i = 0; i < 200; ++i)
        V1[i] = i;

    spin_sort(&V1[0], &V1[200], comp);
    for (unsigned i = 1; i < 200; i++)
    {
        HPX_TEST(V1[i - 1] <= V1[i]);
    }

    for (std::uint32_t i = 0; i < 200; ++i)
        V1[i] = 199 - i;

    spin_sort(&V1[0], &V1[200], comp);
    for (unsigned i = 1; i < 200; i++)
    {
        HPX_TEST(V1[i - 1] <= V1[i]);
    }

    for (std::uint32_t i = 0; i < 300; ++i)
        V1[i] = 299 - i;

    spin_sort(&V1[0], &V1[300], comp);
    for (unsigned i = 1; i < 300; i++)
    {
        HPX_TEST(V1[i - 1] <= V1[i]);
    }

    for (std::uint32_t i = 0; i < 300; ++i)
        V1[i] = 88;

    spin_sort(&V1[0], &V1[300], comp);
    for (unsigned i = 1; i < 300; i++)
    {
        HPX_TEST(V1[i - 1] <= V1[i]);
    }
}

void test3()
{
    std::mt19937_64 my_rand(std::rand());
    constexpr std::uint32_t NMAX = NUMELEMS;

    std::vector<xk> V1, V2, V3;
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
    V3 = V2 = V1;
    spin_sort(V1.begin(), V1.end());

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

    spin_sort(V1.begin(), V1.end());
    for (unsigned i = 1; i < NElem; i++)
    {
        HPX_TEST(V1[i - 1] <= V1[i]);
    }

    V1.clear();
    for (std::uint32_t i = 0; i < NElem; ++i)
        V1.push_back(i);

    spin_sort(V1.begin(), V1.end());
    for (unsigned i = 1; i < NElem; i++)
    {
        HPX_TEST(V1[i - 1] <= V1[i]);
    }

    V1.clear();
    for (std::uint32_t i = 0; i < NElem; ++i)
        V1.push_back(NElem - i);

    spin_sort(V1.begin(), V1.end());
    for (unsigned i = 1; i < NElem; i++)
    {
        HPX_TEST(V1[i - 1] <= V1[i]);
    }

    V1.clear();
    for (std::uint32_t i = 0; i < NElem; ++i)
        V1.push_back(1000);

    spin_sort(V1.begin(), V1.end());
    for (unsigned i = 1; i < NElem; i++)
    {
        HPX_TEST(V1[i - 1] == V1[i]);
    }
}

void test5()
{
    typedef std::less<std::uint64_t> compare;
    constexpr std::uint32_t KMax = NUMELEMS;
    std::vector<std::uint64_t> K, M;
    std::mt19937_64 my_rand(std::rand());
    compare comp;

    for (std::uint32_t i = 0; i < KMax; ++i)
        K.push_back(my_rand());
    M = K;

    // spin_sort assumes that the memory is uninitialized
    std::uint64_t* Ptr = static_cast<std::uint64_t*>(
        std::malloc(sizeof(std::uint64_t) * (KMax >> 1)));
    if (Ptr == nullptr)
        throw std::bad_alloc();
    try
    {
        range<std::uint64_t*> Rbuf(Ptr, Ptr + (KMax >> 1));
        spin_sort(K.begin(), K.end(), comp, Rbuf);
    }
    catch (...)
    {
        std::free(Ptr);
        throw;
    }
    std::free(Ptr);

    std::stable_sort(M.begin(), M.end(), comp);
    for (unsigned i = 0; i < KMax; i++)
        HPX_TEST(M[i] == K[i]);
}

void test6()
{
    std::vector<std::uint64_t> V;

    for (std::uint32_t i = 0; i < 2083333; ++i)
        V.push_back(i);
    spin_sort(V.begin(), V.end(), std::less<std::uint64_t>());
    for (std::uint32_t i = 0; i < V.size(); ++i)
    {
        HPX_TEST(V[i] == i);
    }
}

void test7(void)
{
    typedef typename std::vector<std::uint64_t>::iterator iter_t;
    constexpr std::uint32_t NELEM = 41667;
    constexpr std::uint32_t N1 = (NELEM + 1) / 2;
    std::vector<std::uint64_t> A;

    for (std::uint32_t i = 0; i < 1000; ++i)
        A.push_back(0);
    for (std::uint32_t i = 0; i < NELEM; ++i)
        A.push_back(NELEM - i);
    for (std::uint32_t i = 0; i < 1000; ++i)
        A.push_back(0);

    spin_sort(A.begin() + 1000, A.begin() + (1000 + NELEM));

    for (iter_t it = A.begin() + 1000; it != A.begin() + (1000 + NELEM); ++it)
    {
        HPX_TEST((*(it - 1)) <= (*it));
    }
    HPX_TEST(A[998] == 0 && A[999] == 0 && A[1000 + NELEM] == 0 &&
        A[1001 + NELEM] == 0);

    //------------------------------------------------------------------------
    A.clear();
    for (std::uint32_t i = 0; i < 1000; ++i)
        A.push_back(999999999);
    for (std::uint32_t i = 0; i < NELEM; ++i)
        A.push_back(NELEM - i);
    for (std::uint32_t i = 0; i < 1000; ++i)
        A.push_back(999999999);

    spin_sort(A.begin() + 1000, A.begin() + (1000 + NELEM));

    for (iter_t it = A.begin() + 1001; it != A.begin() + (1000 + NELEM); ++it)
    {
        HPX_TEST((*(it - 1)) <= (*it));
    }
    HPX_TEST(A[998] == 999999999 && A[999] == 999999999 &&
        A[1000 + NELEM] == 999999999 && A[1001 + NELEM] == 999999999);

    //------------------------------------------------------------------------
    std::vector<std::uint64_t> B(N1 + 2000, 0);

    A.clear();
    range<std::uint64_t*> Rbuf(&B[1000], (&B[1000]) + N1);
    for (std::uint32_t i = 0; i < NELEM; ++i)
        A.push_back(NELEM - i);
    spin_sort(A.begin(), A.end(), std::less<std::uint64_t>(), Rbuf);
    for (iter_t it = A.begin() + 1; it != A.end(); ++it)
    {
        if ((*(it - 1)) > (*it))
            std::cout << "error 1\n";
    }
    HPX_TEST(
        B[998] == 0 && B[999] == 0 && B[1000 + N1] == 0 && B[1001 + N1] == 0);

    for (std::uint32_t i = 0; i < B.size(); ++i)
        B[i] = 999999999;
    A.clear();
    for (std::uint32_t i = 0; i < NELEM; ++i)
        A.push_back(NELEM - i);
    spin_sort(A.begin(), A.end(), std::less<std::uint64_t>(), Rbuf);

    for (iter_t it = A.begin() + 1; it != A.end(); ++it)
    {
        HPX_TEST((*(it - 1)) <= (*it));
    }
    HPX_TEST(B[998] == 999999999 && B[999] == 999999999 &&
        B[1000 + N1] == 999999999 && B[1001 + N1] == 999999999);
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    test2();
    test3();
    test4();
    test5();
    test6();
    test7();

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
