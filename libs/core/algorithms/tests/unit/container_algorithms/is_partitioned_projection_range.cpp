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

using identity = hpx::identity;

struct add_one
{
    std::size_t operator()(std::size_t x)
    {
        return ++x;
    }
};

struct is_even
{
    bool operator()(std::size_t x)
    {
        return x % 2 == 0;
    }
};

struct is_odd
{
    bool operator()(std::size_t x)
    {
        return x % 2 == 1;
    }
};

template <typename ExPolicy, typename IteratorTag>
void test_partitioned1(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    //fill first half of array with even numbers and second half
    //with odd numbers
    std::fill(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        2 * (dis(gen)));
    std::fill(std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        std::end(c), 2 * (dis(gen)) + 1);

    bool parted1 = hpx::ranges::is_partitioned(policy, iterator(std::begin(c)),
        iterator(std::end(c)), is_even(), identity());
    bool parted2 = hpx::ranges::is_partitioned(policy, iterator(std::begin(c)),
        iterator(std::end(c)), is_odd(), identity());
    bool parted3 = hpx::ranges::is_partitioned(policy, iterator(std::begin(c)),
        iterator(std::end(c)), is_even(), add_one());
    bool parted4 = hpx::ranges::is_partitioned(policy, iterator(std::begin(c)),
        iterator(std::end(c)), is_odd(), add_one());

    HPX_TEST(parted1);
    HPX_TEST(!parted2);
    HPX_TEST(!parted3);
    HPX_TEST(parted4);
}

template <typename ExPolicy, typename IteratorTag>
void test_partitioned1_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c(10007);
    //fill first half of array with even numbers and second half
    //with odd numbers
    std::fill(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        2 * (dis(gen)));
    std::fill(std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        std::end(c), 2 * (dis(gen)) + 1);

    hpx::future<bool> f1 = hpx::ranges::is_partitioned(p,
        iterator(std::begin(c)), iterator(std::end(c)), is_even(), identity());
    f1.wait();
    hpx::future<bool> f2 = hpx::ranges::is_partitioned(p,
        iterator(std::begin(c)), iterator(std::end(c)), is_odd(), identity());
    f2.wait();
    hpx::future<bool> f3 = hpx::ranges::is_partitioned(p,
        iterator(std::begin(c)), iterator(std::end(c)), is_even(), add_one());
    f3.wait();
    hpx::future<bool> f4 = hpx::ranges::is_partitioned(
        p, iterator(std::begin(c)), iterator(std::end(c)), is_odd(), add_one());
    f4.wait();

    HPX_TEST(f1.get());
    HPX_TEST(!f2.get());
    HPX_TEST(!f3.get());
    HPX_TEST(f4.get());
}

template <typename ExPolicy>
void test_partitioned1(ExPolicy policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    //fill first half of array with even numbers and second half
    //with odd numbers
    std::fill(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        2 * (dis(gen)));
    std::fill(std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        std::end(c), 2 * (dis(gen)) + 1);

    bool parted1 =
        hpx::ranges::is_partitioned(policy, c, is_even(), identity());
    bool parted2 = hpx::ranges::is_partitioned(policy, c, is_odd(), identity());
    bool parted3 = hpx::ranges::is_partitioned(policy, c, is_even(), add_one());
    bool parted4 = hpx::ranges::is_partitioned(policy, c, is_odd(), add_one());

    HPX_TEST(parted1);
    HPX_TEST(!parted2);
    HPX_TEST(!parted3);
    HPX_TEST(parted4);
}

template <typename ExPolicy>
void test_partitioned1_async(ExPolicy p)
{
    std::vector<std::size_t> c(10007);
    //fill first half of array with even numbers and second half
    //with odd numbers
    std::fill(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        2 * (dis(gen)));
    std::fill(std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        std::end(c), 2 * (dis(gen)) + 1);

    hpx::future<bool> f1 =
        hpx::ranges::is_partitioned(p, c, is_even(), identity());
    f1.wait();
    hpx::future<bool> f2 =
        hpx::ranges::is_partitioned(p, c, is_odd(), identity());
    f2.wait();
    hpx::future<bool> f3 =
        hpx::ranges::is_partitioned(p, c, is_even(), add_one());
    f3.wait();
    hpx::future<bool> f4 =
        hpx::ranges::is_partitioned(p, c, is_odd(), add_one());
    f4.wait();

    HPX_TEST(f1.get());
    HPX_TEST(!f2.get());
    HPX_TEST(!f3.get());
    HPX_TEST(f4.get());
}

template <typename IteratorTag>
void test_partitioned1()
{
    using namespace hpx::execution;
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_partitioned1(seq, IteratorTag());
        test_partitioned1(par, IteratorTag());
        test_partitioned1(par_unseq, IteratorTag());

        test_partitioned1_async(seq(task), IteratorTag());
        test_partitioned1_async(par(task), IteratorTag());
    }
}

void partitioned_test1()
{
    test_partitioned1<std::random_access_iterator_tag>();
    test_partitioned1<std::forward_iterator_tag>();

    using namespace hpx::execution;
    test_partitioned1(seq);
    test_partitioned1(par);
    test_partitioned1(par_unseq);

    test_partitioned1_async(seq(task));
    test_partitioned1_async(par(task));
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_partitioned2(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c_odd(10007);
    //fill all of array with odds
    std::fill(std::begin(c_odd), std::end(c_odd), 2 * (dis(gen)) + 1);
    std::vector<std::size_t> c_even(10007);
    //fill all of array with evens
    std::fill(std::begin(c_even), std::end(c_even), 2 * (dis(gen)));

    bool parted_odd1 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_odd)),
            iterator(std::end(c_odd)), is_even(), identity());
    bool parted_odd2 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_odd)),
            iterator(std::end(c_odd)), is_odd(), identity());
    bool parted_odd3 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_odd)),
            iterator(std::end(c_odd)), is_even(), add_one());
    bool parted_odd4 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_odd)),
            iterator(std::end(c_odd)), is_odd(), add_one());
    bool parted_even1 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_even)),
            iterator(std::end(c_even)), is_even(), identity());
    bool parted_even2 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_even)),
            iterator(std::end(c_even)), is_odd(), identity());
    bool parted_even3 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_even)),
            iterator(std::end(c_even)), is_even(), add_one());
    bool parted_even4 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_even)),
            iterator(std::end(c_even)), is_odd(), add_one());

    HPX_TEST(parted_odd1);
    HPX_TEST(parted_odd2);
    HPX_TEST(parted_odd3);
    HPX_TEST(parted_odd4);
    HPX_TEST(parted_even1);
    HPX_TEST(parted_even2);
    HPX_TEST(parted_even3);
    HPX_TEST(parted_even4);
}

template <typename ExPolicy, typename IteratorTag>
void test_partitioned2_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c_odd(10007);
    //fill all of array with odds
    std::fill(std::begin(c_odd), std::end(c_odd), 2 * (dis(gen)) + 1);
    std::vector<std::size_t> c_even(10007);
    //fill all of array with evens
    std::fill(std::begin(c_even), std::end(c_even), 2 * (dis(gen)));

    hpx::future<bool> f_odd1 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_odd)),
            iterator(std::end(c_odd)), is_even(), identity());
    f_odd1.wait();
    hpx::future<bool> f_odd2 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_odd)),
            iterator(std::end(c_odd)), is_odd(), identity());
    f_odd2.wait();
    hpx::future<bool> f_odd3 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_odd)),
            iterator(std::end(c_odd)), is_even(), add_one());
    f_odd3.wait();
    hpx::future<bool> f_odd4 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_odd)),
            iterator(std::end(c_odd)), is_odd(), add_one());
    f_odd4.wait();
    hpx::future<bool> f_even1 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_even)),
            iterator(std::end(c_even)), is_even(), identity());
    f_even1.wait();
    hpx::future<bool> f_even2 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_even)),
            iterator(std::end(c_even)), is_odd(), identity());
    f_even2.wait();
    hpx::future<bool> f_even3 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_even)),
            iterator(std::end(c_even)), is_even(), add_one());
    f_even3.wait();
    hpx::future<bool> f_even4 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_even)),
            iterator(std::end(c_even)), is_odd(), add_one());
    f_even4.wait();

    HPX_TEST(f_odd1.get());
    HPX_TEST(f_odd2.get());
    HPX_TEST(f_odd3.get());
    HPX_TEST(f_odd4.get());
    HPX_TEST(f_even1.get());
    HPX_TEST(f_even2.get());
    HPX_TEST(f_even3.get());
    HPX_TEST(f_even4.get());
}

template <typename ExPolicy>
void test_partitioned2(ExPolicy policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c_odd(10007);
    //fill all of array with odds
    std::fill(std::begin(c_odd), std::end(c_odd), 2 * (dis(gen)) + 1);
    std::vector<std::size_t> c_even(10007);
    //fill all of array with evens
    std::fill(std::begin(c_even), std::end(c_even), 2 * (dis(gen)));

    bool parted_odd1 =
        hpx::ranges::is_partitioned(policy, c_odd, is_even(), identity());
    bool parted_odd2 =
        hpx::ranges::is_partitioned(policy, c_odd, is_odd(), identity());
    bool parted_odd3 =
        hpx::ranges::is_partitioned(policy, c_odd, is_even(), add_one());
    bool parted_odd4 =
        hpx::ranges::is_partitioned(policy, c_odd, is_odd(), add_one());
    bool parted_even1 =
        hpx::ranges::is_partitioned(policy, c_even, is_even(), identity());
    bool parted_even2 =
        hpx::ranges::is_partitioned(policy, c_even, is_odd(), identity());
    bool parted_even3 =
        hpx::ranges::is_partitioned(policy, c_even, is_even(), add_one());
    bool parted_even4 =
        hpx::ranges::is_partitioned(policy, c_even, is_odd(), add_one());

    HPX_TEST(parted_odd1);
    HPX_TEST(parted_odd2);
    HPX_TEST(parted_odd3);
    HPX_TEST(parted_odd4);
    HPX_TEST(parted_even1);
    HPX_TEST(parted_even2);
    HPX_TEST(parted_even3);
    HPX_TEST(parted_even4);
}

template <typename ExPolicy>
void test_partitioned2_async(ExPolicy p)
{
    std::vector<std::size_t> c_odd(10007);
    //fill all of array with odds
    std::fill(std::begin(c_odd), std::end(c_odd), 2 * (dis(gen)) + 1);
    std::vector<std::size_t> c_even(10007);
    //fill all of array with evens
    std::fill(std::begin(c_even), std::end(c_even), 2 * (dis(gen)));

    hpx::future<bool> f_odd1 =
        hpx::ranges::is_partitioned(p, c_odd, is_even(), identity());
    f_odd1.wait();
    hpx::future<bool> f_odd2 =
        hpx::ranges::is_partitioned(p, c_odd, is_odd(), identity());
    f_odd2.wait();
    hpx::future<bool> f_odd3 =
        hpx::ranges::is_partitioned(p, c_odd, is_even(), add_one());
    f_odd3.wait();
    hpx::future<bool> f_odd4 =
        hpx::ranges::is_partitioned(p, c_odd, is_odd(), add_one());
    f_odd4.wait();
    hpx::future<bool> f_even1 =
        hpx::ranges::is_partitioned(p, c_even, is_even(), identity());
    f_even1.wait();
    hpx::future<bool> f_even2 =
        hpx::ranges::is_partitioned(p, c_even, is_odd(), identity());
    f_even2.wait();
    hpx::future<bool> f_even3 =
        hpx::ranges::is_partitioned(p, c_even, is_even(), add_one());
    f_even3.wait();
    hpx::future<bool> f_even4 =
        hpx::ranges::is_partitioned(p, c_even, is_odd(), add_one());
    f_even4.wait();

    HPX_TEST(f_odd1.get());
    HPX_TEST(f_odd2.get());
    HPX_TEST(f_odd3.get());
    HPX_TEST(f_odd4.get());
    HPX_TEST(f_even1.get());
    HPX_TEST(f_even2.get());
    HPX_TEST(f_even3.get());
    HPX_TEST(f_even4.get());
}

template <typename IteratorTag>
void test_partitioned2()
{
    using namespace hpx::execution;
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_partitioned2(seq, IteratorTag());
        test_partitioned2(par, IteratorTag());
        test_partitioned2(par_unseq, IteratorTag());

        test_partitioned2_async(seq(task), IteratorTag());
        test_partitioned2_async(par(task), IteratorTag());
    }
}

void partitioned_test2()
{
    test_partitioned2<std::random_access_iterator_tag>();
    test_partitioned2<std::forward_iterator_tag>();

    using namespace hpx::execution;
    test_partitioned2(seq);
    test_partitioned2(par);
    test_partitioned2(par_unseq);

    test_partitioned2_async(seq(task));
    test_partitioned2_async(par(task));
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_partitioned3(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c_beg(10007);
    //fill first half of array with even numbers and second half
    //with odd numbers
    std::fill(std::begin(c_beg),
        std::begin(c_beg) +
            static_cast<std::ptrdiff_t>(
                static_cast<std::ptrdiff_t>(c_beg.size() / 2)),
        2 * (dis(gen)));
    std::fill(std::begin(c_beg) +
            static_cast<std::ptrdiff_t>(
                static_cast<std::ptrdiff_t>(c_beg.size() / 2)),
        std::end(c_beg), 2 * (dis(gen)) + 1);
    std::vector<size_t> c_end = c_beg;
    //add odd number to the beginning
    c_beg[0] -= 1;
    //add even number to end
    c_end[c_end.size() - 1] -= 1;

    bool parted_beg1 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_beg)),
            iterator(std::end(c_beg)), is_even(), identity());
    bool parted_beg2 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_beg)),
            iterator(std::end(c_beg)), is_odd(), identity());
    bool parted_beg3 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_beg)),
            iterator(std::end(c_beg)), is_even(), add_one());
    bool parted_beg4 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_beg)),
            iterator(std::end(c_beg)), is_odd(), add_one());
    bool parted_end1 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_end)),
            iterator(std::end(c_end)), is_even(), identity());
    bool parted_end2 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_end)),
            iterator(std::end(c_end)), is_odd(), identity());
    bool parted_end3 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_end)),
            iterator(std::end(c_end)), is_even(), add_one());
    bool parted_end4 =
        hpx::ranges::is_partitioned(policy, iterator(std::begin(c_end)),
            iterator(std::end(c_end)), is_odd(), add_one());

    HPX_TEST(!parted_beg1);
    HPX_TEST(!parted_beg2);
    HPX_TEST(!parted_beg3);
    HPX_TEST(!parted_beg4);
    HPX_TEST(!parted_end1);
    HPX_TEST(!parted_end2);
    HPX_TEST(!parted_end3);
    HPX_TEST(!parted_end4);
}

template <typename ExPolicy, typename IteratorTag>
void test_partitioned3_async(ExPolicy p, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c_beg(10007);
    //fill first half of array with even numbers and second half
    //with odd numbers
    std::fill(std::begin(c_beg),
        std::begin(c_beg) + static_cast<std::ptrdiff_t>(c_beg.size() / 2),
        2 * (dis(gen)));
    std::fill(std::begin(c_beg) + static_cast<std::ptrdiff_t>(c_beg.size() / 2),
        std::end(c_beg), 2 * (dis(gen)) + 1);
    std::vector<size_t> c_end = c_beg;
    //add odd number to the beginning
    c_beg[0] -= 1;
    //add even number to end
    c_end[c_end.size() - 1] -= 1;

    hpx::future<bool> f_beg1 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_beg)),
            iterator(std::end(c_beg)), is_even(), identity());
    f_beg1.wait();
    hpx::future<bool> f_beg2 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_beg)),
            iterator(std::end(c_beg)), is_odd(), identity());
    f_beg2.wait();
    hpx::future<bool> f_beg3 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_beg)),
            iterator(std::end(c_beg)), is_even(), add_one());
    f_beg3.wait();
    hpx::future<bool> f_beg4 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_beg)),
            iterator(std::end(c_beg)), is_odd(), add_one());
    f_beg4.wait();
    hpx::future<bool> f_end1 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_end)),
            iterator(std::end(c_end)), is_even(), identity());
    f_end1.wait();
    hpx::future<bool> f_end2 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_end)),
            iterator(std::end(c_end)), is_odd(), identity());
    f_end2.wait();
    hpx::future<bool> f_end3 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_end)),
            iterator(std::end(c_end)), is_even(), add_one());
    f_end3.wait();
    hpx::future<bool> f_end4 =
        hpx::ranges::is_partitioned(p, iterator(std::begin(c_end)),
            iterator(std::end(c_end)), is_odd(), add_one());
    f_end4.wait();

    HPX_TEST(!f_beg1.get());
    HPX_TEST(!f_beg2.get());
    HPX_TEST(!f_beg3.get());
    HPX_TEST(!f_beg4.get());
    HPX_TEST(!f_end1.get());
    HPX_TEST(!f_end2.get());
    HPX_TEST(!f_end3.get());
    HPX_TEST(!f_end4.get());
}

template <typename ExPolicy>
void test_partitioned3(ExPolicy policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c_beg(10007);
    //fill first half of array with even numbers and second half
    //with odd numbers
    std::fill(std::begin(c_beg),
        std::begin(c_beg) + static_cast<std::ptrdiff_t>(c_beg.size() / 2),
        2 * (dis(gen)));
    std::fill(std::begin(c_beg) + static_cast<std::ptrdiff_t>(c_beg.size() / 2),
        std::end(c_beg), 2 * (dis(gen)) + 1);
    std::vector<size_t> c_end = c_beg;
    //add odd number to the beginning
    c_beg[0] -= 1;
    //add even number to end
    c_end[c_end.size() - 1] -= 1;

    bool parted_beg1 =
        hpx::ranges::is_partitioned(policy, c_beg, is_even(), identity());
    bool parted_beg2 =
        hpx::ranges::is_partitioned(policy, c_beg, is_odd(), identity());
    bool parted_beg3 =
        hpx::ranges::is_partitioned(policy, c_beg, is_even(), add_one());
    bool parted_beg4 =
        hpx::ranges::is_partitioned(policy, c_beg, is_odd(), add_one());
    bool parted_end1 =
        hpx::ranges::is_partitioned(policy, c_end, is_even(), identity());
    bool parted_end2 =
        hpx::ranges::is_partitioned(policy, c_end, is_odd(), identity());
    bool parted_end3 =
        hpx::ranges::is_partitioned(policy, c_end, is_even(), add_one());
    bool parted_end4 =
        hpx::ranges::is_partitioned(policy, c_end, is_odd(), add_one());

    HPX_TEST(!parted_beg1);
    HPX_TEST(!parted_beg2);
    HPX_TEST(!parted_beg3);
    HPX_TEST(!parted_beg4);
    HPX_TEST(!parted_end1);
    HPX_TEST(!parted_end2);
    HPX_TEST(!parted_end3);
    HPX_TEST(!parted_end4);
}

template <typename ExPolicy>
void test_partitioned3_async(ExPolicy p)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c_beg(10007);
    //fill first half of array with even numbers and second half
    //with odd numbers
    std::fill(std::begin(c_beg),
        std::begin(c_beg) + static_cast<std::ptrdiff_t>(c_beg.size() / 2),
        2 * (dis(gen)));
    std::fill(std::begin(c_beg) + static_cast<std::ptrdiff_t>(c_beg.size() / 2),
        std::end(c_beg), 2 * (dis(gen)) + 1);
    std::vector<size_t> c_end = c_beg;
    //add odd number to the beginning
    c_beg[0] -= 1;
    //add even number to end
    c_end[c_end.size() - 1] -= 1;

    hpx::future<bool> f_beg1 =
        hpx::ranges::is_partitioned(p, c_beg, is_even(), identity());
    f_beg1.wait();
    hpx::future<bool> f_beg2 =
        hpx::ranges::is_partitioned(p, c_beg, is_odd(), identity());
    f_beg2.wait();
    hpx::future<bool> f_beg3 =
        hpx::ranges::is_partitioned(p, c_beg, is_even(), add_one());
    f_beg3.wait();
    hpx::future<bool> f_beg4 =
        hpx::ranges::is_partitioned(p, c_beg, is_odd(), add_one());
    f_beg4.wait();
    hpx::future<bool> f_end1 =
        hpx::ranges::is_partitioned(p, c_end, is_even(), identity());
    f_end1.wait();
    hpx::future<bool> f_end2 =
        hpx::ranges::is_partitioned(p, c_end, is_odd(), identity());
    f_end2.wait();
    hpx::future<bool> f_end3 =
        hpx::ranges::is_partitioned(p, c_end, is_even(), add_one());
    f_end3.wait();
    hpx::future<bool> f_end4 =
        hpx::ranges::is_partitioned(p, c_end, is_odd(), add_one());
    f_end4.wait();

    HPX_TEST(!f_beg1.get());
    HPX_TEST(!f_beg2.get());
    HPX_TEST(!f_beg3.get());
    HPX_TEST(!f_beg4.get());
    HPX_TEST(!f_end1.get());
    HPX_TEST(!f_end2.get());
    HPX_TEST(!f_end3.get());
    HPX_TEST(!f_end4.get());
}

template <typename IteratorTag>
void test_partitioned3()
{
    using namespace hpx::execution;
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_partitioned3(seq, IteratorTag());
        test_partitioned3(par, IteratorTag());
        test_partitioned3(par_unseq, IteratorTag());

        test_partitioned3_async(seq(task), IteratorTag());
        test_partitioned3_async(par(task), IteratorTag());
    }
}

void partitioned_test3()
{
    test_partitioned3<std::random_access_iterator_tag>();
    test_partitioned3<std::forward_iterator_tag>();

    using namespace hpx::execution;
    test_partitioned3(seq);
    test_partitioned3(par);
    test_partitioned3(par_unseq);

    test_partitioned3_async(seq(task));
    test_partitioned3_async(par(task));
}

int hpx_main()
{
    partitioned_test1();
    partitioned_test2();
    partitioned_test3();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
