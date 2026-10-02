//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iterator>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_utils.hpp"

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

void record_concurrency(std::atomic<std::size_t>& active_count,
    std::atomic<std::size_t>& max_active_count,
    std::atomic<std::size_t>& invocation_count)
{
    std::size_t const active =
        active_count.fetch_add(1, std::memory_order_relaxed) + 1;
    std::size_t previous_max = max_active_count.load(std::memory_order_relaxed);
    while (previous_max < active &&
        !max_active_count.compare_exchange_weak(previous_max, active,
            std::memory_order_relaxed, std::memory_order_relaxed))
    {
    }

    if (invocation_count.fetch_add(1, std::memory_order_relaxed) < 16)
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    active_count.fetch_sub(1, std::memory_order_relaxed);
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_remove_copy_scheduler(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(!hpx::is_async_execution_policy_v<ExPolicy>);

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using scheduler_type = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> input{1, 2, 3, 2, 4};
    std::vector<int> output(input.size(), -1);
    auto exec = ex::explicit_scheduler_executor(scheduler_type(ln_policy));

    auto result = hpx::remove_copy(ex_policy.on(exec), iterator(input.begin()),
        iterator(input.end()), output.begin(), 2);
    static_assert(std::is_same_v<decltype(result), base_iterator>);

    HPX_TEST(result == output.begin() + 3);
    std::vector<int> const expected{1, 3, 4};
    HPX_TEST(std::equal(expected.begin(), expected.end(), output.begin()));
    HPX_TEST(std::all_of(
        result, output.end(), [](int value) { return value == -1; }));
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_remove_copy_sender_case(LnPolicy ln_policy, ExPolicy&& ex_policy,
    IteratorTag, std::vector<int> input, std::vector<int> const& expected)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>);

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using scheduler_type = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> output(input.size(), -1);
    auto exec = ex::explicit_scheduler_executor(scheduler_type(ln_policy));

    auto sender = ex::just(iterator(input.begin()), iterator(input.end()),
                      output.begin(), 2) |
        hpx::remove_copy(ex_policy.on(exec));
    static_assert(ex::is_sender_v<decltype(sender)>);

    auto result = tt::sync_wait(std::move(sender));
    HPX_TEST(result.has_value());
    if (!result.has_value())
    {
        return;
    }

    auto const output_end = hpx::get<0>(*result);
    HPX_TEST(output_end ==
        output.begin() + static_cast<std::ptrdiff_t>(expected.size()));
    HPX_TEST(std::equal(expected.begin(), expected.end(), output.begin()));
    HPX_TEST(std::all_of(
        output_end, output.end(), [](int value) { return value == -1; }));
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_remove_copy_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    test_remove_copy_sender_case(
        ln_policy, ex_policy, IteratorTag{}, {1, 2, 3, 2, 4}, {1, 3, 4});
    test_remove_copy_sender_case(ln_policy, ex_policy, IteratorTag{}, {}, {});
    test_remove_copy_sender_case(
        ln_policy, ex_policy, IteratorTag{}, {2, 2, 2}, {});
    test_remove_copy_sender_case(
        ln_policy, ex_policy, IteratorTag{}, {1, 3, 4}, {1, 3, 4});
}

void test_remove_copy_sender_parallel()
{
    using namespace hpx::execution;

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::decorated_iterator<base_iterator,
        std::random_access_iterator_tag>;
    using scheduler_type =
        ex::thread_pool_policy_scheduler<hpx::launch::async_policy>;

    constexpr std::size_t count = 65536;
    std::vector<int> input(count, 1);
    std::vector<int> output(count, -1);
    std::atomic<std::size_t> active_count{0};
    std::atomic<std::size_t> max_active_count{0};
    std::atomic<std::size_t> invocation_count{0};
    auto exec =
        ex::explicit_scheduler_executor(scheduler_type(hpx::launch::async));

    auto sender =
        ex::just(iterator(input.begin(),
                     [&active_count, &max_active_count, &invocation_count]() {
                         record_concurrency(
                             active_count, max_active_count, invocation_count);
                     }),
            iterator(input.end()), output.begin(), 2) |
        hpx::remove_copy(par(task).on(exec));

    auto result = tt::sync_wait(std::move(sender));
    HPX_TEST(result.has_value());
    if (!result.has_value())
    {
        return;
    }

    HPX_TEST(hpx::get<0>(*result) == output.end());
    if (hpx::get_os_thread_count() > 1)
    {
        HPX_TEST(max_active_count.load(std::memory_order_relaxed) > 1);
    }
}

template <typename Exception, typename LnPolicy, typename ExPolicy,
    typename IteratorTag>
void test_remove_copy_sender_exception(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>);

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::decorated_iterator<base_iterator, IteratorTag>;
    using scheduler_type = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> input{1, 2, 3, 4};
    std::vector<int> output(input.size());
    auto exec = ex::explicit_scheduler_executor(scheduler_type(ln_policy));

    bool caught_expected_exception = false;
    try
    {
        tt::sync_wait(ex::just(iterator(input.begin(),
                                   []() {
                                       if constexpr (std::is_same_v<Exception,
                                                         std::runtime_error>)
                                       {
                                           throw std::runtime_error("test");
                                       }
                                       else
                                       {
                                           throw std::bad_alloc();
                                       }
                                   }),
                          iterator(input.end()), output.begin(), 2) |
            hpx::remove_copy(ex_policy.on(exec)));
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& errors)
    {
        if constexpr (std::is_same_v<Exception, std::runtime_error>)
        {
            test::test_num_exceptions<ExPolicy, IteratorTag>::call(
                ex_policy, errors);

            bool all_exceptions_expected = errors.begin() != errors.end();
            for (std::exception_ptr const& error : errors)
            {
                bool is_expected_exception = false;
                try
                {
                    std::rethrow_exception(error);
                }
                catch (std::runtime_error const&)
                {
                    is_expected_exception = true;
                }
                catch (...)
                {
                    is_expected_exception = false;
                }
                all_exceptions_expected =
                    all_exceptions_expected && is_expected_exception;
            }
            HPX_TEST(all_exceptions_expected);
            caught_expected_exception = all_exceptions_expected;
        }
        else
        {
            HPX_TEST(false);
        }
    }
    catch (Exception const&)
    {
        if constexpr (std::is_same_v<Exception, std::bad_alloc>)
        {
            caught_expected_exception = true;
        }
        else
        {
            HPX_TEST(false);
        }
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_expected_exception);
}

template <typename IteratorTag>
void remove_copy_sender_test()
{
    using namespace hpx::execution;

    test_remove_copy_scheduler(hpx::launch::sync, seq, IteratorTag{});
    test_remove_copy_scheduler(hpx::launch::sync, unseq, IteratorTag{});
    test_remove_copy_scheduler(hpx::launch::async, par, IteratorTag{});
    test_remove_copy_scheduler(hpx::launch::async, par_unseq, IteratorTag{});

    test_remove_copy_sender(hpx::launch::sync, seq(task), IteratorTag{});
    test_remove_copy_sender(hpx::launch::sync, unseq(task), IteratorTag{});
    test_remove_copy_sender(hpx::launch::async, par(task), IteratorTag{});
    test_remove_copy_sender(hpx::launch::async, par_unseq(task), IteratorTag{});

    test_remove_copy_sender_exception<std::runtime_error>(
        hpx::launch::sync, seq(task), IteratorTag{});
    test_remove_copy_sender_exception<std::runtime_error>(
        hpx::launch::async, par(task), IteratorTag{});
    test_remove_copy_sender_exception<std::bad_alloc>(
        hpx::launch::sync, seq(task), IteratorTag{});
    test_remove_copy_sender_exception<std::bad_alloc>(
        hpx::launch::async, par(task), IteratorTag{});
}

int hpx_main()
{
    remove_copy_sender_test<std::forward_iterator_tag>();
    remove_copy_sender_test<std::random_access_iterator_tag>();
    test_remove_copy_sender_parallel();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
