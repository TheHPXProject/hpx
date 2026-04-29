//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

// for is_sender
#include <hpx/modules/execution_base.hpp>

#include <type_traits>
#include <utility>

namespace hpx::parallel::util::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T,
        typename Enable = void>
    struct algorithm_result_impl;

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T>
    struct algorithm_result_impl<ExPolicy, T,
        std::enable_if_t<!hpx::is_async_execution_policy_v<ExPolicy> &&
            !hpx::execution_policy_has_scheduler_executor_v<ExPolicy>>>
    {
        // The return type of the initiating function.
        using type = T;

        // Obtain initiating function's return type.
        static constexpr type get() noexcept(
            std::is_nothrow_default_constructible_v<T>)
        {
            return T();
        }

        template <typename T_>
        static constexpr auto get(T_&& t)
        {
            return HPX_FORWARD(T_, t);
        }

        template <typename T_>
        static auto get(hpx::future<T_>&& t)
        {
            return t.get();
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct algorithm_result_impl<ExPolicy, void,
        std::enable_if_t<!hpx::is_async_execution_policy_v<ExPolicy> &&
            !hpx::execution_policy_has_scheduler_executor_v<ExPolicy>>>
    {
        // The return type of the initiating function.
        using type = void;

        // Obtain initiating function's return type.
        static constexpr void get() noexcept {}

        static constexpr void get(hpx::util::unused_type) noexcept {}

        static void get(hpx::future<void>&& t)
        {
            t.get();
        }

        template <typename T>
        static void get(hpx::future<T>&& t)
        {
            t.get();
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T>
    struct algorithm_result_impl<ExPolicy, T,
        std::enable_if_t<hpx::is_async_execution_policy_v<ExPolicy> &&
            !hpx::execution_policy_has_scheduler_executor_v<ExPolicy>>>
    {
        // The return type of the initiating function.
        using type = hpx::future<T>;

        // Obtain initiating function's return type.
        static type get(T&& t)
        {
            return hpx::make_ready_future(HPX_MOVE(t));
        }

        static type get(hpx::future<T>&& t)
        {
            return HPX_MOVE(t);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct algorithm_result_impl<ExPolicy, void,
        std::enable_if_t<hpx::is_async_execution_policy_v<ExPolicy> &&
            !hpx::execution_policy_has_scheduler_executor_v<ExPolicy>>>
    {
        // The return type of the initiating function.
        using type = hpx::future<void>;

        // Obtain initiating function's return type.
        static type get()
        {
            return hpx::make_ready_future();
        }

        static type get(hpx::util::unused_type)
        {
            return hpx::make_ready_future();
        }

        static type get(hpx::future<void>&& t) noexcept
        {
            return HPX_MOVE(t);
        }

        template <typename T>
        static type get(hpx::future<T>&& t) noexcept
        {
            return hpx::future<void>(HPX_MOVE(t));
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T>
    struct algorithm_result_impl<ExPolicy, T,
        std::enable_if_t<!hpx::is_async_execution_policy_v<ExPolicy> &&
            hpx::execution_policy_has_scheduler_executor_v<ExPolicy>>>
    {
        // The return type of the initiating function.
        using type = T;

        static constexpr type get() noexcept(
            std::is_nothrow_default_constructible_v<T>)
        {
            return T();
        }

        template <typename T_>
        static constexpr auto get(T_&& t)
        {
            namespace ex = hpx::execution::experimental;
            if constexpr (ex::is_sender_v<T_>)
            {
                namespace tt = hpx::this_thread::experimental;
                auto result = tt::sync_wait(HPX_FORWARD(T_, t));
                if constexpr (hpx::tuple_size_v<
                                  std::decay_t<decltype(*result)>> == 0)
                {
                    return;
                }
                else
                {
                    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                    return hpx::get<0>(*result);
                }
            }
            else
            {
                return HPX_FORWARD(T_, t);
            }
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct algorithm_result_impl<ExPolicy, void,
        std::enable_if_t<!hpx::is_async_execution_policy_v<ExPolicy> &&
            hpx::execution_policy_has_scheduler_executor_v<ExPolicy>>>
    {
        // The return type of the initiating function.
        using type = void;

        static constexpr type get() noexcept {}

        template <typename T>
        static constexpr auto get(T&& t)
        {
            namespace ex = hpx::execution::experimental;
            if constexpr (ex::is_sender_v<T>)
            {
                namespace tt = hpx::this_thread::experimental;
                tt::sync_wait(HPX_FORWARD(T, t));
            }
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T>
    struct algorithm_result_impl<ExPolicy, T,
        std::enable_if_t<hpx::is_async_execution_policy_v<ExPolicy> &&
            hpx::execution_policy_has_scheduler_executor_v<ExPolicy>>>
    {
        // Due to the nature of senders, this only serves as a dummy.
        using type =
            decltype(hpx::execution::experimental::just(std::declval<T>()));

        template <typename T_>
        static constexpr auto get(T_&& t)
        {
            namespace ex = hpx::execution::experimental;
            if constexpr (ex::is_sender_v<T_>)
            {
                return HPX_FORWARD(T_, t);
            }
            else
            {
                return ex::just(HPX_FORWARD(T_, t));
            }
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct algorithm_result_impl<ExPolicy, void,
        std::enable_if_t<hpx::is_async_execution_policy_v<ExPolicy> &&
            hpx::execution_policy_has_scheduler_executor_v<ExPolicy>>>
    {
        // Due to the nature of senders, this only serves as a dummy.
        using type = decltype(hpx::execution::experimental::just());

        template <typename T_>
        static constexpr auto get(T_&& t)
        {
            namespace ex = hpx::execution::experimental;
            if constexpr (ex::is_sender_v<T_>)
            {
                return HPX_FORWARD(T_, t);
            }
            else
            {
                return ex::just(HPX_FORWARD(T_, t));
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T = void>
    struct algorithm_result : algorithm_result_impl<std::decay_t<ExPolicy>, T>
    {
        static_assert(!std::is_lvalue_reference_v<T>,
            "T shouldn't be a lvalue reference");
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T = void>
    using algorithm_result_t = typename algorithm_result<ExPolicy, T>::type;

    ///////////////////////////////////////////////////////////////////////////

    HPX_CXX_CORE_EXPORT template <typename U, typename Conv>
    // clang-format off
        requires (
           !hpx::execution::experimental::is_sender_v<U> &&
            hpx::is_invocable_v<Conv, U>
        )
    // clang-format on
    constexpr hpx::util::invoke_result_t<Conv, U> convert_to_result(
        U&& val, Conv&& conv)
    {
        return HPX_INVOKE(conv, val);
    }

    HPX_CXX_CORE_EXPORT template <typename Sender, typename Conv>
        requires(hpx::execution::experimental::is_sender_v<Sender>)
    constexpr decltype(auto) convert_to_result(Sender&& sender, Conv&& conv)
    {
        return hpx::execution::experimental::then(HPX_FORWARD(Sender, sender),
            [conv = HPX_FORWARD(Conv, conv)](auto&& value) mutable {
                return HPX_INVOKE(conv, HPX_FORWARD(decltype(value), value));
            });
    }

    HPX_CXX_CORE_EXPORT template <typename U, typename Conv>
        requires(hpx::is_invocable_v<Conv, U>)
    hpx::future<hpx::util::invoke_result_t<Conv, U>> convert_to_result(
        hpx::future<U>&& f, Conv&& conv)
    {
        using result_type = hpx::util::invoke_result_t<Conv, U>;

        return hpx::make_future<result_type>(
            HPX_MOVE(f), HPX_FORWARD(Conv, conv));
    }
}    // namespace hpx::parallel::util::detail
