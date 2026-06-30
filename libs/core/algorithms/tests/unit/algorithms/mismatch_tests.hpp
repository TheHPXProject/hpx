//  Copyright (c) 2014-2025 Hartmut Kaiser
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
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(0, 10006);

template <typename IteratorTag>
void test_mismatch1(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result = hpx::mismatch(begin1, end1, std::begin(c2));

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result = hpx::mismatch(begin1, end1, std::begin(c2));

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch1(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result = hpx::mismatch(policy, begin1, end1, std::begin(c2));

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result = hpx::mismatch(policy, begin1, end1, std::begin(c2));

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch1_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto f = hpx::mismatch(p, begin1, end1, std::begin(c2));
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto f = hpx::mismatch(p, begin1, end1, std::begin(c2));
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_mismatch2(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result =
            hpx::mismatch(begin1, end1, std::begin(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result =
            hpx::mismatch(begin1, end1, std::begin(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch2(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto result = hpx::mismatch(
            policy, begin1, end1, std::begin(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto result = hpx::mismatch(
            policy, begin1, end1, std::begin(c2), std::equal_to<>());

        // verify values
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_mismatch2_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto f =
            hpx::mismatch(p, begin1, end1, std::begin(c2), std::equal_to<>());
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), c1.size());
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto f =
            hpx::mismatch(p, begin1, end1, std::begin(c2), std::equal_to<>());
        f.wait();

        // verify values
        auto result = f.get();
        HPX_TEST_EQ(
            std::size_t(std::distance(begin1, result.first)), changed_idx);
        HPX_TEST_EQ(std::size_t(std::distance(std::begin(c2), result.second)),
            changed_idx);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_mismatch_exception(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_exception = false;
    try
    {
        hpx::mismatch(iterator(std::begin(c1)), iterator(std::end(c1)),
            std::begin(c2),
            [](auto, auto) { return throw std::runtime_error("test"), true; });

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
void test_mismatch_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_exception = false;
    try
    {
        hpx::mismatch(policy, iterator(std::begin(c1)), iterator(std::end(c1)),
            std::begin(c2),
            [](auto, auto) { return throw std::runtime_error("test"), true; });

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
void test_mismatch_exception_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::mismatch(p, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2),
            [](auto, auto) { return throw std::runtime_error("test"), true; });
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

/////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_mismatch_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_bad_alloc = false;
    try
    {
        hpx::mismatch(policy, iterator(std::begin(c1)), iterator(std::end(c1)),
            std::begin(c2),
            [](auto, auto) { return throw std::bad_alloc(), true; });

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
void test_mismatch_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::mismatch(p, iterator(std::begin(c1)),
            iterator(std::end(c1)), std::begin(c2),
            [](auto, auto) { return throw std::bad_alloc(), true; });
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

////////////////////////////////////////////////////////////////////////////////

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_mismatch_sender(LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;
    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<int> c1(10007);
    std::vector<int> c2(c1.size());

    unsigned int first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);
    std::iota(std::begin(c2), std::end(c2), first_value);

    iterator begin1 = iterator(std::begin(c1));
    iterator end1 = iterator(std::end(c1));

    {
        auto snd_result = tt::sync_wait(ex::just(begin1, end1, std::begin(c2)) |
            hpx::mismatch(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        // verify values
        HPX_TEST_EQ(
            static_cast<std::size_t>(std::distance(begin1, result.first)),
            c1.size());
        HPX_TEST_EQ(static_cast<std::size_t>(
                        std::distance(std::begin(c2), result.second)),
            c2.size());
    }

    {
        std::size_t changed_idx = dis(gen);    //-V104
        ++c1[changed_idx];

        auto snd_result = tt::sync_wait(ex::just(begin1, end1, std::begin(c2)) |
            hpx::mismatch(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        // verify values
        HPX_TEST_EQ(
            static_cast<std::size_t>(std::distance(begin1, result.first)),
            changed_idx);
        HPX_TEST_EQ(static_cast<std::size_t>(
                        std::distance(std::begin(c2), result.second)),
            changed_idx);
    }

    {
        // edge case: empty range

        auto snd_result =
            tt::sync_wait(ex::just(iterator(std::begin(c1)),
                              iterator(std::begin(c1)), std::begin(c2)) |
                hpx::mismatch(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        // verify values
        HPX_TEST(result.first.base() == std::begin(c1));
        HPX_TEST(result.second == std::begin(c2));
    }
}

///////////////////////////////////////////////////////////////////////////////
// Cross-policy consistency tests for hpx::mismatch
//
// Extends the empty-range coverage gap: the STDEXEC sender path
// (test_mismatch_sender) has an empty-range check, but the standard policy
// overloads test_mismatch1 / test_mismatch2 do not. This function exercises
// seq, par, and par_unseq for empty range and several boundary conditions.
//
// Datasets covered:
//   1. Empty range              -> result.first == first1, result.second == first2
//   2. Single element, equal   -> no mismatch, result at end
//   3. Mismatch at position 0  -> first pair differs
//   4. Mismatch at last pos    -> last pair differs
//   5. Fully matching range    -> result at end for all policies
//   6. Cross-policy agreement  -> seq, par, par_unseq must agree on all above
template <typename IteratorTag>
void test_mismatch_cross_policy(IteratorTag)
{
    using namespace hpx::execution;

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    auto check_policy = [&](std::vector<int>& a, std::vector<int>& b,
                            char const* scenario) {
        auto rs = hpx::mismatch(
            seq, iterator(a.begin()), iterator(a.end()), b.begin());
        auto rp = hpx::mismatch(
            par, iterator(a.begin()), iterator(a.end()), b.begin());
        auto ru = hpx::mismatch(
            par_unseq, iterator(a.begin()), iterator(a.end()), b.begin());
        HPX_TEST_MSG(rs == rp, scenario);
        HPX_TEST_MSG(rs == ru, scenario);
    };

    // 1. Empty range (fixes the sender-only coverage gap)
    {
        std::vector<int> a, b;
        auto rs = hpx::mismatch(
            seq, iterator(a.begin()), iterator(a.end()), b.begin());
        auto rp = hpx::mismatch(
            par, iterator(a.begin()), iterator(a.end()), b.begin());
        auto ru = hpx::mismatch(
            par_unseq, iterator(a.begin()), iterator(a.end()), b.begin());
        // All must agree
        HPX_TEST_MSG(rs == rp, "mismatch: empty range, seq==par");
        HPX_TEST_MSG(rs == ru, "mismatch: empty range, seq==par_unseq");
        // Result must be {first1, first2}
        HPX_TEST_MSG(rs.first == iterator(a.begin()),
            "mismatch: empty range, first1 returned");
        HPX_TEST_MSG(
            rs.second == b.begin(), "mismatch: empty range, first2 returned");
    }

    // 2. Single element, equal
    {
        std::vector<int> a = {7}, b = {7};
        check_policy(a, b, "mismatch: single element, equal");
        // Validate: no mismatch -> result is at end
        auto r = hpx::mismatch(
            seq, iterator(a.begin()), iterator(a.end()), b.begin());
        HPX_TEST(r.first == iterator(a.end()));
    }

    // 3. Mismatch at position 0 (first elements differ)
    {
        std::vector<int> a(500, 1), b(500, 1);
        a[0] = 9;
        check_policy(a, b, "mismatch: diff at index 0");
        auto r = hpx::mismatch(
            seq, iterator(a.begin()), iterator(a.end()), b.begin());
        HPX_TEST_EQ(std::size_t(std::distance(iterator(a.begin()), r.first)),
            std::size_t(0));
    }

    // 4. Mismatch at last position
    {
        std::vector<int> a(500, 1), b(500, 1);
        a[499] = 9;
        check_policy(a, b, "mismatch: diff at last index");
        auto r = hpx::mismatch(
            seq, iterator(a.begin()), iterator(a.end()), b.begin());
        HPX_TEST_EQ(std::size_t(std::distance(iterator(a.begin()), r.first)),
            std::size_t(499));
    }

    // 5. Fully matching range - result must be at end for all policies
    {
        std::vector<int> a(500, 3), b(500, 3);
        check_policy(a, b, "mismatch: fully matching");
        auto r = hpx::mismatch(
            seq, iterator(a.begin()), iterator(a.end()), b.begin());
        HPX_TEST(r.first == iterator(a.end()));
    }
}
