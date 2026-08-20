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
            auto cores =
                hpx::execution::experimental::processing_units_count(policy);

            // Create scheduler with proper core count configuration
            auto sched =
                hpx::execution::experimental::with_processing_units_count(
                    hpx::execution::experimental::thread_pool_scheduler{},
                    cores);

            namespace ex = hpx::execution::experimental;

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

                // 1. Extract lambdas into variables BEFORE the pipeline to prevent
                // Clang 22 template instantiation crashes (exit code 139).
                auto bulk_task = [sp, f = HPX_FORWARD(F, f)](std::size_t i) {
                    std::apply(
                        [&](auto&... r) { f(r.iteration_value(i)...); }, *sp);
                };

                auto cleanup_task = [sp = HPX_MOVE(sp)]() mutable {
                    std::apply(
                        [](auto&... r) { (r.exit_iteration(0), ...); }, *sp);
                };

                // 2. Build the graph
                auto s = ex::schedule(sched) |
                    ex::bulk(cores, HPX_MOVE(bulk_task)) |
                    ex::then(HPX_MOVE(cleanup_task));

                // 3. Adapt the return type to a future
                return ex::make_future(HPX_MOVE(s));
            }
            else
            {
                // Initialize all reductions
                auto&& all_reductions = std::forward_as_tuple(
                    HPX_FORWARD(Reductions, reductions)...);

                std::apply([](auto&... r) { (r.init_iteration(0, 0), ...); },
                    all_reductions);

                // 1. Extract lambdas into variables BEFORE the pipeline to prevent
                // Clang 22 template instantiation crashes (exit code 139).
                auto bulk_task = [&all_reductions, &f](std::size_t i) {
                    std::apply([&](auto&... r) { f(r.iteration_value(i)...); },
                        all_reductions);
                };

                auto cleanup_task = [&all_reductions]() {
                    std::apply([](auto&... r) { (r.exit_iteration(0), ...); },
                        all_reductions);
                };

                // 2. Build the graph
                auto s = ex::schedule(sched) |
                    ex::bulk(cores, HPX_MOVE(bulk_task)) |
                    ex::then(HPX_MOVE(cleanup_task));

                // 3. Adapt for void return (Synchronous blocking)
                hpx::this_thread::experimental::sync_wait(HPX_MOVE(s));
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
