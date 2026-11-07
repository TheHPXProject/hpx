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
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/for_loop_reduction.hpp>

#include <cstddef>
#include <exception>
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

        template <typename Tuple, std::size_t... Is>
        constexpr void init_iterations(Tuple& t, hpx::util::index_pack<Is...>)
        {
            if constexpr (sizeof...(Is) > 0)
            {
                (std::get<Is>(t).init_iteration(0, 0), ...);
            }
        }

        template <typename Tuple, std::size_t... Is>
        constexpr void exit_iterations(Tuple& t, hpx::util::index_pack<Is...>)
        {
            if constexpr (sizeof...(Is) > 0)
            {
                (std::get<Is>(t).exit_iteration(0), ...);
            }
        }

        template <typename F, typename Tuple, std::size_t... Is>
        constexpr void invoke_iteration_values(
            F& f, Tuple& t, std::size_t i, hpx::util::index_pack<Is...>)
        {
            if constexpr (sizeof...(Is) == 0)
            {
                if constexpr (std::is_invocable_v<F&, std::size_t>)
                {
                    f(i);
                }
                else
                {
                    f();
                }
            }
            else
            {
                f(std::get<Is>(t).iteration_value(i)...);
            }
        }

        template <typename Sp>
        struct run_on_all_cleanup
        {
            Sp sp_;

            void operator()() const
            {
                exit_iterations(*sp_,
                    hpx::util::make_index_pack_t<
                        std::tuple_size_v<std::decay_t<decltype(*sp_)>>>{});
            }
        };

        template <typename Cleanup>
        struct run_on_all_error
        {
            Cleanup cleanup_;

            auto operator()(std::exception_ptr ep) const
            {
                try
                {
                    cleanup_();
                }
                // NOLINTNEXTLINE(bugprone-empty-catch)
                catch (...)
                {
                    // silently swallow secondary exceptions during cleanup
                }
                return hpx::execution::experimental::just_error(HPX_MOVE(ep));
            }
        };

        template <typename Sp, typename F>
        struct run_on_all_bulk_task
        {
            Sp sp_;
            std::decay_t<F> f_;

            template <typename... Ts>
            void operator()(std::size_t i, Ts&&...)
            {
                invoke_iteration_values(f_, *sp_, i,
                    hpx::util::make_index_pack_t<
                        std::tuple_size_v<std::decay_t<decltype(*sp_)>>>{});
            }
        };

        template <typename Scheduler, typename F, typename ReductionsTuple>
        struct run_on_all_start
        {
            std::decay_t<Scheduler> sched_;
            std::decay_t<F> f_;
            ReductionsTuple reductions_;

            auto operator()()
            {
                namespace ex = hpx::execution::experimental;

                auto cores = ex::processing_units_count(sched_);
                auto const limit =
                    hpx::parallel::execution::detail::get_os_thread_count();
                if (cores > limit)
                {
                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "run_on_all",
                        "The number of cores exceeds the allocated reduction "
                        "storage size.");
                }

                auto sp =
                    std::make_shared<ReductionsTuple>(HPX_MOVE(reductions_));

                init_iterations(*sp,
                    hpx::util::make_index_pack_t<
                        std::tuple_size_v<ReductionsTuple>>{});

                auto sched =
                    ex::with_hint(ex::with_priority(sched_,
                                      hpx::threads::thread_priority::bound),
                        hpx::threads::thread_schedule_hint(hpx::threads::
                                thread_sharing_hint::do_not_share_function));

                run_on_all_bulk_task<decltype(sp), F> bulk_task{
                    sp, HPX_MOVE(f_)};
                run_on_all_cleanup<decltype(sp)> cleanup_action{sp};
                run_on_all_error<decltype(cleanup_action)> error_action{
                    cleanup_action};

                return ex::schedule(HPX_MOVE(sched)) |
                    ex::bulk(cores, HPX_MOVE(bulk_task)) |
                    ex::let_error(HPX_MOVE(error_action)) |
                    ex::then(HPX_MOVE(cleanup_action));
            }
        };

        HPX_CXX_CORE_EXPORT template <typename Scheduler, typename F,
            typename... Reductions>
        // clang-format off
            requires (
                hpx::execution::experimental::is_scheduler_v<std::decay_t<Scheduler>>
            )
        // clang-format on
        decltype(auto) run_on_all_sched(
            Scheduler&& sched, F&& f, Reductions&&... reductions)
        {
            namespace ex = hpx::execution::experimental;

            auto all_reductions =
                std::make_tuple(HPX_FORWARD(Reductions, reductions)...);
            using reductions_type = decltype(all_reductions);

            return ex::let_value(ex::just(),
                run_on_all_start<Scheduler, F, reductions_type>{
                    HPX_FORWARD(Scheduler, sched), HPX_FORWARD(F, f),
                    HPX_MOVE(all_reductions)});
        }

        HPX_CXX_CORE_EXPORT template <typename Scheduler, std::size_t... Is,
            typename... Ts>
        decltype(auto) run_on_all_sched(
            Scheduler&& sched, hpx::util::index_pack<Is...>, Ts&&... ts)
        {
            auto&& t = std::forward_as_tuple(HPX_FORWARD(Ts, ts)...);
            auto f = std::get<sizeof...(Ts) - 1>(t);

            return run_on_all_sched(
                HPX_FORWARD(Scheduler, sched), HPX_MOVE(f), std::get<Is>(t)...);
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

    /// Run a function on all available worker threads with reduction
    /// support using the given P2300 scheduler
    ///
    /// \tparam Scheduler The scheduler type
    /// \tparam T         The first type in a list of reduction types
    ///                   and the function type to invoke (last
    ///                   argument)
    /// \tparam Ts        The list of reduction types and the function
    ///                   type to invoke (last argument)
    /// \param sched      The scheduler to use
    /// \param t          The first in a list of reductions and the
    ///                   function to invoke (last argument)
    /// \param ts         The list of reductions and the function to
    ///                   invoke (last argument)
    ///
    /// \returns A P2300 sender. Unlike the legacy \c par overload,
    ///          execution is deferred: nothing runs until the
    ///          returned sender is connected and started (e.g.
    ///          via \c sync_wait).
    HPX_CXX_CORE_EXPORT template <typename Scheduler, typename T,
        typename... Ts>
        requires(hpx::execution::experimental::is_scheduler_v<
            std::decay_t<Scheduler>>)
    decltype(auto) run_on_all(Scheduler&& sched, T&& t, Ts&&... ts)
    {
        return detail::run_on_all_sched(HPX_FORWARD(Scheduler, sched),
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
        requires(!hpx::is_execution_policy_v<T> &&
            !hpx::execution::experimental::is_scheduler_v<std::decay_t<T>>)
    decltype(auto) run_on_all(T&& t, Ts&&... ts)
    {
        return detail::run_on_all(hpx::execution::par,
            hpx::util::make_index_pack_t<sizeof...(Ts)>(), HPX_FORWARD(T, t),
            HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::experimental
