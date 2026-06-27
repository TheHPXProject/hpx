//  Copyright (c) 2025 Harith Reddy
//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file run_on_all.hpp
/// \page hpx::experimental::run_on_all
/// \headerfile hpx/task_block.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/for_loop_reduction.hpp>

#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace hpx::experimental {

    /// \cond NOINTERNAL
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F,
            typename... Reductions>
        decltype(auto) run_on_all(
            ExPolicy&& policy, F&& f, Reductions&&... reductions)
        {
            // force using index_queue scheduler with given amount of threads
            hpx::threads::thread_schedule_hint hint;
            hint.sharing_mode(
                hpx::threads::thread_sharing_hint::do_not_share_function);

            auto cores =
                hpx::execution::experimental::processing_units_count(policy);

            // Create executor with proper configuration
            auto exec =
                hpx::execution::experimental::with_processing_units_count(
                    hpx::execution::parallel_executor(
                        hpx::threads::thread_priority::bound,
                        hpx::threads::thread_stacksize::default_, hint),
                    cores);

            // Execute based on policy type
            if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
            {
                // Initialize all reductions
                auto all_reductions =
                    std::make_tuple(HPX_FORWARD(Reductions, reductions)...);
                auto sp = std::make_shared<decltype(all_reductions)>(
                    HPX_MOVE(all_reductions));

                std::apply(
                    [&](auto&... r) { (r.init_iteration(0, 0), ...); }, *sp);

                // Create a lambda that captures all reductions
                auto task = [sp, f = HPX_FORWARD(F, f)](std::size_t i) {
                    std::apply(
                        [&](auto&... r) { f(r.iteration_value(i)...); }, *sp);
                };

                auto fut = hpx::parallel::execution::bulk_async_execute(
                    HPX_MOVE(exec), HPX_MOVE(task), cores);

                // Return a future that performs cleanup after all tasks
                // complete
                return fut.then([sp = HPX_MOVE(sp)](auto&& fut_inner) mutable {
                    std::apply(
                        [](auto&... r) { (r.exit_iteration(0), ...); }, *sp);
                    return fut_inner.get();
                });
            }
            else
            {
                // Initialize all reductions
                auto&& all_reductions = std::forward_as_tuple(
                    HPX_FORWARD(Reductions, reductions)...);

                std::apply([](auto&... r) { (r.init_iteration(0, 0), ...); },
                    all_reductions);

                // Create a lambda that captures all reductions
                auto task = [&all_reductions, &f](std::size_t i) {
                    std::apply([&](auto&... r) { f(r.iteration_value(i)...); },
                        all_reductions);
                };

                hpx::parallel::execution::bulk_sync_execute(
                    HPX_MOVE(exec), HPX_MOVE(task), cores);

                // Clean up reductions
                std::apply([](auto&... r) { (r.exit_iteration(0), ...); },
                    all_reductions);
            }
        }

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, std::size_t... Is,
            typename... Ts>
        decltype(auto) run_on_all(
            ExPolicy&& policy, hpx::util::index_pack<Is...>, Ts&&... ts)
        {
            auto&& t = std::forward_as_tuple(HPX_FORWARD(Ts, ts)...);
            auto f = std::get<sizeof...(Ts) - 1>(t);

            return run_on_all(
                HPX_FORWARD(ExPolicy, policy), HPX_MOVE(f), std::get<Is>(t)...);
        }
    }    // namespace detail
    /// \endcond

    /// Run a function on all available worker threads with reduction support
    /// using the given execution policy
    ///
    /// \tparam ExPolicy The execution policy type
    /// \tparam T        The first type in a list of reduction types and the
    ///                  function type to invoke (last argument)
    /// \tparam Ts       The list of reduction types and the function type to
    ///                  invoke (last argument)
    /// \param policy    The execution policy to use
    /// \param t         The first in a list of reductions and the function to
    ///                  invoke (last argument)
    /// \param ts        The list of reductions and the function to invoke (last
    ///                  argument)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T, typename... Ts>
        requires(hpx::is_execution_policy_v<ExPolicy>)
    decltype(auto) run_on_all(ExPolicy&& policy, T&& t, Ts&&... ts)
    {
        return detail::run_on_all(HPX_FORWARD(ExPolicy, policy),
            hpx::util::make_index_pack_t<sizeof...(Ts)>(), HPX_FORWARD(T, t),
            HPX_FORWARD(Ts, ts)...);
    }

    /// Run a function on all available worker threads with reduction support
    /// using the \a hpx::execution::par execution policy
    ///
    /// \tparam T        The first type in a list of reduction types and the
    ///                  function type to invoke (last argument)
    /// \tparam Ts       The list of reduction types and the function type to
    ///                  invoke (last argument)
    /// \param t         The first in a list of reductions and the function to
    ///                  invoke (last argument)
    /// \param ts        The list of reductions and the function to invoke (last
    ///                  argument)
    HPX_CXX_CORE_EXPORT template <typename T, typename... Ts>
        requires(!hpx::is_execution_policy_v<T>)
    decltype(auto) run_on_all(T&& t, Ts&&... ts)
    {
        return detail::run_on_all(hpx::execution::par,
            hpx::util::make_index_pack_t<sizeof...(Ts)>(), HPX_FORWARD(T, t),
            HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::experimental
