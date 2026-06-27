//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/threading_base.hpp>

#include <exception>
#include <type_traits>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    // dispatch point used for launch_policy implementations
    HPX_CXX_CORE_EXPORT template <typename Action, typename Enable = void>
    struct async_launch_policy_dispatch;

    ///////////////////////////////////////////////////////////////////////////
    template <typename F, typename... Ts>
    HPX_FORCEINLINE hpx::future<
        util::detail::invoke_deferred_result_t<std::decay_t<F>, Ts...>>
    call_sync(F&& f, Ts... vs) noexcept
    {
        using R =
            util::detail::invoke_deferred_result_t<std::decay_t<F>, Ts...>;

        try
        {
            if constexpr (std::is_void_v<R>)
            {
                HPX_INVOKE(HPX_FORWARD(F, f), HPX_MOVE(vs)...);
                return make_ready_future();
            }
            else
            {
                return make_ready_future<R>(
                    HPX_INVOKE(HPX_FORWARD(F, f), HPX_MOVE(vs)...));
            }
        }
        catch (...)
        {
            return make_exceptional_future<R>(std::current_exception());
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct async_launch_policy_dispatch<hpx::launch::sync_policy>
    {
        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy&&, F&& f, Ts&&... ts)
        {
            return call_sync(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy&&, hpx::threads::thread_description const& desc, F&& f,
            Ts&&... ts)
        {
            auto ann = hpx::scoped_annotation(desc.get_description());
            return call_sync(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy&&, hpx::threads::thread_description const& desc,
            threads::thread_pool_base*, F&& f, Ts&&... ts)
        {
            auto ann = hpx::scoped_annotation(desc.get_description());
            return call_sync(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }
    };

    template <>
    struct async_launch_policy_dispatch<hpx::launch::deferred_policy>
    {
        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy&&, F&& f, Ts&&... ts)
        {
            using result_type =
                util::detail::invoke_deferred_result_t<F, Ts...>;

            lcos::local::futures_factory<result_type()> p(
                util::deferred_call(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
            return p.get_future();
        }

        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy&& policy, hpx::threads::thread_description const& desc,
            F&& f, Ts&&... ts)
        {
            return call(HPX_FORWARD(Policy, policy),
                hpx::annotated_function(
                    HPX_FORWARD(F, f), desc.get_description()),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy&& policy, hpx::threads::thread_description const& desc,
            threads::thread_pool_base*, F&& f, Ts&&... ts)
        {
            return call(HPX_FORWARD(Policy, policy),
                hpx::annotated_function(
                    HPX_FORWARD(F, f), desc.get_description()),
                HPX_FORWARD(Ts, ts)...);
        }
    };

    template <>
    struct async_launch_policy_dispatch<hpx::launch::async_policy>
    {
        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy policy, hpx::threads::thread_description const& desc,
            threads::thread_pool_base* pool, F&& f, Ts&&... ts)
        {
            HPX_ASSERT(pool);

            using result_type =
                util::detail::invoke_deferred_result_t<F, Ts...>;

            // if one of the arguments is a future we play it conservatively
            auto hint = policy.hint();
            if constexpr (hpx::traits::is_future_any_v<std::decay_t<Ts>...>)
            {
                if (hint.runs_as_child_mode() ==
                    hpx::threads::thread_execution_hint::run_as_child)
                {
                    hint.runs_as_child_mode(
                        hpx::threads::thread_execution_hint::none);
                    policy.set_hint(hint);
                }
            }

            lcos::local::futures_factory<result_type()> p(
                util::deferred_call(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));

            if (threads::thread_id_ref_type tid =
                    p.post(pool, desc.get_description(), HPX_MOVE(policy)))
            {
                auto runs_as_child = hint.runs_as_child_mode();
                if (runs_as_child ==
                        hpx::threads::thread_execution_hint::run_as_child &&
                    !pool->get_scheduler()->supports_direct_execution())
                {
                    runs_as_child = hpx::threads::thread_execution_hint::none;
                }

                if (runs_as_child == hpx::threads::thread_execution_hint::none)
                {
                    // keep thread alive, if needed
                    auto result = p.get_future();
                    traits::detail::get_shared_state(result)->set_on_completed(
                        [tid = HPX_MOVE(tid)]() { (void) tid; });
                    return result;
                }
            }

            return p.get_future();
        }

        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy&& policy, hpx::threads::thread_description const& desc,
            F&& f, Ts&&... ts)
        {
            return call(HPX_FORWARD(Policy, policy), desc,
                threads::detail::get_self_or_default_pool(), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy&& policy, F&& f, Ts&&... ts)
        {
            hpx::threads::thread_description desc(f);
            return call(HPX_FORWARD(Policy, policy), desc,
                threads::detail::get_self_or_default_pool(), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    };

    template <>
    struct async_launch_policy_dispatch<hpx::launch::fork_policy>
    {
        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy policy, hpx::threads::thread_description const& desc,
            threads::thread_pool_base* pool, F&& f, Ts&&... ts)
        {
            HPX_ASSERT(pool != nullptr);

            using result_type =
                util::detail::invoke_deferred_result_t<F, Ts...>;

            // if one of the arguments is a future we play it conservatively
            auto hint = policy.hint();
            if constexpr (hpx::traits::is_future_any_v<std::decay_t<Ts>...>)
            {
                if (hint.runs_as_child_mode() ==
                    hpx::threads::thread_execution_hint::run_as_child)
                {
                    hint.runs_as_child_mode(
                        hpx::threads::thread_execution_hint::none);
                    policy.set_hint(hint);
                }
            }

            lcos::local::futures_factory<result_type()> p(
                util::deferred_call(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));

            threads::thread_id_ref_type tid =
                p.post(pool, desc.get_description(), policy);

            // make sure this thread is executed last
            threads::thread_id_type const tid_self = threads::get_self_id();
            if (tid && tid_self &&
                get_thread_id_data(tid)->get_scheduler_base() ==
                    get_thread_id_data(tid_self)->get_scheduler_base())
            {
                // yield_to
                hpx::this_thread::suspend(
                    threads::thread_schedule_state::pending, tid.noref(),
                    desc.get_description());

                auto runs_as_child = hint.runs_as_child_mode();
                if (runs_as_child ==
                        hpx::threads::thread_execution_hint::run_as_child &&
                    !pool->get_scheduler()->supports_direct_execution())
                {
                    runs_as_child = hpx::threads::thread_execution_hint::none;
                }

                if (runs_as_child == hpx::threads::thread_execution_hint::none)
                {
                    // keep thread alive, if needed
                    auto result = p.get_future();
                    traits::detail::get_shared_state(result)->set_on_completed(
                        [tid = HPX_MOVE(tid)]() { (void) tid; });
                    return result;
                }
            }

            return p.get_future();
        }

        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy&& policy, hpx::threads::thread_description const& desc,
            F&& f, Ts&&... ts)
        {
            return call(HPX_FORWARD(Policy, policy), desc,
                threads::detail::get_self_or_default_pool(), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(Policy&& policy, F&& f, Ts&&... ts)
        {
            hpx::threads::thread_description desc(f);
            return call(HPX_FORWARD(Policy, policy), desc,
                threads::detail::get_self_or_default_pool(), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    };

    template <typename Action>
    struct async_launch_policy_dispatch<Action,
        std::enable_if_t<!traits::is_action_v<Action>>>
    {
        template <typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(launch policy, hpx::threads::thread_description const& desc,
            threads::thread_pool_base* pool, F&& f, Ts&&... ts)
        {
            if (policy == launch::async)
            {
                return async_launch_policy_dispatch<
                    hpx::launch::async_policy>::call(HPX_MOVE(policy), desc,
                    pool, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }
            if (policy == launch::sync)
            {
                return async_launch_policy_dispatch<
                    hpx::launch::sync_policy>::call(HPX_MOVE(policy), desc,
                    pool, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }
            if (policy == launch::deferred)
            {
                return async_launch_policy_dispatch<
                    hpx::launch::deferred_policy>::call(HPX_MOVE(policy), desc,
                    pool, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }
            if (policy == launch::fork)
            {
                return async_launch_policy_dispatch<
                    hpx::launch::fork_policy>::call(HPX_MOVE(policy), desc,
                    pool, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            return async_launch_policy_dispatch<
                hpx::launch::async_policy>::call(HPX_MOVE(policy), desc, pool,
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(launch policy, hpx::threads::thread_description const& desc, F&& f,
            Ts&&... ts)
        {
            return call(HPX_MOVE(policy), desc,
                threads::detail::get_self_or_default_pool(), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            hpx::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(launch policy, F&& f, Ts&&... ts)
        {
            hpx::threads::thread_description desc(f);
            return call(HPX_MOVE(policy), desc,
                threads::detail::get_self_or_default_pool(), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    };
}    // namespace hpx::detail

#include <hpx/config/warnings_suffix.hpp>
