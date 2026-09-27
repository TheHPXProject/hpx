//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2018 Bruno Pitrus
//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(3, 102);
std::uniform_int_distribution<> dist(7, 106);

template <typename IteratorTag>
void test_find_end1(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1, 2};

    base_iterator index = hpx::ranges::find_end(c, h);

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end1(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1, 2};

    base_iterator index =
        test::subrange_begin(hpx::ranges::find_end(policy, c, h));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename IteratorTag>
void test_find_end1_proj(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1 + 65536;
    c[c.size() / 2 + 1] = 2 + 65536;

    std::size_t h[] = {1, 2};

    base_iterator index = hpx::ranges::find_end(c, h,
        std::equal_to<std::size_t>(), [](std::size_t x) { return x % 65536; });

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end1_proj(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1 + 65536;
    c[c.size() / 2 + 1] = 2 + 65536;

    std::size_t h[] = {1, 2};

    base_iterator index = test::subrange_begin(
        hpx::ranges::find_end(policy, c, h, std::equal_to<std::size_t>(),
            [](std::size_t x) { return x % 65536; }));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end1_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1, 2};

    hpx::future<base_iterator> f =
        test::subrange_begin(hpx::ranges::find_end(p, c, h));
    f.wait();

    // create iterator at position of value to be found
    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(f.get() == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end1_async_proj(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1 + 65536;
    c[c.size() / 2 + 1] = 2 + 65536;

    std::size_t h[] = {1, 2};

    hpx::future<base_iterator> f = test::subrange_begin(
        hpx::ranges::find_end(p, c, h, std::equal_to<std::size_t>(),
            [](std::size_t x) { return x % 65536; }));
    f.wait();

    // create iterator at position of value to be found
    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(f.get() == test_index);
}

template <typename IteratorTag>
void test_find_end1()
{
    using namespace hpx::execution;

    test_find_end1(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_end1(seq, IteratorTag());
        test_find_end1(par, IteratorTag());
        test_find_end1(par_unseq, IteratorTag());
    }

    test_find_end1_proj(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_end1_proj(seq, IteratorTag());
        test_find_end1_proj(par, IteratorTag());
        test_find_end1_proj(par_unseq, IteratorTag());

        test_find_end1_async(seq(task), IteratorTag());
        test_find_end1_async(par(task), IteratorTag());
        test_find_end1_async_proj(seq(task), IteratorTag());
        test_find_end1_async_proj(par(task), IteratorTag());
    }
}

void find_end_test1()
{
    test_find_end1<std::random_access_iterator_tag>();
    test_find_end1<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_end2(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values about 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    std::size_t h[] = {1, 2};

    base_iterator index = hpx::ranges::find_end(c, h);

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() - 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end2(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values about 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    std::size_t h[] = {1, 2};

    base_iterator index =
        test::subrange_begin(hpx::ranges::find_end(policy, c, h));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() - 2);

    HPX_TEST(index == test_index);
}

template <typename IteratorTag>
void test_find_end2_proj(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values about 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    std::size_t h[] = {1 + 65536, 2 + 65536};

    auto proj = [](std::size_t x) { return x % 65536; };

    base_iterator index =
        hpx::ranges::find_end(c, h, std::equal_to<std::size_t>(), proj, proj);

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() - 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end2_proj(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values about 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    std::size_t h[] = {1 + 65536, 2 + 65536};

    auto proj = [](std::size_t x) { return x % 65536; };

    base_iterator index = test::subrange_begin(hpx::ranges::find_end(
        policy, c, h, std::equal_to<std::size_t>(), proj, proj));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() - 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end2_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    // fill vector with random values above 2
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    std::size_t h[] = {1, 2};

    hpx::future<base_iterator> f =
        test::subrange_begin(hpx::ranges::find_end(p, c, h));
    f.wait();

    // create iterator at position of value to be found
    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() - 2);

    HPX_TEST(f.get() == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end2_async_proj(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    // fill vector with random values above 2
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    std::size_t h[] = {1 + 65536, 2 + 65536};

    auto proj = [](std::size_t x) { return x % 65536; };

    hpx::future<base_iterator> f = test::subrange_begin(hpx::ranges::find_end(
        p, c, h, std::equal_to<std::size_t>(), proj, proj));

    f.wait();

    // create iterator at position of value to be found
    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() - 2);

    HPX_TEST(f.get() == test_index);
}

template <typename IteratorTag>
void test_find_end2()
{
    using namespace hpx::execution;

    test_find_end2(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_end2(seq, IteratorTag());
        test_find_end2(par, IteratorTag());
        test_find_end2(par_unseq, IteratorTag());
    }

    test_find_end2_proj(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_end2_proj(seq, IteratorTag());
        test_find_end2_proj(par, IteratorTag());
        test_find_end2_proj(par_unseq, IteratorTag());

        test_find_end2_async(seq(task), IteratorTag());
        test_find_end2_async(par(task), IteratorTag());
        test_find_end2_async_proj(seq(task), IteratorTag());
        test_find_end2_async_proj(par(task), IteratorTag());
    }
}

void find_end_test2()
{
    test_find_end2<std::random_access_iterator_tag>();
    test_find_end2<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_end3(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    // fill vector with random values above 2
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence large enough to always be split into multiple partitions
    std::iota(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 16 + 1), 1);
    std::size_t sub_size = c.size() / 16 + 1;

    std::vector<std::size_t> h(sub_size);
    std::iota(std::begin(h), std::end(h), 1);

    base_iterator index = hpx::ranges::find_end(c, h);

    base_iterator test_index = std::begin(c);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end3(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;

    // fill vector with random values above 2
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence large enough to always be split into multiple partitions
    std::iota(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 16 + 1), 1);
    std::size_t sub_size = c.size() / 16 + 1;

    std::vector<std::size_t> h(sub_size);
    std::iota(std::begin(h), std::end(h), 1);

    base_iterator index =
        test::subrange_begin(hpx::ranges::find_end(policy, c, h));

    base_iterator test_index = std::begin(c);

    HPX_TEST(index == test_index);
}

template <typename IteratorTag>
void test_find_end3_proj(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    // fill vector with random values above 2
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence large enough to always be split into multiple partitions
    std::iota(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 16 + 1), 1);
    std::size_t sub_size = c.size() / 16 + 1;

    std::vector<std::size_t> h(sub_size);
    std::iota(std::begin(h), std::end(h), 1 + 65536);

    auto proj = [](std::size_t x) { return x % 65536; };

    base_iterator index =
        hpx::ranges::find_end(c, h, std::equal_to<std::size_t>(), proj, proj);

    base_iterator test_index = std::begin(c);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end3_proj(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;

    // fill vector with random values above 2
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence large enough to always be split into multiple partitions
    std::iota(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 16 + 1), 1);
    std::size_t sub_size = c.size() / 16 + 1;

    std::vector<std::size_t> h(sub_size);
    std::iota(std::begin(h), std::end(h), 1 + 65536);

    auto proj = [](std::size_t x) { return x % 65536; };

    base_iterator index = test::subrange_begin(hpx::ranges::find_end(
        policy, c, h, std::equal_to<std::size_t>(), proj, proj));

    base_iterator test_index = std::begin(c);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end3_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    // fill vector with random values above 6
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), dist(gen));

    // create subsequence large enough to always be split into multiple partitions
    std::iota(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 16 + 1), 1);
    std::size_t sub_size = c.size() / 16 + 1;

    std::vector<std::size_t> h(sub_size);
    std::iota(std::begin(h), std::end(h), 1);

    // create only two partitions, splitting the desired sub sequence into
    // separate partitions.
    hpx::future<base_iterator> f =
        test::subrange_begin(hpx::ranges::find_end(p, c, h));
    f.wait();

    //create iterator at position of value to be found
    base_iterator test_index = std::begin(c);

    HPX_TEST(f.get() == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end3_async_proj(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    // fill vector with random values above 6
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), dist(gen));

    // create subsequence large enough to always be split into multiple partitions
    std::iota(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 16 + 1), 1);
    std::size_t sub_size = c.size() / 16 + 1;

    std::vector<std::size_t> h(sub_size);
    std::iota(std::begin(h), std::end(h), 1 + 65536);

    auto proj = [](std::size_t x) { return x % 65536; };

    // create only two partitions, splitting the desired sub sequence into
    // separate partitions.
    hpx::future<base_iterator> f = test::subrange_begin(hpx::ranges::find_end(
        p, c, h, std::equal_to<std::size_t>(), proj, proj));

    f.wait();

    //create iterator at position of value to be found
    base_iterator test_index = std::begin(c);

    HPX_TEST(f.get() == test_index);
}
template <typename IteratorTag>
void test_find_end3()
{
    using namespace hpx::execution;

    test_find_end3(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_end3(seq, IteratorTag());
        test_find_end3(par, IteratorTag());
        test_find_end3(par_unseq, IteratorTag());
    }

    test_find_end3_proj(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_end3_proj(seq, IteratorTag());
        test_find_end3_proj(par, IteratorTag());
        test_find_end3_proj(par_unseq, IteratorTag());

        test_find_end3_async(seq(task), IteratorTag());
        test_find_end3_async(par(task), IteratorTag());
        test_find_end3_async_proj(seq(task), IteratorTag());
        test_find_end3_async_proj(par(task), IteratorTag());
    }
}

void find_end_test3()
{
    test_find_end3<std::random_access_iterator_tag>();
    test_find_end3<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_end4(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1, 2};

    base_iterator index = hpx::ranges::find_end(
        c, h, [](std::size_t v1, std::size_t v2) { return !(v1 != v2); });

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end4(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1, 2};

    base_iterator index = test::subrange_begin(hpx::ranges::find_end(policy, c,
        h, [](std::size_t v1, std::size_t v2) { return !(v1 != v2); }));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename IteratorTag>
void test_find_end4_proj(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1 + 65536, 2 + 65536};

    auto proj = [](std::size_t x) { return x % 65536; };

    base_iterator index = hpx::ranges::find_end(
        c, h, [](std::size_t v1, std::size_t v2) { return !(v1 != v2); }, proj,
        proj);

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end4_proj(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1 + 65536, 2 + 65536};

    auto proj = [](std::size_t x) { return x % 65536; };

    base_iterator index = test::subrange_begin(hpx::ranges::find_end(
        policy, c, h,
        [](std::size_t v1, std::size_t v2) { return !(v1 != v2); }, proj,
        proj));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end4_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    // fill vector with random values above 2
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1, 2};

    hpx::future<base_iterator> f = test::subrange_begin(hpx::ranges::find_end(
        p, c, h, [](std::size_t v1, std::size_t v2) { return !(v1 != v2); }));
    f.wait();

    //create iterator at position of value to be found
    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(f.get() == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end4_async_proj(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    // fill vector with random values above 2
    std::vector<std::size_t> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::size_t h[] = {1 + 65536, 2 + 65536};

    auto proj = [](std::size_t x) { return x % 65536; };

    hpx::future<base_iterator> f = test::subrange_begin(hpx::ranges::find_end(
        p, c, h, [](std::size_t v1, std::size_t v2) { return !(v1 != v2); },
        proj, proj));

    f.wait();

    //create iterator at position of value to be found
    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(f.get() == test_index);
}

template <typename IteratorTag>
void test_find_end4()
{
    using namespace hpx::execution;

    test_find_end4(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_end4(seq, IteratorTag());
        test_find_end4(par, IteratorTag());
        test_find_end4(par_unseq, IteratorTag());
    }

    test_find_end4_proj(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_end4_proj(seq, IteratorTag());
        test_find_end4_proj(par, IteratorTag());
        test_find_end4_proj(par_unseq, IteratorTag());

        test_find_end4_async(seq(task), IteratorTag());
        test_find_end4_async(par(task), IteratorTag());
        test_find_end4_async_proj(seq(task), IteratorTag());
        test_find_end4_async_proj(par(task), IteratorTag());
    }
}

void find_end_test4()
{
    test_find_end4<std::random_access_iterator_tag>();
    test_find_end4<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    find_end_test1();
    find_end_test2();
    find_end_test3();
    find_end_test4();
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
