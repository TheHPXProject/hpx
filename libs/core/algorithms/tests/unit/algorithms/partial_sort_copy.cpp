//----------------------------------------------------------------------------
/// \file partial_sort_copy.cpp
/// \brief Test program of the partial_sort_copy function
///
//  Copyright (c) 2020 Francisco Jose Tapia (fjtapia@gmail.com )
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#include <hpx/algorithm.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <list>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

#if defined(HPX_DEBUG)
#define NELEM 111
#else
#define NELEM 1007
#endif

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

template <typename IteratorTag>
void test_partial_sort_copy1(IteratorTag)
{
    std::list<std::uint64_t> l = {9, 7, 6, 8, 5, 4, 1, 2, 3};
    std::uint64_t v1[20], v2[20];

    //------------------------------------------------------------------------
    // Output size is smaller than input size
    //------------------------------------------------------------------------
    for (int i = 0; i < 20; ++i)
        v1[i] = v2[i] = 999;

    hpx::partial_sort_copy(l.begin(), l.end(), &v1[0], &v1[4]);
    std::partial_sort_copy(l.begin(), l.end(), &v2[0], &v2[4]);

    for (int i = 0; i < 4; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
    for (int i = 4; i < 20; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };

    //------------------------------------------------------------------------
    // Output size is equal than input size
    //------------------------------------------------------------------------
    for (int i = 0; i < 20; ++i)
        v1[i] = v2[i] = 999;

    hpx::partial_sort_copy(l.begin(), l.end(), &v1[0], &v1[9]);
    std::partial_sort_copy(l.begin(), l.end(), &v2[0], &v2[9]);

    for (int i = 0; i < 9; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
    for (int i = 9; i < 20; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };

    //------------------------------------------------------------------------
    // Output size is greater than input size
    //------------------------------------------------------------------------
    for (int i = 0; i < 20; ++i)
        v1[i] = v2[i] = 999;

    hpx::partial_sort_copy(l.begin(), l.end(), &v1[0], &v1[20]);
    std::partial_sort_copy(l.begin(), l.end(), &v2[0], &v2[20]);

    for (int i = 0; i < 9; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
    for (int i = 9; i < 20; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
}

template <typename ExPolicy, typename IteratorTag>
void test_partial_sort_copy1(ExPolicy policy, IteratorTag)
{
    std::list<std::uint64_t> l = {9, 7, 6, 8, 5, 4, 1, 2, 3};
    std::uint64_t v1[20], v2[20];

    //------------------------------------------------------------------------
    // Output size is smaller than input size
    //------------------------------------------------------------------------
    for (int i = 0; i < 20; ++i)
        v1[i] = v2[i] = 999;

    hpx::partial_sort_copy(policy, l.begin(), l.end(), &v1[0], &v1[4]);
    std::partial_sort_copy(l.begin(), l.end(), &v2[0], &v2[4]);

    for (int i = 0; i < 4; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
    for (int i = 4; i < 20; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };

    //------------------------------------------------------------------------
    // Output size is equal than input size
    //------------------------------------------------------------------------
    for (int i = 0; i < 20; ++i)
        v1[i] = v2[i] = 999;

    hpx::partial_sort_copy(policy, l.begin(), l.end(), &v1[0], &v1[9]);
    std::partial_sort_copy(l.begin(), l.end(), &v2[0], &v2[9]);

    for (int i = 0; i < 9; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
    for (int i = 9; i < 20; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };

    //------------------------------------------------------------------------
    // Output size is greater than input size
    //------------------------------------------------------------------------
    for (int i = 0; i < 20; ++i)
        v1[i] = v2[i] = 999;

    hpx::partial_sort_copy(policy, l.begin(), l.end(), &v1[0], &v1[20]);
    std::partial_sort_copy(l.begin(), l.end(), &v2[0], &v2[20]);

    for (int i = 0; i < 9; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
    for (int i = 9; i < 20; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
}

template <typename ExPolicy, typename IteratorTag>
void test_partial_sort_copy1_async(ExPolicy p, IteratorTag)
{
    std::list<std::uint64_t> l = {9, 7, 6, 8, 5, 4, 1, 2, 3};
    std::vector<std::uint64_t> v1(20);
    std::vector<std::uint64_t> v2(20);

    //------------------------------------------------------------------------
    // Output size is smaller than input size
    //------------------------------------------------------------------------
    for (int i = 0; i < 20; ++i)
        v1[i] = v2[i] = 999;

    auto f = hpx::partial_sort_copy(
        p, l.begin(), l.end(), v1.begin(), v1.begin() + 4);
    f.wait();
    std::partial_sort_copy(l.begin(), l.end(), v2.begin(), v2.begin() + 4);

    for (int i = 0; i < 4; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
    for (int i = 4; i < 20; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };

    //------------------------------------------------------------------------
    // Output size is equal than input size
    //------------------------------------------------------------------------
    for (int i = 0; i < 20; ++i)
        v1[i] = v2[i] = 999;

    auto f1 = hpx::partial_sort_copy(
        p, l.begin(), l.end(), v1.begin(), v1.begin() + 9);
    f1.wait();
    std::partial_sort_copy(l.begin(), l.end(), v2.begin(), v2.begin() + 9);

    for (int i = 0; i < 9; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
    for (int i = 9; i < 20; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };

    //------------------------------------------------------------------------
    // Output size is greater than input size
    //------------------------------------------------------------------------
    for (int i = 0; i < 20; ++i)
        v1[i] = v2[i] = 999;

    auto f2 =
        hpx::partial_sort_copy(p, l.begin(), l.end(), v1.begin(), v1.end());
    f2.wait();
    std::partial_sort_copy(l.begin(), l.end(), v2.begin(), v2.end());

    for (int i = 0; i < 9; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
    for (int i = 9; i < 20; ++i)
    {
        HPX_TEST(v1[i] == v2[i]);
    };
}

template <typename IteratorTag>
void test_partial_sort_copy1()
{
    using namespace hpx::execution;
    test_partial_sort_copy1(IteratorTag());
    test_partial_sort_copy1(seq, IteratorTag());
    test_partial_sort_copy1(par, IteratorTag());
    test_partial_sort_copy1(par_unseq, IteratorTag());

    test_partial_sort_copy1_async(seq(task), IteratorTag());
    test_partial_sort_copy1_async(par(task), IteratorTag());
}

void partial_sort_test1()
{
    test_partial_sort_copy1<std::random_access_iterator_tag>();
    test_partial_sort_copy1<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_partial_sort_copy2(IteratorTag)
{
    using compare_t = std::less<std::uint64_t>;
    std::list<std::uint64_t> lst;
    std::vector<std::uint64_t> A, B;
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
        B.emplace_back(0);
    }

    std::shuffle(A.begin(), A.end(), gen);
    lst.insert(lst.end(), A.begin(), A.end());

    for (std::uint64_t i = 0; i <= NELEM; ++i)
    {
        A = B;

        hpx::partial_sort_copy(lst.begin(), lst.end(), A.begin(),
            A.begin() + static_cast<std::ptrdiff_t>(i), compare_t());

        for (std::uint64_t j = 0; j < i; ++j)
        {
            HPX_ASSERT(A[j] == j);
        };
    };
}

template <typename ExPolicy, typename IteratorTag>
void test_partial_sort_copy2(ExPolicy policy, IteratorTag)
{
    using compare_t = std::less<std::uint64_t>;
    std::list<std::uint64_t> lst;
    std::vector<std::uint64_t> A, B;
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
        B.emplace_back(0);
    }

    std::shuffle(A.begin(), A.end(), gen);
    lst.insert(lst.end(), A.begin(), A.end());

    for (std::uint64_t i = 0; i <= NELEM; ++i)
    {
        A = B;

        hpx::partial_sort_copy(policy, lst.begin(), lst.end(), A.begin(),
            A.begin() + static_cast<std::ptrdiff_t>(i), compare_t());

        for (std::uint64_t j = 0; j < i; ++j)
        {
            HPX_ASSERT(A[j] == j);
        };
    };
}

template <typename ExPolicy, typename IteratorTag>
void test_partial_sort_copy2_async(ExPolicy p, IteratorTag)
{
    using compare_t = std::less<std::uint64_t>;
    std::list<std::uint64_t> lst;
    std::vector<std::uint64_t> A, B;
    A.reserve(NELEM);
    B.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
        B.emplace_back(0);
    }

    std::shuffle(A.begin(), A.end(), gen);
    lst.insert(lst.end(), A.begin(), A.end());

    for (std::uint64_t i = 0; i <= NELEM; ++i)
    {
        A = B;

        auto f = hpx::partial_sort_copy(p, lst.begin(), lst.end(), A.begin(),
            A.begin() + static_cast<std::ptrdiff_t>(i), compare_t());
        f.wait();

        for (std::uint64_t j = 0; j < i; ++j)
        {
            HPX_ASSERT(A[j] == j);
        };
    };
}

template <typename IteratorTag>
void test_partial_sort_copy2()
{
    using namespace hpx::execution;
    test_partial_sort_copy2(IteratorTag());
    test_partial_sort_copy2(seq, IteratorTag());
    test_partial_sort_copy2(par, IteratorTag());
    test_partial_sort_copy2(par_unseq, IteratorTag());

    test_partial_sort_copy2_async(seq(task), IteratorTag());
    test_partial_sort_copy2_async(par(task), IteratorTag());
}

void partial_sort_test2()
{
    test_partial_sort_copy2<std::random_access_iterator_tag>();
    test_partial_sort_copy2<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_partial_sort_copy3(IteratorTag)
{
    using compare_t = std::less<std::uint64_t>;
    std::list<std::uint64_t> lst;
    std::mt19937 my_rand(0);
    std::vector<std::uint64_t> A, B, C;
    A.reserve(NELEM);
    B.reserve(NELEM);
    C.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
        B.emplace_back(0);
    }

    std::shuffle(A.begin(), A.end(), my_rand);
    lst.insert(lst.end(), A.begin(), A.end());

    uint32_t const STEP = NELEM / 20;

    for (std::uint64_t i = 0; i <= NELEM; i += STEP)
    {
        A = B;
        hpx::partial_sort_copy(lst.begin(), lst.end(), A.begin(),
            A.begin() + static_cast<std::ptrdiff_t>(i), compare_t());

        for (std::uint64_t j = 0; j < i; ++j)
        {
            HPX_ASSERT(A[j] == j);
        };
    };
}

template <typename ExPolicy, typename IteratorTag>
void test_partial_sort_copy3(ExPolicy policy, IteratorTag)
{
    using compare_t = std::less<std::uint64_t>;
    std::list<std::uint64_t> lst;
    std::mt19937 my_rand(0);
    std::vector<std::uint64_t> A, B, C;
    A.reserve(NELEM);
    B.reserve(NELEM);
    C.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
        B.emplace_back(0);
    }

    std::shuffle(A.begin(), A.end(), my_rand);
    lst.insert(lst.end(), A.begin(), A.end());

    uint32_t const STEP = NELEM / 20;

    for (std::uint64_t i = 0; i <= NELEM; i += STEP)
    {
        A = B;
        hpx::partial_sort_copy(policy, lst.begin(), lst.end(), A.begin(),
            A.begin() + static_cast<std::ptrdiff_t>(i), compare_t());

        for (std::uint64_t j = 0; j < i; ++j)
        {
            HPX_ASSERT(A[j] == j);
        };
    };
}

template <typename ExPolicy, typename IteratorTag>
void test_partial_sort_copy3_async(ExPolicy p, IteratorTag)
{
    using compare_t = std::less<std::uint64_t>;
    std::list<std::uint64_t> lst;
    std::mt19937 my_rand(0);
    std::vector<std::uint64_t> A, B, C;
    A.reserve(NELEM);
    B.reserve(NELEM);
    C.reserve(NELEM);

    for (std::uint64_t i = 0; i < NELEM; ++i)
    {
        A.emplace_back(i);
        B.emplace_back(0);
    }

    std::shuffle(A.begin(), A.end(), my_rand);
    lst.insert(lst.end(), A.begin(), A.end());

    uint32_t const STEP = NELEM / 20;

    for (std::uint64_t i = 0; i <= NELEM; i += STEP)
    {
        A = B;
        auto f = hpx::partial_sort_copy(p, lst.begin(), lst.end(), A.begin(),
            A.begin() + static_cast<std::ptrdiff_t>(i), compare_t());
        f.wait();

        for (std::uint64_t j = 0; j < i; ++j)
        {
            HPX_ASSERT(A[j] == j);
        };
    };
}

template <typename IteratorTag>
void test_partial_sort_copy3()
{
    using namespace hpx::execution;
    test_partial_sort_copy3(IteratorTag());
    test_partial_sort_copy3(seq, IteratorTag());
    test_partial_sort_copy3(par, IteratorTag());
    test_partial_sort_copy3(par_unseq, IteratorTag());

    test_partial_sort_copy3_async(seq(task), IteratorTag());
    test_partial_sort_copy3_async(par(task), IteratorTag());
}

void partial_sort_test3()
{
    test_partial_sort_copy3<std::random_access_iterator_tag>();
    test_partial_sort_copy3<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    partial_sort_test1();
    partial_sort_test2();
    partial_sort_test3();

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
