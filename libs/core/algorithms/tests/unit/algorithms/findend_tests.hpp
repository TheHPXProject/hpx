//  Copyright (c) 2021 Srinivas Yadav
//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2024 Tobias Wukovitsch
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
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
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    int h[] = {1, 2};

    iterator index = hpx::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end1(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    int h[] = {1, 2};

    iterator index = hpx::find_end(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == test_index);
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_find_end1_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    int h[] = {1, 2};

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    {
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                std::begin(h), std::end(h)) |
            hpx::find_end(ex_policy.on(exec)));

        iterator index = hpx::get<0>(*snd_result);

        iterator test_index = std::find_end(iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(h), std::end(h));

        HPX_TEST(index == test_index);
    }

    {
        // edge case: first2 == end2

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                std::begin(h), std::begin(h)) |
            hpx::find_end(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        HPX_TEST(iterator(std::end(c)) == result);
    }

    {
        // edge case: distance(first2, end2) > distance(first1, end1)

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::begin(c)),
                std::begin(h), std::end(h)) |
            hpx::find_end(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        HPX_TEST(iterator(std::begin(c)) == result);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end1_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    int h[] = {1, 2};

    hpx::future<iterator> f = hpx::find_end(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));
    f.wait();

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(f.get() == test_index);
}

template <typename IteratorTag>
void test_find_end2(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    // fill vector with random values about 2
    std::fill(std::begin(c), std::end(c), dis(gen));
    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    int h[] = {1, 2};

    iterator index = hpx::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end2(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    // fill vector with random values about 2
    std::fill(std::begin(c), std::end(c), dis(gen));
    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    int h[] = {1, 2};

    iterator index = hpx::find_end(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == test_index);
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_find_end2_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> c(10007);
    // fill vector with random values about 2
    std::fill(std::begin(c), std::end(c), dis(gen));
    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    int h[] = {1, 2};

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    auto snd_result =
        tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                          std::begin(h), std::end(h)) |
            hpx::find_end(ex_policy.on(exec)));
    iterator index = hpx::get<0>(*snd_result);

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end2_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // fill vector with random values above 2
    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence at start and end
    c[0] = 1;
    c[1] = 2;
    c[c.size() - 1] = 2;
    c[c.size() - 2] = 1;

    int h[] = {1, 2};

    hpx::future<iterator> f = hpx::find_end(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));
    f.wait();

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(f.get() == test_index);
}

template <typename IteratorTag>
void test_find_end3(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // fill vector with random values above 2
    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence large enough to always be split into multiple partitions
    std::iota(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 16 + 1), 1);
    std::size_t sub_size = c.size() / 16 + 1;

    std::vector<int> h(sub_size);
    std::iota(std::begin(h), std::end(h), 1);

    iterator index = hpx::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end3(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // fill vector with random values above 2
    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence large enough to always be split into multiple partitions
    std::iota(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 16 + 1), 1);
    std::size_t sub_size = c.size() / 16 + 1;

    std::vector<int> h(sub_size);
    std::iota(std::begin(h), std::end(h), 1);

    iterator index = hpx::find_end(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end3_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // fill vector with random values above 6
    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dist(gen));

    // create subsequence large enough to always be split into multiple partitions
    std::iota(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 16 + 1), 1);
    std::size_t sub_size = c.size() / 16 + 1;

    std::vector<int> h(sub_size);
    std::iota(std::begin(h), std::end(h), 1);

    // create only two partitions, splitting the desired sub sequence into
    // separate partitions.
    hpx::future<iterator> f = hpx::find_end(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));
    f.wait();

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    HPX_TEST(f.get() == test_index);
}

template <typename IteratorTag>
void test_find_end4(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    int h[] = {1, 2};

    iterator index = hpx::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h),
        [](auto v1, auto v2) { return !(v1 != v2); });

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h),
        [](auto v1, auto v2) { return !(v1 != v2); });

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end4(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), dis(gen));
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    int h[] = {1, 2};

    iterator index = hpx::find_end(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h),
        [](auto v1, auto v2) { return !(v1 != v2); });

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h),
        [](auto v1, auto v2) { return !(v1 != v2); });

    HPX_TEST(index == test_index);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end4_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // fill vector with random values above 2
    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));

    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    int h[] = {1, 2};

    hpx::future<iterator> f = hpx::find_end(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h),
        [](auto v1, auto v2) { return !(v1 != v2); });
    f.wait();

    iterator test_index = std::find_end(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h),
        [](auto v1, auto v2) { return !(v1 != v2); });

    HPX_TEST(f.get() == test_index);
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_find_end_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    bool caught_exception = false;
    try
    {
        std::vector<int> h;
        h.push_back(1);
        h.push_back(2);

        hpx::find_end(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c), []() { throw std::runtime_error("test"); }),
            std::begin(h), std::end(h));
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename IteratorTag>
void test_find_end_exception(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    bool caught_exception = false;
    try
    {
        std::vector<int> h;
        h.push_back(1);
        h.push_back(2);

        hpx::find_end(decorated_iterator(std::begin(c),
                          []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c), []() { throw std::runtime_error("test"); }),
            std::begin(h), std::end(h));
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<hpx::execution::sequenced_policy,
            IteratorTag>::call(hpx::execution::seq, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_find_end_exception_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        int h[] = {1, 2};

        hpx::future<decorated_iterator> f = hpx::find_end(p,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(
                std::end(c), []() { throw std::runtime_error("test"); }),
            std::begin(h), std::end(h));
        returned_from_algorithm = true;
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
    HPX_TEST(returned_from_algorithm);
}

//////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_find_end_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(100007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 0;

    bool caught_bad_alloc = false;
    try
    {
        int h[] = {1, 2};

        hpx::find_end(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c), []() { throw std::bad_alloc(); }),
            std::begin(h), std::end(h));
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

template <typename ExPolicy, typename IteratorTag>
void test_find_end_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 0;

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        int h[] = {1, 2};

        hpx::future<decorated_iterator> f = hpx::find_end(p,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c), []() { throw std::bad_alloc(); }),
            std::begin(h), std::end(h));
        returned_from_algorithm = true;
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
    HPX_TEST(returned_from_algorithm);
}
