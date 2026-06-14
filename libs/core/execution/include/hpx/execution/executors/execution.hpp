//  Copyright (c) 2017-2026 Hartmut Kaiser
//  Copyright (c) 2017 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/execution.hpp

#pragma once

#include <hpx/config.hpp>
// Necessary to avoid circular include
#include <hpx/modules/execution_base.hpp>

#include <hpx/assert.hpp>
#include <hpx/execution/detail/future_exec.hpp>
#include <hpx/execution/executors/fused_bulk_execute.hpp>
#include <hpx/execution/traits/executor_traits.hpp>
#include <hpx/execution/traits/future_then_result_exec.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/type_support.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::execution {

    /// \cond NOINTERNAL

    // customization point for OneWayExecutor interface
    // execute()
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename Executor, typename F, typename... Ts>
        struct sync_execute_not_callable;

        ///////////////////////////////////////////////////////////////////////
        // default implementation of the sync_execute() customization point
        template <typename Executor, typename F, typename... Ts>
        HPX_FORCEINLINE auto sync_execute_dispatch(
            hpx::traits::detail::wrap_int, Executor&& /* exec */, F&& /* f */,
            Ts&&... /* ts */) -> sync_execute_not_callable<Executor, F, Ts...>
        {
            return sync_execute_not_callable<Executor, F, Ts...>{};
        }

        template <typename OneWayExecutor, typename F, typename... Ts,
            typename Enable = std::enable_if_t<std::is_invocable_v<
                hpx::parallel::execution::sync_execute_t const&,
                OneWayExecutor&&, F&&, Ts&&...>>>
        HPX_FORCEINLINE decltype(auto) sync_execute_dispatch(
            int, OneWayExecutor&& exec, F&& f, Ts&&... ts)
        {
            return hpx::parallel::execution::sync_execute(
                exec, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename OneWayExecutor, typename F, typename... Ts,
            typename Enable = std::enable_if_t<!std::is_invocable_v<
                hpx::parallel::execution::sync_execute_t const&,
                OneWayExecutor&&, F&&, Ts&&...>>>
        HPX_DEPRECATED_V(1, 9,
            "Exposing sync_execute() from an executor is deprecated, please "
            "expose this functionality through a corresponding member function")
        auto sync_execute_dispatch(int, OneWayExecutor&& exec, F&& f,
            Ts&&... ts) -> decltype(exec.sync_execute(HPX_FORWARD(F, f),
            HPX_FORWARD(Ts, ts)...))
        {
            return exec.sync_execute(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        ///////////////////////////////////////////////////////////////////////
        // emulate async_execute() on OneWayExecutors
        template <typename Executor>
        struct async_execute_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_one_way_executor_v<Executor> &&
                !hpx::traits::is_two_way_executor_v<Executor>>>
        {
            template <typename OneWayExecutor, typename F, typename... Ts>
            HPX_FORCEINLINE static auto call(OneWayExecutor&& exec, F&& f,
                Ts&&... ts) -> hpx::future<decltype(sync_execute_dispatch(0,
                HPX_FORWARD(OneWayExecutor, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...))>
            {
                // clang-format off
                static constexpr bool is_void =
                    std::is_void_v<decltype(sync_execute_dispatch(0,
                        HPX_FORWARD(OneWayExecutor, exec), HPX_FORWARD(F, f),
                        HPX_FORWARD(Ts, ts)...))>;
                // clang-format on

                if constexpr (is_void)
                {
                    sync_execute_dispatch(0, HPX_FORWARD(OneWayExecutor, exec),
                        HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
                    return hpx::make_ready_future();
                }
                else
                {
                    return hpx::make_ready_future(sync_execute_dispatch(0,
                        HPX_FORWARD(OneWayExecutor, exec), HPX_FORWARD(F, f),
                        HPX_FORWARD(Ts, ts)...));
                }
            }

            template <typename OneWayExecutor, typename F, typename... Ts>
            struct result
            {
                using type = decltype(call(std::declval<OneWayExecutor>(),
                    std::declval<F>(), std::declval<Ts>()...));
            };
        };

        // emulate sync_execute() on OneWayExecutors
        template <typename Executor>
        struct sync_execute_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_one_way_executor_v<Executor> &&
                !hpx::traits::is_two_way_executor_v<Executor>>>
        {
            template <typename OneWayExecutor, typename F, typename... Ts>
            HPX_FORCEINLINE static auto call(OneWayExecutor&& exec, F&& f,
                Ts&&... ts) -> decltype(sync_execute_dispatch(0,
                HPX_FORWARD(OneWayExecutor, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...))
            {
                return sync_execute_dispatch(0,
                    HPX_FORWARD(OneWayExecutor, exec), HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...);
            }

            template <typename OneWayExecutor, typename F, typename... Ts>
            struct result
            {
                using type = decltype(call(std::declval<OneWayExecutor>(),
                    std::declval<F>(), std::declval<Ts>()...));
            };
        };

        // emulate then_execute() on OneWayExecutors
        template <typename Executor>
        struct then_execute_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_one_way_executor_v<Executor> &&
                !hpx::traits::is_two_way_executor_v<Executor>>>
        {
            template <typename OneWayExecutor, typename F, typename Future,
                typename... Ts>
            HPX_FORCEINLINE static hpx::future<
                hpx::util::detail::invoke_deferred_result_t<F, Future, Ts...>>
            call(OneWayExecutor&& exec, F&& f, Future&& predecessor, Ts&&... ts)
            {
                using result_type =
                    hpx::util::detail::invoke_deferred_result_t<F, Future,
                        Ts...>;

                auto func = hpx::util::one_shot(
                    hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));

                hpx::traits::detail::shared_state_ptr_t<result_type> p =
                    lcos::detail::make_continuation_exec<result_type>(
                        HPX_FORWARD(Future, predecessor),
                        HPX_FORWARD(OneWayExecutor, exec), HPX_MOVE(func));

                return hpx::traits::future_access<
                    hpx::future<result_type>>::create(HPX_MOVE(p));
            }

            template <typename OneWayExecutor, typename F, typename Future,
                typename... Ts>
            struct result
            {
                // clang-format off
                using type = decltype(call(std::declval<OneWayExecutor>(),
                    std::declval<F>(), std::declval<Future>(),
                    std::declval<Ts>()...));
                // clang-format on
            };
        };

        ///////////////////////////////////////////////////////////////////////
        // emulate post() on OneWayExecutors
        template <typename Executor>
        struct post_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_one_way_executor_v<Executor> &&
                !hpx::traits::is_two_way_executor_v<Executor> &&
                !hpx::traits::is_never_blocking_one_way_executor_v<Executor>>>
        {
            template <typename OneWayExecutor, typename F, typename... Ts>
            HPX_FORCEINLINE static void call_impl(hpx::traits::detail::wrap_int,
                OneWayExecutor&& exec, F&& f, Ts&&... ts)
            {
                // execute synchronously
                sync_execute_dispatch(0, HPX_FORWARD(OneWayExecutor, exec),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            // dispatch to V1 executors
            template <typename OneWayExecutor, typename F, typename... Ts,
                typename Enable = std::enable_if_t<
                    std::is_invocable_v<hpx::parallel::execution::post_t const&,
                        OneWayExecutor&&, F&&, Ts&&...>>>
            HPX_FORCEINLINE static decltype(auto) call_impl(
                int, OneWayExecutor&& exec, F&& f, Ts&&... ts)
            {
                // use post, if exposed
                return hpx::parallel::execution::post(
                    exec, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename OneWayExecutor, typename F, typename... Ts,
                typename Enable = std::enable_if_t<!std::is_invocable_v<
                    hpx::parallel::execution::post_t const&, OneWayExecutor&&,
                    F&&, Ts&&...>>>
            HPX_DEPRECATED_V(1, 9,
                "Exposing post() from an executor is deprecated, please "
                "expose this functionality through a corresponding "
                "member function")
            HPX_FORCEINLINE static auto call_impl(int, OneWayExecutor&& exec,
                F&& f, Ts&&... ts) -> decltype(exec.post(HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...))
            {
                // use post, if exposed
                return exec.post(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename OneWayExecutor, typename F, typename... Ts>
            HPX_FORCEINLINE static auto call(OneWayExecutor&& exec, F&& f,
                Ts&&... ts) -> decltype(call_impl(0, exec, HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...))
            {
                // simply discard the returned future
                return call_impl(0, HPX_FORWARD(OneWayExecutor, exec),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename OneWayExecutor, typename F, typename... Ts>
            struct result
            {
                using type = decltype(call(std::declval<OneWayExecutor>(),
                    std::declval<F>(), std::declval<Ts>()...));
            };
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // customization points for TwoWayExecutor interface
    // async_execute(), sync_execute(), then_execute()
    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        template <typename Executor, typename F, typename... Ts>
        struct async_execute_not_callable;

        ///////////////////////////////////////////////////////////////////////
        // default implementation of the async_execute() customization point
        template <typename Executor, typename F, typename... Ts>
        HPX_FORCEINLINE auto async_execute_dispatch(
            hpx::traits::detail::wrap_int, Executor&& /* exec */, F&& /* f */,
            Ts&&... /* ts */) -> async_execute_not_callable<Executor, F, Ts...>
        {
            return async_execute_not_callable<Executor, F, Ts...>{};
        }

        template <typename TwoWayExecutor, typename F, typename... Ts,
            typename Enable = std::enable_if_t<std::is_invocable_v<
                hpx::parallel::execution::async_execute_t const&,
                TwoWayExecutor&&, F&&, Ts&&...>>>
        HPX_FORCEINLINE decltype(auto) async_execute_dispatch(
            int, TwoWayExecutor&& exec, F&& f, Ts&&... ts)
        {
            return hpx::parallel::execution::async_execute(
                exec, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename TwoWayExecutor, typename F, typename... Ts,
            typename Enable = std::enable_if_t<!std::is_invocable_v<
                hpx::parallel::execution::async_execute_t const&,
                TwoWayExecutor&&, F&&, Ts&&...>>>
        HPX_DEPRECATED_V(1, 9,
            "Exposing async_execute() from an executor is deprecated, please "
            "expose this functionality through a corresponding member function")
        HPX_FORCEINLINE auto async_execute_dispatch(int, TwoWayExecutor&& exec,
            F&& f, Ts&&... ts) -> decltype(exec.async_execute(HPX_FORWARD(F, f),
            HPX_FORWARD(Ts, ts)...))
        {
            return exec.async_execute(
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Executor>
        struct async_execute_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_two_way_executor_v<Executor>>>
        {
            template <typename TwoWayExecutor, typename F, typename... Ts>
            HPX_FORCEINLINE static auto call(TwoWayExecutor&& exec, F&& f,
                Ts&&... ts) -> decltype(async_execute_dispatch(0,
                HPX_FORWARD(TwoWayExecutor, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...))
            {
                return async_execute_dispatch(0,
                    HPX_FORWARD(TwoWayExecutor, exec), HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...);
            }

            template <typename TwoWayExecutor, typename F, typename... Ts>
            struct result
            {
                using type = decltype(call(std::declval<TwoWayExecutor>(),
                    std::declval<F>(), std::declval<Ts>()...));
            };
        };

        ///////////////////////////////////////////////////////////////////////
        // default implementation of the sync_execute() customization point
        template <typename Executor>
        struct sync_execute_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_two_way_executor_v<Executor>>>
        {
            // fall-back: emulate sync_execute using async_execute
            template <typename TwoWayExecutor, typename F, typename... Ts>
            HPX_FORCEINLINE static auto call_impl(hpx::traits::detail::wrap_int,
                TwoWayExecutor&& exec, F&& f, Ts&&... ts)
                -> hpx::util::invoke_result_t<F, Ts...>
            {
                static constexpr bool is_void = std::is_void_v<
                    hpx::util::detail::invoke_deferred_result_t<F, Ts...>>;

                if constexpr (is_void)
                {
                    async_execute_dispatch(0, HPX_FORWARD(TwoWayExecutor, exec),
                        HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)
                        .get();
                }
                else
                {
                    try
                    {
                        using result_type =
                            hpx::util::detail::invoke_deferred_result_t<F,
                                Ts...>;

                        // use async execution, wait for result, propagate exceptions
                        return async_execute_dispatch(0,
                            HPX_FORWARD(TwoWayExecutor, exec),
                            [&]() -> result_type {
                                return HPX_INVOKE(
                                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
                            })
                            .get();
                    }
                    catch (std::bad_alloc const& ba)
                    {
                        throw ba;
                    }
                    catch (...)
                    {
                        // note: constructor doesn't lock/suspend
                        throw hpx::exception_list(std::current_exception());
                    }
                }
            }

            template <typename TwoWayExecutor, typename F, typename... Ts,
                typename Enable = std::enable_if_t<std::is_invocable_v<
                    hpx::parallel::execution::sync_execute_t const&,
                    TwoWayExecutor&&, F&&, Ts&&...>>>
            HPX_FORCEINLINE static decltype(auto) call_impl(
                int, TwoWayExecutor&& exec, F&& f, Ts&&... ts)
            {
                return hpx::parallel::execution::sync_execute(
                    exec, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename TwoWayExecutor, typename F, typename... Ts,
                typename Enable = std::enable_if_t<!std::is_invocable_v<
                    hpx::parallel::execution::sync_execute_t const&,
                    TwoWayExecutor&&, F&&, Ts&&...>>>
            HPX_DEPRECATED_V(1, 9,
                "Exposing sync_execute() from an executor is deprecated, "
                "please expose this functionality through a corresponding "
                "member function")
            HPX_FORCEINLINE static auto call_impl(
                int, TwoWayExecutor&& exec, F&& f, Ts&&... ts)
                -> decltype(exec.sync_execute(
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
            {
                return exec.sync_execute(
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename TwoWayExecutor, typename F, typename... Ts>
            HPX_FORCEINLINE static auto call(
                TwoWayExecutor&& exec, F&& f, Ts&&... ts)
                -> decltype(call_impl(0, HPX_FORWARD(TwoWayExecutor, exec),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
            {
                return call_impl(0, HPX_FORWARD(TwoWayExecutor, exec),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename TwoWayExecutor, typename F, typename... Ts>
            struct result
            {
                using type = decltype(call(std::declval<TwoWayExecutor>(),
                    std::declval<F>(), std::declval<Ts>()...));
            };
        };

        ///////////////////////////////////////////////////////////////////////
        // then_execute()

        template <typename Executor>
        struct then_execute_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_two_way_executor_v<Executor>>>
        {
            template <typename TwoWayExecutor, typename F, typename Future,
                typename... Ts>
            static hpx::future<
                hpx::util::detail::invoke_deferred_result_t<F, Future, Ts...>>
            call_impl(hpx::traits::detail::wrap_int, TwoWayExecutor&& exec,
                F&& f, Future&& predecessor, Ts&&... ts)
            {
                using result_type =
                    hpx::util::detail::invoke_deferred_result_t<F, Future,
                        Ts...>;

                auto func = hpx::util::one_shot(
                    hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));

                hpx::traits::detail::shared_state_ptr_t<result_type> p =
                    lcos::detail::make_continuation_exec<result_type>(
                        HPX_FORWARD(Future, predecessor),
                        HPX_FORWARD(TwoWayExecutor, exec), HPX_MOVE(func));

                return hpx::traits::future_access<
                    hpx::future<result_type>>::create(HPX_MOVE(p));
            }

            template <typename TwoWayExecutor, typename F, typename Future,
                typename... Ts,
                typename Enable = std::enable_if_t<std::is_invocable_v<
                    hpx::parallel::execution::then_execute_t const&,
                    TwoWayExecutor&&, F&&, Future&&, Ts&&...>>>
            HPX_FORCEINLINE static decltype(auto) call_impl(int,
                TwoWayExecutor&& exec, F&& f, Future&& predecessor, Ts&&... ts)
            {
                return hpx::parallel::execution::then_execute(exec,
                    HPX_FORWARD(F, f), HPX_FORWARD(Future, predecessor),
                    HPX_FORWARD(Ts, ts)...);
            }

            template <typename TwoWayExecutor, typename F, typename Future,
                typename... Ts,
                typename Enable = std::enable_if_t<!std::is_invocable_v<
                    hpx::parallel::execution::then_execute_t const&,
                    TwoWayExecutor&&, F&&, Future&&, Ts&&...>>>
            HPX_DEPRECATED_V(1, 9,
                "Exposing then_execute() from an executor is deprecated, "
                "please expose this functionality through a corresponding "
                "member function")
            HPX_FORCEINLINE static auto call_impl(int, TwoWayExecutor&& exec,
                F&& f, Future&& predecessor, Ts&&... ts)
                -> decltype(exec.then_execute(HPX_FORWARD(F, f),
                    HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...))
            {
                return exec.then_execute(HPX_FORWARD(F, f),
                    HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
            }

            template <typename TwoWayExecutor, typename F, typename Future,
                typename... Ts>
            HPX_FORCEINLINE static auto call(TwoWayExecutor&& exec, F&& f,
                Future&& predecessor, Ts&&... ts) -> decltype(call_impl(0,
                HPX_FORWARD(TwoWayExecutor, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...))
            {
                return call_impl(0, HPX_FORWARD(TwoWayExecutor, exec),
                    HPX_FORWARD(F, f), HPX_FORWARD(Future, predecessor),
                    HPX_FORWARD(Ts, ts)...);
            }

            template <typename TwoWayExecutor, typename F, typename Future,
                typename... Ts>
            struct result
            {
                // clang-format off
                using type = decltype(call(std::declval<TwoWayExecutor>(),
                    std::declval<F>(), std::declval<Future>(),
                    std::declval<Ts>()...));
                // clang-format on
            };
        };

        ///////////////////////////////////////////////////////////////////////
        // emulate post() on TwoWayExecutors
        template <typename Executor>
        struct post_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_two_way_executor_v<Executor> &&
                !hpx::traits::is_never_blocking_one_way_executor_v<Executor>>>
        {
            template <typename TwoWayExecutor, typename F, typename... Ts>
            HPX_FORCEINLINE static void call_impl(hpx::traits::detail::wrap_int,
                TwoWayExecutor&& exec, F&& f, Ts&&... ts)
            {
                // simply discard the returned future
                hpx::parallel::execution::async_execute(
                    exec, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            // dispatch to V1 executors
            template <typename TwoWayExecutor, typename F, typename... Ts,
                typename Enable = std::enable_if_t<
                    std::is_invocable_v<hpx::parallel::execution::post_t const&,
                        TwoWayExecutor&&, F&&, Ts&&...>>>
            HPX_FORCEINLINE static decltype(auto) call_impl(
                int, TwoWayExecutor&& exec, F&& f, Ts&&... ts)
            {
                // use post, if exposed
                return hpx::parallel::execution::post(
                    exec, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename TwoWayExecutor, typename F, typename... Ts,
                typename Enable = std::enable_if_t<!std::is_invocable_v<
                    hpx::parallel::execution::post_t const&, TwoWayExecutor&&,
                    F&&, Ts&&...>>>
            HPX_DEPRECATED_V(1, 9,
                "Exposing post() from an executor is deprecated, please "
                "expose this functionality through a corresponding "
                "member function")
            HPX_FORCEINLINE static auto call_impl(int, TwoWayExecutor&& exec,
                F&& f, Ts&&... ts) -> decltype(exec.post(HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...))
            {
                // use post, if exposed
                return exec.post(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename TwoWayExecutor, typename F, typename... Ts>
            HPX_FORCEINLINE static auto call(
                TwoWayExecutor&& exec, F&& f, Ts&&... ts)
                -> decltype(call_impl(0, HPX_FORWARD(TwoWayExecutor, exec),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
            {
                return call_impl(0, HPX_FORWARD(TwoWayExecutor, exec),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename TwoWayExecutor, typename F, typename... Ts>
            struct result
            {
                using type = decltype(call(std::declval<TwoWayExecutor>(),
                    std::declval<F>(), std::declval<Ts>()...));
            };
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // post()
    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        // default implementation of the post() customization point

        template <typename Executor, typename F, typename... Ts>
        struct post_not_callable;

        template <typename Executor, typename F, typename... Ts>
        HPX_FORCEINLINE auto post_dispatch(hpx::traits::detail::wrap_int,
            Executor&& /* exec */, F&& /* f */, Ts&&... /* ts */)
            -> post_not_callable<Executor, F, Ts...>
        {
            return post_not_callable<Executor, F, Ts...>{};
        }

        // default implementation of the post() customization point
        template <typename NonBlockingOneWayExecutor, typename F,
            typename... Ts,
            typename Enable = std::enable_if_t<
                std::is_invocable_v<hpx::parallel::execution::post_t const&,
                    NonBlockingOneWayExecutor&&, F&&, Ts&&...>>>
        HPX_FORCEINLINE decltype(auto) post_dispatch(
            int, NonBlockingOneWayExecutor&& exec, F&& f, Ts&&... ts)
        {
            return hpx::parallel::execution::post(
                exec, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename NonBlockingOneWayExecutor, typename F,
            typename... Ts,
            typename Enable = std::enable_if_t<
                !std::is_invocable_v<hpx::parallel::execution::post_t const&,
                    NonBlockingOneWayExecutor&&, F&&, Ts&&...>>>
        HPX_DEPRECATED_V(1, 9,
            "Exposing post() from an executor is deprecated, please "
            "expose this functionality through a corresponding member function")
        HPX_FORCEINLINE auto post_dispatch(
            int, NonBlockingOneWayExecutor&& exec, F&& f, Ts&&... ts)
            -> decltype(exec.post(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
        {
            return exec.post(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Executor>
        struct post_fn_helper<Executor,
            std::enable_if_t<
                hpx::traits::is_never_blocking_one_way_executor_v<Executor>>>
        {
            template <typename NonBlockingOneWayExecutor, typename F,
                typename... Ts>
            HPX_FORCEINLINE static auto call(NonBlockingOneWayExecutor&& exec,
                F&& f, Ts&&... ts) -> decltype(post_dispatch(0,
                HPX_FORWARD(NonBlockingOneWayExecutor, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...))
            {
                return post_dispatch(0,
                    HPX_FORWARD(NonBlockingOneWayExecutor, exec),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename NonBlockingOneWayExecutor, typename F,
                typename... Ts>
            struct result
            {
                using type =
                    decltype(call(std::declval<NonBlockingOneWayExecutor>(),
                        std::declval<F>(), std::declval<Ts>()...));
            };
        };
    }    // namespace detail
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    // customization points for BulkTwoWayExecutor interface
    // bulk_async_execute(), bulk_sync_execute(), bulk_then_execute()

    /// \cond NOINTERNAL
    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        // bulk_async_execute()

        ///////////////////////////////////////////////////////////////////////
        // default implementation of the bulk_async_execute() customization point

        template <typename Executor, typename F, typename Shape, typename... Ts>
        struct bulk_async_execute_not_callable;

        template <typename Executor, typename F, typename Shape, typename... Ts>
        auto bulk_async_execute_dispatch(hpx::traits::detail::wrap_int,
            Executor&& /* exec */, F&& /* f */, Shape const& /* shape */,
            Ts&&... /* ts */)
            -> bulk_async_execute_not_callable<Executor, F, Shape, Ts...>
        {
            return bulk_async_execute_not_callable<Executor, F, Shape, Ts...>{};
        }

        template <typename BulkTwoWayExecutor, typename F, typename Shape,
            typename... Ts,
            typename Enable = std::enable_if_t<std::is_invocable_v<
                hpx::parallel::execution::bulk_async_execute_t const&,
                BulkTwoWayExecutor&&, F&&, Shape const&, Ts&&...>>>
        HPX_FORCEINLINE decltype(auto) bulk_async_execute_dispatch(int,
            BulkTwoWayExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
        {
            return hpx::parallel::execution::bulk_async_execute(
                exec, HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        template <typename BulkTwoWayExecutor, typename F, typename Shape,
            typename... Ts,
            typename Enable = std::enable_if_t<!std::is_invocable_v<
                hpx::parallel::execution::bulk_async_execute_t const&,
                BulkTwoWayExecutor&&, F&&, Shape const&, Ts&&...>>>
        HPX_DEPRECATED_V(1, 9,
            "Exposing bulk_async_execute() from an executor is deprecated, "
            "please expose this functionality through a corresponding "
            "member function")
        HPX_FORCEINLINE auto bulk_async_execute_dispatch(int,
            BulkTwoWayExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
            -> decltype(exec.bulk_async_execute(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...))
        {
            return exec.bulk_async_execute(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        HPX_CXX_CORE_EXPORT template <typename F, typename Shape,
            typename... Ts>
        struct bulk_function_result
        {
            using value_type =
                typename hpx::traits::range_traits<Shape>::value_type;
            using type = hpx::util::detail::invoke_deferred_result_t<F,
                value_type, Ts...>;
        };

        HPX_CXX_CORE_EXPORT template <typename F, typename Shape,
            typename... Ts>
        using bulk_function_result_t =
            typename bulk_function_result<F, Shape, Ts...>::type;

        template <typename Executor>
        struct bulk_async_execute_fn_helper<Executor,
            std::enable_if_t<
                (hpx::traits::is_one_way_executor_v<Executor> ||
                    hpx::traits::is_two_way_executor_v<Executor>) &&
                !hpx::traits::is_bulk_two_way_executor_v<Executor>>>
        {
            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            static auto call_impl(hpx::traits::detail::wrap_int,
                BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
                -> std::vector<hpx::traits::executor_future_t<Executor,
                    bulk_function_result_t<F, Shape, Ts...>, Ts...>>
            {
                std::vector<hpx::traits::executor_future_t<Executor,
                    bulk_function_result_t<F, Shape, Ts...>, Ts...>>
                    results;
                results.reserve(hpx::util::size(shape));

                for (auto const& elem : shape)
                {
                    results.push_back(
                        execution::async_execute(exec, f, elem, ts...));
                }

                return results;
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts,
                typename Enable = std::enable_if_t<std::is_invocable_v<
                    hpx::parallel::execution::bulk_async_execute_t const&,
                    BulkExecutor&&, F&&, Shape const&, Ts&&...>>>
            HPX_FORCEINLINE static decltype(auto) call_impl(
                int, BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
            {
                return hpx::parallel::execution::bulk_async_execute(
                    exec, HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts,
                typename Enable = std::enable_if_t<!std::is_invocable_v<
                    hpx::parallel::execution::bulk_async_execute_t const&,
                    BulkExecutor&&, F&&, Shape const&, Ts&&...>>>
            HPX_DEPRECATED_V(1, 9,
                "Exposing bulk_async_execute() from an executor is deprecated, "
                "please expose this functionality through a corresponding "
                "member function")
            HPX_FORCEINLINE static auto call_impl(
                int, BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
                -> decltype(exec.bulk_async_execute(
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...))
            {
                return exec.bulk_async_execute(
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            HPX_FORCEINLINE static auto call(
                BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
                -> decltype(call_impl(0, HPX_FORWARD(BulkExecutor, exec),
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...))
            {
                return call_impl(0, HPX_FORWARD(BulkExecutor, exec),
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            struct result
            {
                // clang-format off
                using type = decltype(call(std::declval<BulkExecutor>(),
                    std::declval<F>(), std::declval<Shape const&>(),
                    std::declval<Ts>()...));
                // clang-format on
            };
        };

        template <typename Executor>
        struct bulk_async_execute_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_bulk_two_way_executor_v<Executor>>>
        {
            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            HPX_FORCEINLINE static auto call(
                BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
                -> decltype(bulk_async_execute_dispatch(0,
                    HPX_FORWARD(BulkExecutor, exec), HPX_FORWARD(F, f), shape,
                    HPX_FORWARD(Ts, ts)...))
            {
                return bulk_async_execute_dispatch(0,
                    HPX_FORWARD(BulkExecutor, exec), HPX_FORWARD(F, f), shape,
                    HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            struct result
            {
                // clang-format off
                using type = decltype(call(std::declval<BulkExecutor>(),
                    std::declval<F>(), std::declval<Shape const&>(),
                    std::declval<Ts>()...));
                // clang-format on
            };
        };

        ///////////////////////////////////////////////////////////////////////
        // bulk_sync_execute()

        // default implementation of the bulk_sync_execute() customization point
        template <typename Executor, typename F, typename Shape, typename... Ts>
        struct bulk_sync_execute_not_callable;

        template <typename Executor, typename F, typename Shape, typename... Ts>
        auto bulk_sync_execute_dispatch(hpx::traits::detail::wrap_int,
            Executor&& /* exec */, F&& /* f */, Shape const& /* shape */,
            Ts&&... /* ts */)
            -> bulk_sync_execute_not_callable<Executor, F, Shape, Ts...>
        {
            return bulk_sync_execute_not_callable<Executor, F, Shape, Ts...>{};
        }

        template <typename BulkTwoWayExecutor, typename F, typename Shape,
            typename... Ts,
            typename Enable = std::enable_if_t<std::is_invocable_v<
                hpx::parallel::execution::bulk_sync_execute_t const&,
                BulkTwoWayExecutor&&, F&&, Shape const&, Ts&&...>>>
        HPX_FORCEINLINE decltype(auto) bulk_sync_execute_dispatch(int,
            BulkTwoWayExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
        {
            return hpx::parallel::execution::bulk_sync_execute(
                exec, HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        template <typename BulkTwoWayExecutor, typename F, typename Shape,
            typename... Ts,
            typename Enable = std::enable_if_t<!std::is_invocable_v<
                hpx::parallel::execution::bulk_sync_execute_t const&,
                BulkTwoWayExecutor&&, F&&, Shape const&, Ts&&...>>>
        HPX_DEPRECATED_V(1, 9,
            "Exposing bulk_sync_execute() from an executor is deprecated, "
            "please expose this functionality through a corresponding "
            "member function")
        HPX_FORCEINLINE auto bulk_sync_execute_dispatch(int,
            BulkTwoWayExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
            -> decltype(exec.bulk_sync_execute(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...))
        {
            return exec.bulk_sync_execute(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename F, typename Shape, bool IsVoid, typename... Ts>
        struct bulk_execute_result_impl;

        template <typename F, typename Shape, typename... Ts>
        struct bulk_execute_result_impl<F, Shape, false, Ts...>
        {
            using type = std::vector<bulk_function_result_t<F, Shape, Ts...>>;
        };

        template <typename F, typename Shape, typename... Ts>
        struct bulk_execute_result_impl<F, Shape, true, Ts...>
        {
            using type = void;
        };

        template <typename F, typename Shape, bool IsVoid, typename... Ts>
        using bulk_execute_result_impl_t =
            typename bulk_execute_result_impl<F, Shape, IsVoid, Ts...>::type;

        HPX_CXX_CORE_EXPORT template <typename F, typename Shape,
            typename... Ts>
        struct bulk_execute_result
          : bulk_execute_result_impl<F, Shape,
                std::is_void_v<bulk_function_result_t<F, Shape, Ts...>>, Ts...>
        {
        };

        HPX_CXX_CORE_EXPORT template <typename F, typename Shape,
            typename... Ts>
        using bulk_execute_result_t =
            typename bulk_execute_result<F, Shape, Ts...>::type;

        ///////////////////////////////////////////////////////////////////////
        template <typename Executor>
        struct bulk_sync_execute_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_one_way_executor_v<Executor> &&
                !hpx::traits::is_two_way_executor_v<Executor> &&
                !hpx::traits::is_bulk_one_way_executor_v<Executor>>>
        {
            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            HPX_FORCEINLINE static auto call_impl(hpx::traits::detail::wrap_int,
                BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
                -> bulk_execute_result_t<F, Shape, Ts...>
            {
                static constexpr bool is_void =
                    std::is_void_v<bulk_function_result_t<F, Shape, Ts...>>;

                try
                {
                    if constexpr (is_void)
                    {
                        for (auto const& elem : shape)
                        {
                            execution::sync_execute(exec, f, elem, ts...);
                        }
                    }
                    else
                    {
                        bulk_execute_result_impl_t<F, Shape, false, Ts...>
                            results;
                        results.reserve(hpx::util::size(shape));

                        for (auto const& elem : shape)
                        {
                            results.push_back(
                                execution::sync_execute(exec, f, elem, ts...));
                        }
                        return results;
                    }
                }
                catch (std::bad_alloc const& ba)
                {
                    throw ba;
                }
                catch (...)
                {
                    // note: constructor doesn't lock/suspend
                    throw hpx::exception_list(std::current_exception());
                }
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts,
                typename Enable = std::enable_if_t<std::is_invocable_v<
                    hpx::parallel::execution::bulk_sync_execute_t const&,
                    BulkExecutor&&, F&&, Shape const&, Ts&&...>>>
            static decltype(auto) call_impl(
                int, BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
            {
                return hpx::parallel::execution::bulk_sync_execute(
                    exec, HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts,
                typename Enable = std::enable_if_t<!std::is_invocable_v<
                    hpx::parallel::execution::bulk_sync_execute_t const&,
                    BulkExecutor&&, F&&, Shape const&, Ts&&...>>>
            HPX_DEPRECATED_V(1, 9,
                "Exposing bulk_sync_execute() from an executor is deprecated, "
                "please expose this functionality through a corresponding "
                "member function")
            static auto call_impl(
                int, BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
                -> decltype(exec.bulk_sync_execute(
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...))
            {
                return exec.bulk_sync_execute(
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            HPX_FORCEINLINE static auto call(
                BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
                -> decltype(call_impl(0, HPX_FORWARD(BulkExecutor, exec),
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...))
            {
                return call_impl(0, HPX_FORWARD(BulkExecutor, exec),
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            struct result
            {
                // clang-format off
                using type = decltype(call(std::declval<BulkExecutor>(),
                    std::declval<F>(), std::declval<Shape const&>(),
                    std::declval<Ts>()...));
                // clang-format on
            };
        };

        template <typename Executor>
        struct bulk_sync_execute_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_two_way_executor_v<Executor> &&
                !hpx::traits::is_bulk_one_way_executor_v<Executor>>>
        {
            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            HPX_FORCEINLINE static auto call_impl(hpx::traits::detail::wrap_int,
                BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
                -> bulk_execute_result_t<F, Shape, Ts...>
            {
                static constexpr bool is_void =
                    std::is_void_v<bulk_function_result_t<F, Shape, Ts...>>;

                using result_type =
                    std::vector<hpx::traits::executor_future_t<Executor,
                        bulk_function_result_t<F, Shape, Ts...>>>;

                result_type results;
                try
                {
                    if constexpr (is_void)
                    {
                        results.reserve(hpx::util::size(shape));
                        for (auto const& elem : shape)
                        {
                            results.push_back(
                                execution::async_execute(exec, f, elem, ts...));
                        }
                        hpx::wait_all_nothrow(results);
                    }
                    else
                    {
                        results.reserve(hpx::util::size(shape));
                        for (auto const& elem : shape)
                        {
                            results.push_back(
                                execution::async_execute(exec, f, elem, ts...));
                        }
                        return hpx::unwrap(HPX_MOVE(results));
                    }
                }
                catch (std::bad_alloc const& ba)
                {
                    throw ba;
                }
                catch (...)
                {
                    // note: constructor doesn't lock/suspend
                    throw hpx::exception_list(std::current_exception());
                }

                if constexpr (is_void)
                {
                    // handle exceptions
                    hpx::exception_list exceptions;
                    for (auto& f : results)
                    {
                        if (f.has_exception())
                        {
                            exceptions.add(f.get_exception_ptr());
                        }
                    }

                    if (exceptions.size() != 0)
                    {
                        throw exceptions;
                    }
                }
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts,
                typename Enable = std::enable_if_t<std::is_invocable_v<
                    hpx::parallel::execution::bulk_sync_execute_t const&,
                    BulkExecutor&&, F&&, Shape const&, Ts&&...>>>
            HPX_FORCEINLINE static decltype(auto) call_impl(
                int, BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
            {
                return hpx::parallel::execution::bulk_sync_execute(
                    exec, HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts,
                typename Enable = std::enable_if_t<!std::is_invocable_v<
                    hpx::parallel::execution::bulk_sync_execute_t const&,
                    BulkExecutor&&, F&&, Shape const&, Ts&&...>>>
            HPX_DEPRECATED_V(1, 9,
                "Exposing bulk_sync_execute() from an executor is deprecated, "
                "please expose this functionality through a corresponding "
                "member function")
            HPX_FORCEINLINE static auto call_impl(
                int, BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
                -> decltype(exec.bulk_sync_execute(
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...))
            {
                return exec.bulk_sync_execute(
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            HPX_FORCEINLINE static auto call(
                BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
                -> decltype(call_impl(0, HPX_FORWARD(BulkExecutor, exec),
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...))
            {
                return call_impl(0, HPX_FORWARD(BulkExecutor, exec),
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            struct result
            {
                // clang-format off
                using type = decltype(call(std::declval<BulkExecutor>(),
                    std::declval<F>(), std::declval<Shape const&>(),
                    std::declval<Ts>()...));
                // clang-format on
            };
        };

        template <typename Executor>
        struct bulk_sync_execute_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_bulk_one_way_executor_v<Executor>>>
        {
            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            HPX_FORCEINLINE static auto call(
                BulkExecutor&& exec, F&& f, Shape const& shape, Ts&&... ts)
                -> decltype(bulk_sync_execute_dispatch(0,
                    HPX_FORWARD(BulkExecutor, exec), HPX_FORWARD(F, f), shape,
                    HPX_FORWARD(Ts, ts)...))
            {
                return bulk_sync_execute_dispatch(0,
                    HPX_FORWARD(BulkExecutor, exec), HPX_FORWARD(F, f), shape,
                    HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename... Ts>
            struct result
            {
                // clang-format off
                using type = decltype(call(std::declval<BulkExecutor>(),
                    std::declval<F>(), std::declval<Shape const&>(),
                    std::declval<Ts>()...));
                // clang-format on
            };
        };
    }    // namespace detail
    /// \endcond

    /// \cond NOINTERNAL
    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        // bulk_then_execute()

        template <typename Executor>
        struct bulk_then_execute_fn_helper<Executor,
            std::enable_if_t<
                !hpx::traits::is_bulk_two_way_executor_v<Executor>>>
        {
            template <typename BulkExecutor, typename F, typename Shape,
                typename Future, typename... Ts>
            HPX_FORCEINLINE static auto call_impl(hpx::traits::detail::wrap_int,
                BulkExecutor&& exec, F&& f, Shape const& shape,
                Future&& predecessor, Ts&&... ts)
                -> hpx::future<
                    bulk_then_execute_result_t<F, Shape, Future, Ts...>>
            {
                static constexpr bool is_void = std::is_void_v<
                    then_bulk_function_result_t<F, Shape, Future, Ts...>>;

                if constexpr (is_void)
                {
                    auto func = make_fused_bulk_sync_execute_helper(exec,
                        HPX_FORWARD(F, f), shape,
                        hpx::make_tuple(HPX_FORWARD(Ts, ts)...));

                    hpx::traits::detail::shared_state_ptr_t<void> p =
                        lcos::detail::make_continuation_exec<void>(
                            HPX_FORWARD(Future, predecessor),
                            HPX_FORWARD(BulkExecutor, exec), HPX_MOVE(func));

                    return hpx::traits::future_access<
                        hpx::future<void>>::create(HPX_MOVE(p));
                }
                else
                {
                    using result_type =
                        bulk_then_execute_result_t<F, Shape, Future, Ts...>;

                    using shared_state_type =
                        hpx::traits::detail::shared_state_ptr_t<result_type>;

                    auto func = make_fused_bulk_sync_execute_helper(exec,
                        HPX_FORWARD(F, f), shape,
                        hpx::make_tuple(HPX_FORWARD(Ts, ts)...));

                    shared_state_type p =
                        lcos::detail::make_continuation_exec<result_type>(
                            HPX_FORWARD(Future, predecessor),
                            HPX_FORWARD(BulkExecutor, exec), HPX_MOVE(func));

                    return hpx::traits::future_access<
                        hpx::future<result_type>>::create(HPX_MOVE(p));
                }
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename Future, typename... Ts,
                typename Enable = std::enable_if_t<std::is_invocable_v<
                    hpx::parallel::execution::bulk_then_execute_t const&,
                    BulkExecutor&&, F&&, Shape const&, Future&&, Ts&&...>>>
            HPX_FORCEINLINE static decltype(auto) call_impl(int,
                BulkExecutor&& exec, F&& f, Shape const& shape,
                Future&& predecessor, Ts&&... ts)
            {
                return hpx::parallel::execution::bulk_then_execute(exec,
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Future, predecessor),
                    HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename Future, typename... Ts,
                typename Enable = std::enable_if_t<!std::is_invocable_v<
                    hpx::parallel::execution::bulk_then_execute_t const&,
                    BulkExecutor&&, F&&, Shape const&, Future&&, Ts&&...>>>
            HPX_DEPRECATED_V(1, 9,
                "Exposing bulk_then_execute() from an executor is deprecated, "
                "please expose this functionality through a corresponding "
                "member function")
            HPX_FORCEINLINE static auto call_impl(int, BulkExecutor&& exec,
                F&& f, Shape const& shape, Future&& predecessor, Ts&&... ts)
                -> decltype(exec.bulk_then_execute(HPX_FORWARD(F, f), shape,
                    HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...))
            {
                return exec.bulk_then_execute(HPX_FORWARD(F, f), shape,
                    HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename Future, typename... Ts>
            HPX_FORCEINLINE static auto call(BulkExecutor&& exec, F&& f,
                Shape const& shape, Future&& predecessor, Ts&&... ts)
                -> decltype(call_impl(0, HPX_FORWARD(BulkExecutor, exec),
                    HPX_FORWARD(F, f), shape,
                    hpx::make_shared_future(HPX_FORWARD(Future, predecessor)),
                    HPX_FORWARD(Ts, ts)...))
            {
                return call_impl(0, HPX_FORWARD(BulkExecutor, exec),
                    HPX_FORWARD(F, f), shape,
                    hpx::make_shared_future(HPX_FORWARD(Future, predecessor)),
                    HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename Future, typename... Ts>
            struct result
            {
                using type = decltype(call(std::declval<BulkExecutor>(),
                    std::declval<F>(), std::declval<Shape const&>(),
                    std::declval<Future>(), std::declval<Ts>()...));
            };
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Executor>
        struct bulk_then_execute_fn_helper<Executor,
            std::enable_if_t<hpx::traits::is_bulk_two_way_executor_v<Executor>>>
        {
            template <typename BulkExecutor, typename F, typename Shape,
                typename Future, typename... Ts>
            static auto call_impl(
                hpx::traits::detail::wrap_int, BulkExecutor&& exec, F&& f,
                Shape const& shape, Future&& predecessor,
                Ts&&...
#if !defined(HPX_COMPUTE_DEVICE_CODE)
                ts
#endif
                ) -> hpx::traits::executor_future_t<Executor,
                bulk_then_execute_result_t<F, Shape, Future, Ts...>>
            {
#if defined(HPX_COMPUTE_DEVICE_CODE)
                HPX_UNUSED(exec);
                HPX_UNUSED(f);
                HPX_UNUSED(shape);
                HPX_UNUSED(predecessor);
                HPX_ASSERT(false);
                return hpx::traits::executor_future_t<Executor,
                    bulk_then_execute_result_t<F, Shape, Future, Ts...>>{};
#else
                auto func = make_fused_bulk_async_execute_helper(exec,
                    HPX_FORWARD(F, f), shape,
                    hpx::make_tuple(HPX_FORWARD(Ts, ts)...));

                // void or std::vector<func_result_type>
                using vector_result_type =
                    bulk_then_execute_result_t<F, Shape, Future, Ts...>;

                // future<vector_result_type>
                using result_future_type =
                    hpx::traits::executor_future_t<Executor,
                        vector_result_type>;

                using shared_state_type =
                    hpx::traits::detail::shared_state_ptr_t<vector_result_type>;

                using future_type = std::decay_t<Future>;

                shared_state_type p =
                    lcos::detail::make_continuation_exec<vector_result_type>(
                        HPX_FORWARD(Future, predecessor),
                        HPX_FORWARD(BulkExecutor, exec),
                        [func = HPX_MOVE(func)](
                            future_type&& predecessor) mutable
                            -> vector_result_type {
                            // use unwrap directly (instead of lazily) to avoid
                            // having to pull in dataflow
                            return hpx::unwrap(func(HPX_MOVE(predecessor)));
                        });

                return hpx::traits::future_access<result_future_type>::create(
                    HPX_MOVE(p));
#endif
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename Future, typename... Ts,
                typename Enable = std::enable_if_t<std::is_invocable_v<
                    hpx::parallel::execution::bulk_then_execute_t const&,
                    BulkExecutor&&, F&&, Shape const&, Future&&, Ts&&...>>>
            HPX_FORCEINLINE static decltype(auto) call_impl(int,
                BulkExecutor&& exec, F&& f, Shape const& shape,
                Future&& predecessor, Ts&&... ts)
            {
                return hpx::parallel::execution::bulk_then_execute(exec,
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Future, predecessor),
                    HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename Future, typename... Ts,
                typename Enable = std::enable_if_t<!std::is_invocable_v<
                    hpx::parallel::execution::bulk_then_execute_t const&,
                    BulkExecutor&&, F&&, Shape const&, Future&&, Ts&&...>>>
            HPX_DEPRECATED_V(1, 9,
                "Exposing bulk_then_execute() from an executor is deprecated, "
                "please expose this functionality through a corresponding "
                "member function")
            HPX_FORCEINLINE static auto call_impl(int, BulkExecutor&& exec,
                F&& f, Shape const& shape, Future&& predecessor, Ts&&... ts)
                -> decltype(exec.bulk_then_execute(HPX_FORWARD(F, f), shape,
                    HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...))
            {
                return exec.bulk_then_execute(HPX_FORWARD(F, f), shape,
                    HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename Future, typename... Ts>
            HPX_FORCEINLINE static auto call(BulkExecutor&& exec, F&& f,
                Shape const& shape, Future&& predecessor, Ts&&... ts)
                -> decltype(call_impl(0, HPX_FORWARD(BulkExecutor, exec),
                    HPX_FORWARD(F, f), shape,
                    hpx::make_shared_future(HPX_FORWARD(Future, predecessor)),
                    HPX_FORWARD(Ts, ts)...))
            {
                return call_impl(0, HPX_FORWARD(BulkExecutor, exec),
                    HPX_FORWARD(F, f), shape,
                    hpx::make_shared_future(HPX_FORWARD(Future, predecessor)),
                    HPX_FORWARD(Ts, ts)...);
            }

            template <typename BulkExecutor, typename F, typename Shape,
                typename Future, typename... Ts>
            struct result
            {
                using type = decltype(call(std::declval<BulkExecutor>(),
                    std::declval<F>(), std::declval<Shape const&>(),
                    std::declval<Future>(), std::declval<Ts>()...));
            };
        };
    }    // namespace detail
    /// \endcond

    /// \cond NOINTERNAL
    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        // async_invoke()

        template <typename Executor, typename Enable>
        struct async_invoke_fn_helper
        {
            template <typename Executor_, typename... Fs>
            HPX_FORCEINLINE static hpx::future<void> call(
                Executor_&& exec, Fs&&... fs)
            {
                std::vector<hpx::traits::executor_future_t<Executor, void>>
                    results;
                results.reserve(sizeof...(Fs));

                (results.push_back(hpx::parallel::execution::async_execute(
                     exec, HPX_FORWARD(Fs, fs))),
                    ...);

                return hpx::when_all(HPX_MOVE(results));
            }

            template <typename Executor_, typename... Fs>
            struct result
            {
                // clang-format off
                using type = decltype(call(
                    std::declval<Executor_>(), std::declval<Fs>()...));
                // clang-format on
            };
        };
    }    // namespace detail
    /// \endcond

    /// \cond NOINTERNAL
    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        // sync_invoke()

        template <typename Executor, typename Enable>
        struct sync_invoke_fn_helper
        {
            template <typename Executor_, typename... Fs>
            HPX_FORCEINLINE static auto call(Executor_&& exec, Fs&&... fs)
            {
                std::vector<hpx::traits::executor_future_t<Executor, void>>
                    results;
                results.reserve(sizeof...(Fs));

                (results.push_back(hpx::parallel::execution::async_execute(
                     exec, HPX_FORWARD(Fs, fs))),
                    ...);

                hpx::wait_all(HPX_MOVE(results));
            }

            template <typename Executor_, typename... Fs>
            struct result
            {
                // clang-format off
                using type = decltype(call(
                    std::declval<Executor_>(), std::declval<Fs>()...));
                // clang-format on
            };
        };
    }    // namespace detail
    /// \endcond

}    // namespace hpx::parallel::execution
