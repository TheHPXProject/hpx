//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

enum class failure_mode
{
    none,
    immediate_exception,
    immediate_bad_alloc,
    future_exception,
    future_bad_alloc
};

struct legacy_future_algorithm
  : hpx::parallel::detail::algorithm<legacy_future_algorithm, int>
{
    static constexpr bool uses_legacy_futures = true;

    legacy_future_algorithm()
      : algorithm("legacy_future_algorithm")
    {
    }

    template <typename ExPolicy>
    static hpx::parallel::util::detail::algorithm_result_t<ExPolicy, int>
    parallel(ExPolicy&&, std::shared_ptr<std::atomic<int>> calls,
        failure_mode mode)
    {
        ++*calls;

        if (mode == failure_mode::immediate_exception)
            throw std::runtime_error("immediate failure");
        if (mode == failure_mode::immediate_bad_alloc)
            throw std::bad_alloc();
        if (mode == failure_mode::future_exception)
        {
            return hpx::make_exceptional_future<int>(
                std::runtime_error("future failure"));
        }
        if (mode == failure_mode::future_bad_alloc)
            return hpx::make_exceptional_future<int>(std::bad_alloc());

        return hpx::make_ready_future(42);
    }
};

struct value_sender_void_algorithm
  : hpx::parallel::detail::algorithm<value_sender_void_algorithm>
{
    value_sender_void_algorithm()
      : algorithm("value_sender_void_algorithm")
    {
    }

    template <typename ExPolicy>
    static auto parallel(
        ExPolicy&&, std::shared_ptr<std::atomic<int>> calls)
    {
        namespace ex = hpx::execution::experimental;
        return ex::then(ex::just(), [calls = HPX_MOVE(calls)]() mutable {
            ++*calls;
            return 42;
        });
    }
};

template <typename ExPolicy>
void test_legacy_future_value(ExPolicy policy)
{
    auto calls = std::make_shared<std::atomic<int>>(0);
    auto sender = legacy_future_algorithm{}.call(
        policy, calls, failure_mode::none);

    static_assert(sender_test::ex::is_sender_v<decltype(sender)>);
    HPX_TEST_EQ(calls->load(), 0);

    auto result = sender_test::wait(HPX_MOVE(sender));
    HPX_TEST_EQ(calls->load(), 1);
    HPX_TEST_EQ(hpx::get<0>(*result), 42);
}

template <typename ExPolicy>
void test_legacy_future_error(ExPolicy policy, failure_mode mode,
    bool allocation_failure)
{
    auto calls = std::make_shared<std::atomic<int>>(0);
    auto sender = legacy_future_algorithm{}.call(policy, calls, mode);

    HPX_TEST_EQ(calls->load(), 0);

    bool caught = false;
    try
    {
        sender_test::wait(HPX_MOVE(sender));
    }
    catch (std::bad_alloc const&)
    {
        HPX_TEST(allocation_failure);
        caught = true;
    }
    catch (hpx::exception_list const& errors)
    {
        HPX_TEST(!allocation_failure);
        HPX_TEST_EQ(errors.size(), std::size_t{1});
        caught = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught);
    HPX_TEST_EQ(calls->load(), 1);
}

template <typename ExPolicy>
void test_void_sender_result(ExPolicy policy)
{
    auto calls = std::make_shared<std::atomic<int>>(0);
    auto sender = value_sender_void_algorithm{}.call(policy, calls);

    static_assert(sender_test::ex::is_sender_v<decltype(sender)>);
    HPX_TEST_EQ(calls->load(), 0);

    auto result = sender_test::wait(HPX_MOVE(sender));
    static_assert(hpx::tuple_size_v<std::decay_t<decltype(*result)>> == 0);
    HPX_TEST_EQ(calls->load(), 1);
}

int hpx_main()
{
    namespace ex = sender_test::ex;
    using namespace hpx::execution;

    auto executor = ex::explicit_scheduler_executor(
        ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto policy = par(task).on(executor);

    test_legacy_future_value(policy);
    test_legacy_future_error(
        policy, failure_mode::immediate_exception, false);
    test_legacy_future_error(
        policy, failure_mode::immediate_bad_alloc, true);
    test_legacy_future_error(policy, failure_mode::future_exception, false);
    test_legacy_future_error(policy, failure_mode::future_bad_alloc, true);
    test_void_sender_result(policy);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return sender_test::main(argc, argv, hpx_main);
}
