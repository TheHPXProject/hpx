//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/executors/parallel_executor.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>

#include <type_traits>
#include <utility>

namespace hpx::detail {

    HPX_CXX_CORE_EXPORT template <typename Func, typename Enable = void>
    struct sync_dispatch_launch_policy_helper;

    template <typename Func>
    struct sync_dispatch_launch_policy_helper<Func,
        std::enable_if_t<!traits::is_action_v<Func>>>
    {
        template <typename Policy_, typename F, typename... Ts>
        HPX_FORCEINLINE static auto call(
            Policy_&& launch_policy, F&& f, Ts&&... ts)
            -> decltype(sync_launch_policy_dispatch<std::decay_t<F>>::call(
                HPX_FORWARD(Policy_, launch_policy), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...))
        {
            return sync_launch_policy_dispatch<std::decay_t<F>>::call(
                HPX_FORWARD(Policy_, launch_policy), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    };

    template <typename Policy>
    struct sync_dispatch<Policy,
        std::enable_if_t<traits::is_launch_policy_v<Policy>>>
    {
        // clang-format off
        template <typename Policy_, typename F, typename... Ts>
        HPX_FORCEINLINE static auto call(
            Policy_&& launch_policy, F&& f, Ts&&... ts)
            -> decltype(sync_dispatch_launch_policy_helper<
                std::decay_t<F>>::call(HPX_FORWARD(Policy_, launch_policy),
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
        // clang-format on
        {
            return sync_dispatch_launch_policy_helper<std::decay_t<F>>::call(
                HPX_FORWARD(Policy_, launch_policy), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    };

    // Launch the given function or function object synchronously. This exists
    // mostly for symmetry with hpx::async.
    HPX_CXX_CORE_EXPORT template <typename Func, typename Enable>
    struct sync_dispatch
    {
        // clang-format off
        template <typename F, typename... Ts>
        HPX_FORCEINLINE static auto call(F&& f, Ts&&... ts)
            -> decltype(parallel::execution::sync_execute(
                execution::parallel_executor(), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...))
        // clang-format on
        {
            execution::parallel_executor exec;
            return parallel::execution::sync_execute(
                exec, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }
    };

    // The overload for hpx::sync taking an executor simply forwards to the
    // corresponding executor customization point.
    template <typename Executor>
    struct sync_dispatch<Executor,
        std::enable_if_t<traits::is_one_way_executor_v<Executor> ||
            traits::is_two_way_executor_v<Executor>>>
    {
        // clang-format off
        template <typename Executor_, typename F, typename... Ts>
        HPX_FORCEINLINE static auto call(Executor_&& exec, F&& f, Ts&&... ts)
            -> decltype(parallel::execution::sync_execute(
                HPX_FORWARD(Executor_, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...))
        // clang-format on
        {
            return parallel::execution::sync_execute(
                HPX_FORWARD(Executor_, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    };
}    // namespace hpx::detail
