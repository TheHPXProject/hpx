//  Copyright (c) 2021-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file task_group.hpp
/// \page hpx::experimental::task_group
/// \headerfile hpx/experimental/task_group.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>

#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/synchronization.hpp>

#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

/// Top-level namespace
namespace hpx::experimental {

    /// A \c task_group represents concurrent execution of a group of tasks.
    /// Tasks can be dynamically added to the group while it is executing.
    HPX_CXX_CORE_EXPORT class task_group
    {
    public:
        HPX_CORE_EXPORT task_group();
        HPX_CORE_EXPORT ~task_group();

        task_group(task_group const&) = delete;
        task_group(task_group&&) = delete;

        task_group& operator=(task_group const&) = delete;
        task_group& operator=(task_group&&) = delete;

    public:
        /// \brief Adds a task to compute \c f() and returns immediately.
        ///
        /// \tparam Executor  The type of the executor to associate with this
        ///                   execution policy.
        /// \tparam F         The type of the user defined function to invoke.
        /// \tparam Ts        The type of additional arguments used to invoke \c f().
        ///
        /// \param exec       The executor to use for the execution of the
        ///                   parallel algorithm the returned execution
        ///                   policy is used with.
        /// \param f          The user defined function to invoke inside the task
        ///                   group.
        /// \param ts         Additional arguments to use to invoke \c f().

        template <typename Executor, typename F, typename... Ts>
        // clang-format off
            requires (
                hpx::traits::is_executor_any_v<std::decay_t<Executor>>
            )
        // clang-format on
        void run(Executor&& exec, F&& f, Ts&&... ts)
        {
            hpx::parallel::execution::post(HPX_FORWARD(Executor, exec),
                wrap_task(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
        }

        /// \brief Adds a task to compute \c f() and returns immediately.
        ///
        /// \tparam Scheduler The type of the P2300 scheduler to dispatch on.
        /// \tparam F         The type of the user defined function to invoke.
        /// \tparam Ts        The type of additional arguments used to invoke \c f().
        ///
        /// \param sched      The P2300 scheduler to use for dispatching the
        ///                   task.
        /// \param f          The user defined function to invoke inside the task
        ///                   group.
        /// \param ts         Additional arguments to use to invoke \c f().

        template <typename Scheduler, typename F, typename... Ts>
        // clang-format off
            requires (
                hpx::execution::experimental::is_scheduler_v<
                    std::decay_t<Scheduler>> &&
                !hpx::traits::is_executor_any_v<std::decay_t<Scheduler>>
            )
        // clang-format on
        void run(Scheduler&& sched, F&& f, Ts&&... ts)
        {
            namespace ex = hpx::execution::experimental;

            // Extract the lambda into a separate variable to prevent AST depth
            // crashes on Clang compilers during complex template instantiations.
            auto task = wrap_task(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);

            auto stopped_handler = []() {
                // Note: This silently discards cancellation signals. If the scheduler
                // signals 'stopped' (e.g., the pool is shutting down), this pipeline
                // converts it to just() with no call to add_exception. This is an
                // explicit semantic choice: a cancelled task will silently vanish
                // and not appear in the exception_list.
                return ex::just();
            };

            auto sender = ex::schedule(HPX_FORWARD(Scheduler, sched)) |
                ex::then(HPX_MOVE(task)) |
                ex::let_error([this](std::exception_ptr e) mutable {
                    // Note: The wrap_task inner lambda already wraps the user callable
                    // in try_catch_exception_ptr and reports user-task failures via
                    // add_exception. Therefore, this let_error acts solely as a safety
                    // net for scheduler-level errors (e.g., if schedule() itself fails).
                    // We convert this error channel to a value channel via ex::just()
                    // to satisfy start_detached.
                    add_exception(HPX_MOVE(e));
                    return ex::just();
                }) |
                ex::let_stopped(HPX_MOVE(stopped_handler));

            // Start the sender but don't wait for it to complete
            ex::start_detached(HPX_MOVE(sender));
        }

        /// \brief Adds a task to compute \c f() and returns immediately.
        ///
        /// \tparam F  The type of the user defined function to invoke.
        /// \tparam Ts The type of additional arguments used to invoke \c f().
        ///
        /// \param f   The user defined function to invoke inside the task
        ///            group.
        /// \param ts  Additional arguments to use to invoke \c f().

        template <typename F, typename... Ts>
        // clang-format off
            requires (
                !hpx::traits::is_executor_any_v<std::decay_t<F>> &&
                !hpx::execution::experimental::is_scheduler_v<std::decay_t<F>>
            )
        // clang-format on
        void run(F&& f, Ts&&... ts)
        {
            run(execution::parallel_executor{}, HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        /// \brief Waits for all tasks in the group to complete or be cancelled.
        HPX_CORE_EXPORT void wait();

        /// \brief Adds an exception to this \c task_group
        HPX_CORE_EXPORT void add_exception(std::exception_ptr p);

    private:
        template <typename F, typename... Ts>
        auto wrap_task(F&& f, Ts&&... ts)
        {
            // make sure exceptions don't leave the latch in the wrong state
            if (latch_.reset_if_needed_and_count_up(1, 1))
            {
                has_arrived_.store(false, std::memory_order_release);
            }

            auto on_exit =
                hpx::experimental::scope_exit([this] { latch_.count_down(1); });

            return [this, on_exit = HPX_MOVE(on_exit), f = HPX_FORWARD(F, f),
                       ... ts = HPX_FORWARD(Ts, ts)]() mutable {
                // latch needs to be released before the lambda exits
                auto _(HPX_MOVE(on_exit));

                hpx::detail::try_catch_exception_ptr(
                    [&]() { HPX_INVOKE(f, ts...); },
                    [this](
                        std::exception_ptr e) { add_exception(HPX_MOVE(e)); });
            };
        }

    private:
        friend class serialization::access;

        static constexpr void serialize(
            serialization::input_archive&, unsigned const) noexcept
        {
        }
        HPX_CORE_EXPORT void serialize(
            serialization::output_archive&, unsigned const);

    private:
        using shared_state_type = lcos::detail::future_data<void>;

        hpx::lcos::local::latch latch_;
        hpx::intrusive_ptr<shared_state_type> state_;
        hpx::exception_list errors_;
        std::atomic<bool> has_arrived_;
    };
}    // namespace hpx::experimental

namespace hpx::execution::experimental {

    using task_group HPX_DEPRECATED_V(1, 9,
        "hpx::execution:experimental::task_group is deprecated, use "
        "hpx::experimental::task_group instead") =
        hpx::experimental::task_group;
}    // namespace hpx::execution::experimental
