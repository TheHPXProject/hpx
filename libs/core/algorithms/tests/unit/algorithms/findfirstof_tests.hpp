//  Copyright (c) 2021 Srinivas Yadav
//  copyright (c) 2014 Grant Mercer
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

#include <atomic>
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
std::uniform_int_distribution<> dis(0, 10006);
std::uniform_int_distribution<> dist(0, 2);

template <typename IteratorTag>
void test_find_first_of(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    int find_first_of_pos = dis(gen);
    int random_sub_seq_pos = dist(gen);

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 19);
    int h[] = {1, 7, 18, 3};
    c[find_first_of_pos] = h[random_sub_seq_pos];    //-V108

    iterator index = hpx::find_first_of(iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    base_iterator test_index = std::begin(c) + find_first_of_pos;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_find_first_of(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    int find_first_of_pos = dis(gen);
    int random_sub_seq_pos = dist(gen);

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 19);
    int h[] = {1, 7, 18, 3};
    c[find_first_of_pos] = h[random_sub_seq_pos];    //-V108

    iterator index = hpx::find_first_of(policy, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));

    base_iterator test_index = std::begin(c) + find_first_of_pos;

    HPX_TEST(index == iterator(test_index));
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_find_first_of_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    int find_first_of_pos = dis(gen);
    int random_sub_seq_pos = dist(gen);

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 19);
    int h[] = {1, 7, 18, 3};
    c[find_first_of_pos] = h[random_sub_seq_pos];    //-V108

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    auto snd_result =
        tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                          std::begin(h), std::end(h)) |
            hpx::find_first_of(ex_policy.on(exec)));

    iterator index = hpx::get<0>(*snd_result);

    base_iterator test_index = std::begin(c) + find_first_of_pos;

    HPX_TEST(index == iterator(test_index));
}

template <typename ExPolicy, typename IteratorTag>
void test_find_first_of_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    int find_first_of_pos = dis(gen);
    int random_sub_seq_pos = dist(gen);

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 19);
    int h[] = {1, 7, 18, 3};
    c[find_first_of_pos] = h[random_sub_seq_pos];    //-V108

    hpx::future<iterator> f = hpx::find_first_of(p, iterator(std::begin(c)),
        iterator(std::end(c)), std::begin(h), std::end(h));
    f.wait();

    // create iterator at position of value to be found
    base_iterator test_index = std::begin(c) + find_first_of_pos;

    HPX_TEST(f.get() == iterator(test_index));
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_find_first_of_exception(IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;

    bool caught_exception = false;
    try
    {
        int h[] = {1, 2};

        hpx::find_first_of(decorated_iterator(std::begin(c),
                               []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), std::begin(h), std::end(h));
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
void test_find_first_of_exception(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;
    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;

    bool caught_exception = false;
    try
    {
        int h[] = {1, 2};

        hpx::find_first_of(policy,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), std::begin(h), std::end(h));
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
void test_find_first_of_exception_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        int h[] = {1, 2};

        hpx::future<decorated_iterator> f = hpx::find_first_of(p,
            decorated_iterator(
                std::begin(c), []() { throw std::runtime_error("test"); }),
            decorated_iterator(std::end(c)), std::begin(h), std::end(h));
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
void test_find_first_of_bad_alloc(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(100007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;

    bool caught_bad_alloc = false;
    try
    {
        int h[] = {1, 2};

        hpx::find_first_of(policy,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), std::begin(h), std::end(h));
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
void test_find_first_of_bad_alloc_async(ExPolicy&& p, IteratorTag)
{
    typedef std::vector<int>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<int> c(10007);
    std::iota(std::begin(c), std::end(c), gen() + 1);
    c[c.size() / 2] = 1;

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        int h[] = {1, 2};

        hpx::future<decorated_iterator> f = hpx::find_first_of(p,
            decorated_iterator(std::begin(c), []() { throw std::bad_alloc(); }),
            decorated_iterator(std::end(c)), std::begin(h), std::end(h));
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
// Edge-case and consistency tests added to cover:
//   * empty haystack / empty needle range (boundary conditions)
//   * single-element haystack with / without a match
//   * predicate invocation count (proves the inner search loop
//     short-circuits after the first match - the bug this PR fixes)
template <typename ExPolicy>
void test_find_first_of_edge_cases(ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    // -----------------------------------------------------------------------
    // 1. Empty haystack: must return last
    {
        std::vector<int> haystack;
        std::vector<int> needles = {1, 2, 3};

        auto result = hpx::find_first_of(policy, haystack.begin(),
            haystack.end(), needles.begin(), needles.end());

        HPX_TEST(result == haystack.end());
    }

    // -----------------------------------------------------------------------
    // 2. Empty needle range: nothing can match, must return last
    {
        std::vector<int> haystack = {1, 2, 3, 4, 5};
        std::vector<int> needles;

        auto result = hpx::find_first_of(policy, haystack.begin(),
            haystack.end(), needles.begin(), needles.end());

        HPX_TEST(result == haystack.end());
    }

    // -----------------------------------------------------------------------
    // 3. Single-element haystack with a match
    {
        std::vector<int> haystack = {42};
        std::vector<int> needles = {10, 42, 99};

        auto result = hpx::find_first_of(policy, haystack.begin(),
            haystack.end(), needles.begin(), needles.end());

        HPX_TEST(result == haystack.begin());
    }

    // -----------------------------------------------------------------------
    // 4. Single-element haystack with no match
    {
        std::vector<int> haystack = {7};
        std::vector<int> needles = {1, 2, 3};

        auto result = hpx::find_first_of(policy, haystack.begin(),
            haystack.end(), needles.begin(), needles.end());

        HPX_TEST(result == haystack.end());
    }

    // -----------------------------------------------------------------------
    // 5. Predicate invocation-count consistency (key regression guard):
    //
    //    When the needle range contains duplicate values that match the same
    //    haystack element, a correct implementation must stop testing the
    //    remaining needles once the first match is found (inner loop must
    //    short-circuit).  Before this fix the par/par_unseq partition kernel
    //    was missing the early return, causing extra predicate calls that
    //    are inconsistent with the seq behaviour.
    //
    //    Haystack: [7, 100, 200]
    //    Needles:  [7, 7]  (deliberate duplicates)
    //
    //    Per-element predicate call budget (correct behaviour):
    //      index 0 (value 7)   -> 1 call  (matches needles[0], exit inner loop)
    //      index 1 (value 100) -> 2 calls (no match against 7 twice)
    //      index 2 (value 200) -> 2 calls (no match against 7 twice)
    //    Total: 5 calls.
    //
    //    Before the fix, index 0 cost 2 calls -> 6 total.
    {
        std::atomic<int> call_count{0};
        auto counting_pred = [&call_count](int a, int b) -> bool {
            ++call_count;
            return a == b;
        };

        std::vector<int> haystack = {7, 100, 200};
        std::vector<int> needles = {7, 7};    // deliberate duplicates

        call_count.store(0);
        auto result = hpx::find_first_of(policy, haystack.begin(),
            haystack.end(), needles.begin(), needles.end(), counting_pred);

        // Must point to haystack[0] (value 7)
        HPX_TEST(result == haystack.begin());
        // Inner loop must short-circuit: total calls must be <= 5
        HPX_TEST_LTE(call_count.load(), 5);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Standard-compliance test: empty needle range must return last
//
// C++ standard [alg.find.first.of]:
//   "Returns: last if [s_first, s_last) is empty or if no such iterator
//    is found."
// This is exercised independently for seq, par, and par_unseq because parallel
// implementations must honor the same pre-condition check before any work.
template <typename ExPolicy, typename IteratorTag>
void test_find_first_of_empty_needle(ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<int> c = {1, 2, 3, 4, 5};
    std::vector<int> needle;    // deliberately empty

    iterator result = hpx::find_first_of(policy, iterator(c.begin()),
        iterator(c.end()), needle.begin(), needle.end());

    // Must return last (== c.end()) when needle is empty
    HPX_TEST(result == iterator(c.end()));
}

///////////////////////////////////////////////////////////////////////////////
// Cross-policy consistency tests for hpx::find_first_of
//
// Verifies that seq, par, and par_unseq return the same iterator for the same
// haystack and needle. This is a direct regression guard for bugs where the
// parallel inner loop does not short-circuit correctly (see historical
// find_first_of fix).
//
// Datasets covered:
//   1. Empty haystack                -> all return end (== begin)
//   2. No element in needle present  -> all return end
//   3. Match at position 0           -> first partition boundary
//   4. Match at last position        -> last partition boundary
//   5. Multiple occurrences in needle -> any match, but FIRST in haystack
//   6. Single-element haystack, match
//   7. Single-element haystack, no match
template <typename IteratorTag>
void test_find_first_of_cross_policy(IteratorTag)
{
    using namespace hpx::execution;

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    auto check_policy = [&](std::vector<int>& haystack,
                            std::vector<int>& needle, char const* scenario) {
        auto rs = hpx::find_first_of(seq, iterator(haystack.begin()),
            iterator(haystack.end()), needle.begin(), needle.end());
        auto rp = hpx::find_first_of(par, iterator(haystack.begin()),
            iterator(haystack.end()), needle.begin(), needle.end());
        auto ru = hpx::find_first_of(par_unseq, iterator(haystack.begin()),
            iterator(haystack.end()), needle.begin(), needle.end());
        HPX_TEST_MSG(rs == rp, scenario);
        HPX_TEST_MSG(rs == ru, scenario);
    };

    // 1. Empty haystack - all must return end (which equals begin)
    {
        std::vector<int> hay, ndl = {1, 2, 3};
        check_policy(hay, ndl, "find_first_of: empty haystack");
    }

    // 2. No element in needle present anywhere
    {
        std::vector<int> hay(1013, 5), ndl = {99, 100};
        check_policy(hay, ndl, "find_first_of: no match");
    }

    // 3. Match at position 0
    {
        std::vector<int> hay(1013, 5), ndl = {7, 8};
        hay[0] = 7;
        check_policy(hay, ndl, "find_first_of: match at index 0");
    }

    // 4. Match at last position
    {
        std::vector<int> hay(1013, 5), ndl = {7, 8};
        hay[1012] = 8;
        check_policy(hay, ndl, "find_first_of: match at last index");
    }

    // 5. First occurrence wins when multiple matches exist
    {
        std::vector<int> hay(1013, 5), ndl = {7, 8};
        hay[200] = 7;
        hay[800] = 8;
        check_policy(hay, ndl, "find_first_of: multiple matches, return first");
    }

    // 6. Single-element haystack, needle matches it
    {
        std::vector<int> hay = {42}, ndl = {42};
        check_policy(hay, ndl, "find_first_of: single element, match");
    }

    // 7. Single-element haystack, needle does not match it
    {
        std::vector<int> hay = {1}, ndl = {99};
        check_policy(hay, ndl, "find_first_of: single element, no match");
    }
}
