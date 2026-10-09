//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <new>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

using iterator = std::vector<int>::iterator;

// An unsized sentinel exercises the ranges overload independently of the
// iterator/iterator overload.
struct sentinel
{
    iterator end;
    bool fail = false;

    friend bool operator==(iterator it, sentinel s)
    {
        if (s.fail)
            throw std::runtime_error("make_heap sentinel test");
        return it == s.end;
    }
};

template <typename Policy>
void test_heap(Policy policy)
{
    for (std::size_t size :
        {0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 1007, 10007})
    {
        std::vector<int> values(size);
        std::iota(values.begin(), values.end(), 0);
        auto const original = values;

        auto sender = hpx::make_heap(policy, values.begin(), values.end());
        static_assert(ex::is_sender_v<decltype(sender)>);
        HPX_TEST(values == original);
        HPX_TEST(
            tt::sync_wait(std::move(sender) | ex::then([] {})).has_value());
        HPX_TEST(std::is_heap(values.begin(), values.end()));
        auto sorted = values;
        std::sort(sorted.begin(), sorted.end());
        HPX_TEST(sorted == original);

        // Already heap-ordered input, a custom comparator, and piped arguments.
        auto piped =
            ex::just(values.begin(), values.end(), std::greater<int>{}) |
            hpx::make_heap(policy);
        auto const before = values;
        HPX_TEST(tt::sync_wait(std::move(piped) | ex::then([] {})).has_value());
        HPX_TEST(
            std::is_heap(values.begin(), values.end(), std::greater<int>{}));
        std::sort(values.begin(), values.end());
        HPX_TEST(values == original);

        // All equivalent elements and the singleton end iterator.
        std::fill(values.begin(), values.end(), 42);
        auto result = tt::sync_wait(hpx::ranges::make_heap(policy, values));
        HPX_TEST(result.has_value());
        HPX_TEST(hpx::get<0>(*result) == values.end());
        HPX_TEST(std::all_of(values.begin(), values.end(),
            [](int value) { return value == 42; }));

        values = before;
        auto range_result = tt::sync_wait(hpx::ranges::make_heap(
            policy, values.begin(), sentinel{values.end()}));
        HPX_TEST(range_result.has_value());
        HPX_TEST(hpx::get<0>(*range_result) == values.end());
        HPX_TEST(std::is_heap(values.begin(), values.end()));
    }
}

struct record
{
    int key;
    int id;

    friend bool operator==(record const&, record const&) = default;
};

template <typename Policy>
void test_projection(Policy policy)
{
    std::vector<record> values(1009);
    int id = 0;
    std::generate(values.begin(), values.end(), [&id] {
        auto const current = id++;
        return record{(current * 37) % 23, current};
    });
    auto const original = values;

    // Comparator and projection temporaries must survive lazy execution.
    auto sender = hpx::ranges::make_heap(policy, values.begin() + 1,
        values.end() - 1, std::greater<int>{}, &record::key);
    HPX_TEST(values == original);
    auto result = tt::sync_wait(std::move(sender));
    HPX_TEST(result.has_value());
    HPX_TEST(hpx::get<0>(*result) == values.end() - 1);
    HPX_TEST(values.front() == original.front());
    HPX_TEST(values.back() == original.back());
    HPX_TEST(std::is_heap(values.begin() + 1, values.end() - 1,
        [](record const& lhs, record const& rhs) {
            return lhs.key > rhs.key;
        }));
    std::sort(values.begin(), values.end(),
        [](record const& lhs, record const& rhs) { return lhs.id < rhs.id; });
    HPX_TEST(values == original);

    auto range_sender =
        hpx::ranges::make_heap(policy, values, std::less<int>{}, &record::key);
    auto range_result = tt::sync_wait(std::move(range_sender));
    HPX_TEST(range_result.has_value());
    HPX_TEST(hpx::get<0>(*range_result) == values.end());
    HPX_TEST(std::is_heap(
        values.begin(), values.end(), [](record const& lhs, record const& rhs) {
            return lhs.key < rhs.key;
        }));
}

template <typename Exception, typename Policy>
void test_exception(Policy policy, bool root_only, bool projection)
{
    std::vector<int> values(1007);
    std::iota(values.begin(), values.end(), 0);
    auto const original = values;
    auto fail = [root_only](int value) {
        // Zero stays at the root until the final level. This tests failures
        // after successful completion of the lower levels as well.
        if (!root_only || value == 0)
        {
            if constexpr (std::is_same_v<Exception, std::bad_alloc>)
                throw std::bad_alloc();
            else
                throw std::runtime_error("make_heap sender test");
        }
    };
    bool caught = false;
    try
    {
        if (projection)
        {
            auto sender = hpx::ranges::make_heap(
                policy, values, std::less<int>{}, [fail](int value) {
                    fail(value);
                    return value;
                });
            HPX_TEST(values == original);
            tt::sync_wait(std::move(sender));
        }
        else
        {
            auto sender = hpx::make_heap(
                policy, values.begin(), values.end(), [fail](int lhs, int rhs) {
                    fail(lhs);
                    fail(rhs);
                    return lhs < rhs;
                });
            HPX_TEST(values == original);
            tt::sync_wait(std::move(sender));
        }
        HPX_TEST(false);
    }
    catch (Exception const& e)
    {
        caught = true;
        if constexpr (std::is_same_v<Exception, hpx::exception_list>)
            HPX_TEST_NEQ(e.size(), std::size_t(0));
    }
    catch (...)
    {
        HPX_TEST(false);
    }
    HPX_TEST(caught);
}

template <typename Policy>
void test_sentinel_exception(Policy policy)
{
    std::vector<int> values{1, 2, 3};
    auto const original = values;
    auto sender = hpx::ranges::make_heap(
        policy, values.begin(), sentinel{values.end(), true});
    try
    {
        tt::sync_wait(std::move(sender));
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        HPX_TEST_NEQ(e.size(), std::size_t(0));
    }
    catch (...)
    {
        HPX_TEST(false);
    }
    HPX_TEST(values == original);
}

template <typename Policy>
void test_stopped(Policy policy)
{
    std::vector<int> values{1, 2, 3};
    auto const original = values;
    ex::unique_any_sender<iterator, iterator> predecessor(ex::just_stopped());
    auto result = tt::sync_wait(
        std::move(predecessor) | hpx::make_heap(policy) | ex::then([] {}));
    HPX_TEST(!result.has_value());
    HPX_TEST(values == original);
}

template <typename Policy>
void test_sync(Policy policy)
{
    std::vector<int> values(1007);
    std::iota(values.begin(), values.end(), 0);
    static_assert(std::is_void_v<decltype(hpx::make_heap(
            policy, values.begin(), values.end()))>);
    hpx::make_heap(policy, values.begin(), values.end());
    HPX_TEST(std::is_heap(values.begin(), values.end()));
    auto end = hpx::ranges::make_heap(policy, values, std::greater<int>{});
    HPX_TEST(end == values.end());
    HPX_TEST(std::is_heap(values.begin(), values.end(), std::greater<int>{}));
}

template <typename Launch, typename Policy>
void test_policy(Launch launch, Policy policy)
{
    auto exec = ex::explicit_scheduler_executor(
        ex::thread_pool_policy_scheduler(launch));
    auto task_policy = policy(hpx::execution::task).on(exec);
    test_heap(task_policy);
    test_projection(task_policy);
    test_sync(policy.on(exec));
    test_stopped(task_policy);

    if constexpr (!hpx::is_unsequenced_execution_policy_v<Policy>)
    {
        test_sentinel_exception(task_policy);
        for (bool root_only : {false, true})
        {
            for (bool projection : {false, true})
            {
                test_exception<hpx::exception_list>(
                    task_policy, root_only, projection);
                test_exception<std::bad_alloc>(
                    task_policy, root_only, projection);
            }
        }
    }
}

template <typename Policy>
void test_ordinary_policy(Policy policy)
{
    for (std::size_t size : {0, 1, 17})
    {
        std::vector<int> values(size);
        std::iota(values.begin(), values.end(), 0);
        auto result = hpx::ranges::make_heap(
            policy, values.begin(), sentinel{values.end()});
        if constexpr (hpx::is_async_execution_policy_v<Policy>)
        {
            static_assert(
                std::is_same_v<decltype(result), hpx::future<iterator>>);
            HPX_TEST(result.get() == values.end());
        }
        else
        {
            static_assert(std::is_same_v<decltype(result), iterator>);
            HPX_TEST(result == values.end());
        }
        HPX_TEST(std::is_heap(values.begin(), values.end()));
    }
}

int hpx_main()
{
    using namespace hpx::execution;
    test_policy(hpx::launch::sync, seq);
    test_policy(hpx::launch::sync, unseq);
    test_policy(hpx::launch::async, par);
    test_policy(hpx::launch::async, par_unseq);
    test_ordinary_policy(seq);
    test_ordinary_policy(par);
    test_ordinary_policy(seq(task));
    test_ordinary_policy(par(task));
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::local::init_params init_args;
    init_args.cfg = std::vector<std::string>{"hpx.os_threads=all"};
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
