//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <functional>
#include <type_traits>
#include <utility>

namespace hpx::detail {

    // dispatch point used for launch_policy implementations
    HPX_CXX_CORE_EXPORT template <typename Action, typename Enable = void>
    struct sync_launch_policy_dispatch;

    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct sync_launch_policy_dispatch<launch::sync_policy>
    {
        // launch::sync execute inline
        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(Policy&&, F&& f, Ts&&... ts)
        {
            try
            {
                return HPX_INVOKE(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }
            catch (std::bad_alloc const& ba)
            {
                throw ba;
            }
            catch (...)
            {
                throw exception_list(std::current_exception());
            }
        }
    };

    template <>
    struct sync_launch_policy_dispatch<launch::deferred_policy>
    {
        // launch::deferred execute inline
        template <typename Policy, typename F, typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(Policy&&, F&& f, Ts&&... ts)
        {
            try
            {
                return HPX_INVOKE(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }
            catch (std::bad_alloc const& ba)
            {
                throw ba;
            }
            catch (...)
            {
                throw exception_list(std::current_exception());
            }
        }
    };

    template <typename Action>
    struct sync_launch_policy_dispatch<Action,
        std::enable_if_t<!traits::is_action_v<Action>>>
    {
        // general case execute on separate thread (except launch::sync)
        template <typename F, typename... Ts>
        HPX_FORCEINLINE static auto call(launch policy, F&& f, Ts&&... ts)
        {
            using result_type =
                util::detail::invoke_deferred_result_t<F, Ts...>;

            if (policy == launch::sync)
            {
                return sync_launch_policy_dispatch<launch::sync_policy>::call(
                    HPX_MOVE(policy), HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...);
            }
            else if (policy == launch::deferred)
            {
                return sync_launch_policy_dispatch<
                    launch::deferred_policy>::call(HPX_MOVE(policy),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            lcos::local::futures_factory<result_type()> p(
                util::deferred_call(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));

            if (hpx::has_async_policy(policy))
            {
                // if one of the arguments is a future we play it conservatively
                if constexpr (hpx::traits::is_future_any_v<std::decay_t<Ts>...>)
                {
                    auto hint = policy.hint();
                    if (hint.runs_as_child_mode() ==
                        hpx::threads::thread_execution_hint::run_as_child)
                    {
                        hint.runs_as_child_mode(
                            hpx::threads::thread_execution_hint::none);
                        policy.set_hint(hint);
                    }
                }

                threads::thread_id_ref_type tid =
                    p.post("sync_launch_policy_dispatch<fork>", policy,
                        policy.priority());
                if (tid && policy == launch::fork)
                {
                    // make sure this thread is executed last: yield_to
                    hpx::this_thread::suspend(
                        threads::thread_schedule_state::pending, tid.noref(),
                        "sync_launch_policy_dispatch<fork>");
                }
            }

            return p.get_future().get();
        }
    };
}    // namespace hpx::detail
