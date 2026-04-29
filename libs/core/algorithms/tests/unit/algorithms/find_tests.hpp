//  Copyright (c) 2021 Srinivas Yadav
//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2022-2025 Hartmut Kaiser
//  Copyright (c) 2024 Tobias Wukovitsch
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
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
std::uniform_int_distribution<> dis(2, 101);

template <typename IteratorTag>
void test_find(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    //fill vector with random values about 1
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(c.size() / 2) = 1;

    iterator index =
        hpx::find(iterator(std::begin(c)), iterator(std::end(c)), int(1));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_find(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    // fill vector with random values about 1
    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(c.size() / 2) = 1;

    iterator index = hpx::find(
        policy, iterator(std::begin(c)), iterator(std::end(c)), int(1));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == iterator(test_index));
}

template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_find_explicit_sender_direct(Policy l, ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    // fill vector with random values about 1
    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(c.size() / 2) = 1;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));
    iterator index = hpx::find(policy.on(exec), iterator(std::begin(c)),
        iterator(std::end(c)), int(1));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(index == iterator(test_index));
}

template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_find_explicit_sender(Policy l, ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    // fill vector with random values about 1
    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(c.size() / 2) = 1;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));

    auto result = tt::sync_wait(
        ex::just(iterator(std::begin(c)), iterator(std::end(c)), int(1)) |
        hpx::find(policy.on(exec)));

    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(hpx::get<0>(*result) == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_find_async(ExPolicy&& p, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c(10007);
    // fill vector with random values above 1
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(c.size() / 2) = 1;

    hpx::future<iterator> f =
        hpx::find(p, iterator(std::begin(c)), iterator(std::end(c)), int(1));
    f.wait();

    // create iterator at position of value to be found
    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(f.get() == iterator(test_index));
}

template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_find_explicit_sender_direct_async(Policy l, ExPolicy&& p, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    // fill vector with random values above 1
    std::vector<int> c(10007);
    std::fill(std::begin(c), std::end(c), dis(gen));
    c.at(c.size() / 2) = 1;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));

    auto snd_result = tt::sync_wait(hpx::find(
        p.on(exec), iterator(std::begin(c)), iterator(std::end(c)), int(1)));
    auto result = hpx::get<0>(*snd_result);

    // create iterator at position of value to be found
    base_iterator test_index =
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

    HPX_TEST(result == iterator(test_index));
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_find_exception(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 0;

    bool caught_exception = false;
    try
    {
        hpx::find(decorated_iterator(std::begin(c),
                      []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), int(0));
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
void test_find_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 0;

    bool caught_exception = false;
    try
    {
        hpx::find(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), int(0));
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

template <typename ExPolicy, typename IteratorTag>
void test_find_exception_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 0;

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        hpx::future<decorated_iterator> f = hpx::find(p,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), int(0));
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
void test_find_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(100007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 0;

    bool caught_bad_alloc = false;
    try
    {
        hpx::find(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), int(0));
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
void test_find_bad_alloc_async(ExPolicy&& p, IteratorTag)
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
        hpx::future<decorated_iterator> f = hpx::find(p,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), int(0));
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

///////////////////////////////////////////////////////////////////////////////
// Cross-policy consistency tests for hpx::find
//
// Verifies that seq, par, and par_unseq always return the same iterator for
// identical input data. This regression harness catches bugs where the
// parallel partition / early-exit logic diverges from the sequential path.
//
// Datasets covered:
//   1. Empty range              -> all policies must return last
//   2. Single element, match    -> all policies must return begin
//   3. Single element, no match -> all policies must return end (== begin+1)
//   4. Match at position 0      -> boundary: very first chunk
//   5. Match at last position   -> boundary: very last chunk
//   6. No match anywhere        -> all policies must return end
//   7. Multiple matches         -> all policies must return FIRST occurrence
template <typename IteratorTag>
void test_find_cross_policy(IteratorTag)
{
    using namespace hpx::execution;

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    auto check_policy = [&](std::vector<int>& c, int val,
                            char const* scenario) {
        auto r_seq =
            hpx::find(seq, iterator(c.begin()), iterator(c.end()), val);
        auto r_par =
            hpx::find(par, iterator(c.begin()), iterator(c.end()), val);
        auto r_par_u =
            hpx::find(par_unseq, iterator(c.begin()), iterator(c.end()), val);
        HPX_TEST_MSG(r_seq == r_par, scenario);
        HPX_TEST_MSG(r_seq == r_par_u, scenario);
    };

    // 1. Empty range
    {
        std::vector<int> c;
        check_policy(c, 42, "find: empty range");
    }

    // 2. Single element, match
    {
        std::vector<int> c = {42};
        check_policy(c, 42, "find: single element found");
    }

    // 3. Single element, no match
    {
        std::vector<int> c = {1};
        check_policy(c, 99, "find: single element not found");
    }

    // 4. Match at position 0 (first partition boundary)
    {
        std::vector<int> c(1013, 5);
        c[0] = 7;
        check_policy(c, 7, "find: match at index 0");
    }

    // 5. Match at last position (last partition boundary)
    {
        std::vector<int> c(1013, 5);
        c[1012] = 7;
        check_policy(c, 7, "find: match at last index");
    }

    // 6. No match anywhere - all policies must return end()
    {
        std::vector<int> c(1013, 5);
        check_policy(c, 99, "find: no match, must return end");
    }

    // 7. Multiple matches - all policies must return the FIRST occurrence
    {
        std::vector<int> c(1013, 5);
        c[100] = 7;
        c[700] = 7;
        check_policy(c, 7, "find: multiple matches, return first");
    }
}
