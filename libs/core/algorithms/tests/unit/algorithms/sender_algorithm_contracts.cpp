//  Copyright (c) 2026 the-ivii
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
#include <system_error>
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
    parallel(
        ExPolicy&&, std::shared_ptr<std::atomic<int>> calls, failure_mode mode)
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
    static auto parallel(ExPolicy&&, std::shared_ptr<std::atomic<int>> calls)
    {
        namespace ex = hpx::execution::experimental;
        return ex::then(ex::just(), [calls = HPX_MOVE(calls)]() mutable {
            ++*calls;
            return 42;
        });
    }
};

struct empty_sender_void_algorithm
  : hpx::parallel::detail::algorithm<empty_sender_void_algorithm>
{
    empty_sender_void_algorithm()
      : algorithm("empty_sender_void_algorithm")
    {
    }

    template <typename ExPolicy>
    static auto parallel(ExPolicy&&, std::shared_ptr<std::atomic<int>> calls)
    {
        namespace ex = hpx::execution::experimental;
        return ex::then(
            ex::just(), [calls = HPX_MOVE(calls)]() mutable { ++*calls; });
    }
};

struct error_code_scheduler
{
    using execution_category = hpx::execution::parallel_execution_tag;

    explicit error_code_scheduler(bool fail = false)
      : fail_next(std::make_shared<std::atomic<bool>>(fail))
    {
    }

    explicit error_code_scheduler(std::shared_ptr<std::atomic<bool>> fail_next)
      : fail_next(HPX_MOVE(fail_next))
    {
    }

    std::shared_ptr<std::atomic<bool>> fail_next;

    struct sender;

    sender schedule() const noexcept;

    friend bool operator==(
        error_code_scheduler lhs, error_code_scheduler rhs) noexcept
    {
        return lhs.fail_next == rhs.fail_next;
    }
};

struct error_code_scheduler::sender
{
    using sender_concept = sender_test::ex::sender_t;
    using completion_signatures =
        sender_test::ex::completion_signatures<sender_test::ex::set_value_t(),
            sender_test::ex::set_error_t(std::error_code)>;

    std::shared_ptr<std::atomic<bool>> fail_next;

    template <typename Receiver>
    struct operation_state
    {
        std::decay_t<Receiver> receiver;
        std::shared_ptr<std::atomic<bool>> fail_next;

        void start() & noexcept
        {
            if (fail_next->exchange(false, std::memory_order_relaxed))
            {
                sender_test::ex::set_error(HPX_MOVE(receiver),
                    std::make_error_code(std::errc::operation_canceled));
            }
            else
            {
                sender_test::ex::set_value(HPX_MOVE(receiver));
            }
        }
    };

    template <typename Receiver>
    operation_state<Receiver> connect(Receiver&& receiver) const
    {
        return {HPX_FORWARD(Receiver, receiver), fail_next};
    }

    struct env
    {
        std::shared_ptr<std::atomic<bool>> fail_next;

        error_code_scheduler query(sender_test::ex::get_completion_scheduler_t<
            sender_test::ex::set_value_t>) const noexcept
        {
            return error_code_scheduler{fail_next};
        }
    };

    env get_env() const noexcept
    {
        return {fail_next};
    }
};

inline error_code_scheduler::sender error_code_scheduler::schedule()
    const noexcept
{
    return {fail_next};
}

template <typename ExPolicy>
void test_legacy_future_value(ExPolicy policy)
{
    auto calls = std::make_shared<std::atomic<int>>(0);
    auto sender =
        legacy_future_algorithm{}.call(policy, calls, failure_mode::none);

    static_assert(sender_test::ex::is_sender_v<decltype(sender)>);
    HPX_TEST_EQ(calls->load(), 0);

    auto result = sender_test::wait(HPX_MOVE(sender));
    HPX_TEST_EQ(calls->load(), 1);
    HPX_TEST_EQ(hpx::get<0>(*result), 42);
}

template <typename ExPolicy>
void test_legacy_future_value_sync(ExPolicy policy)
{
    auto calls = std::make_shared<std::atomic<int>>(0);
    auto result =
        legacy_future_algorithm{}.call(policy, calls, failure_mode::none);

    static_assert(!sender_test::ex::is_sender_v<decltype(result)>);
    HPX_TEST_EQ(calls->load(), 1);
    HPX_TEST_EQ(result, 42);
}

template <typename ExPolicy>
void test_legacy_future_error(
    ExPolicy policy, failure_mode mode, bool allocation_failure)
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
void test_legacy_future_error_sync(
    ExPolicy policy, failure_mode mode, bool allocation_failure)
{
    auto calls = std::make_shared<std::atomic<int>>(0);

    bool caught = false;
    try
    {
        legacy_future_algorithm{}.call(policy, calls, mode);
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

template <typename ExPolicy>
void test_empty_void_sender_result(ExPolicy policy)
{
    auto calls = std::make_shared<std::atomic<int>>(0);
    using direct_sender_type =
        decltype(empty_sender_void_algorithm::parallel(policy, calls));
    auto sender = empty_sender_void_algorithm{}.call(policy, calls);

    static_assert(sender_test::ex::is_sender_v<decltype(sender)>);
    static_assert(std::is_same_v<std::decay_t<decltype(sender)>,
        std::decay_t<direct_sender_type>>);
    HPX_TEST_EQ(calls->load(), 0);

    auto result = sender_test::wait(HPX_MOVE(sender));
    static_assert(hpx::tuple_size_v<std::decay_t<decltype(*result)>> == 0);
    HPX_TEST_EQ(calls->load(), 1);
}

void test_scheduler_error_forwarding()
{
    namespace ex = sender_test::ex;
    using namespace hpx::execution;

    {
        auto calls = std::make_shared<std::atomic<int>>(0);
        auto executor =
            ex::explicit_scheduler_executor(error_code_scheduler{false});
        auto sender = legacy_future_algorithm{}.call(
            par(task).on(executor), calls, failure_mode::none);

        auto result = sender_test::wait(HPX_MOVE(sender));
        HPX_TEST_EQ(hpx::get<0>(*result), 42);
        HPX_TEST_EQ(calls->load(), 1);
    }

    {
        auto calls = std::make_shared<std::atomic<int>>(0);
        auto executor =
            ex::explicit_scheduler_executor(error_code_scheduler{true});
        auto sender = legacy_future_algorithm{}.call(
            par(task).on(executor), calls, failure_mode::none);

        bool error_received = false;
        auto handled =
            ex::upon_error(HPX_MOVE(sender), [&error_received](auto&& error) {
                using error_type = std::decay_t<decltype(error)>;
                if constexpr (std::is_same_v<error_type, std::error_code>)
                {
                    HPX_TEST_EQ(error,
                        std::make_error_code(std::errc::operation_canceled));
                    error_received = true;
                }
                else
                {
                    HPX_TEST(false);
                }
                return 0;
            });

        auto result = sender_test::wait(HPX_MOVE(handled));
        HPX_TEST_EQ(hpx::get<0>(*result), 0);
        HPX_TEST(error_received);
        HPX_TEST_EQ(calls->load(), 0);
    }
}

int hpx_main()
{
    namespace ex = sender_test::ex;
    using namespace hpx::execution;

    auto executor = ex::explicit_scheduler_executor(
        ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto task_policy = par(task).on(executor);

    test_legacy_future_value(task_policy);
    test_legacy_future_error(
        task_policy, failure_mode::immediate_exception, false);
    test_legacy_future_error(
        task_policy, failure_mode::immediate_bad_alloc, true);
    test_legacy_future_error(
        task_policy, failure_mode::future_exception, false);
    test_legacy_future_error(task_policy, failure_mode::future_bad_alloc, true);
    test_void_sender_result(task_policy);
    test_empty_void_sender_result(task_policy);

    test_legacy_future_value(par_unseq(task).on(executor));
    auto sync_policy = par.on(executor);
    test_legacy_future_value_sync(sync_policy);
    test_legacy_future_error_sync(
        sync_policy, failure_mode::immediate_exception, false);
    test_legacy_future_error_sync(
        sync_policy, failure_mode::immediate_bad_alloc, true);
    test_legacy_future_error_sync(
        sync_policy, failure_mode::future_exception, false);
    test_legacy_future_error_sync(
        sync_policy, failure_mode::future_bad_alloc, true);
    test_scheduler_error_forwarding();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return sender_test::main(argc, argv, hpx_main);
}
