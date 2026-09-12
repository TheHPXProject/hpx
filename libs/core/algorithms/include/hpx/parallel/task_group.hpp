//  Copyright (c) 2021-2026 Hartmut Kaiser
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
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

/// Top-level namespace
namespace hpx::experimental {

    namespace detail {

        struct task_group_shared_state
        {
            using shared_state_type = lcos::detail::future_data<void>;

            hpx::lcos::local::latch latch_{1};
            hpx::intrusive_ptr<shared_state_type> state_;
            hpx::exception_list errors_;
            std::atomic<bool> has_arrived_{false};
            std::atomic<bool> senders_drained_{false};
            std::atomic<bool> wait_called_{false};
            mutable hpx::spinlock mtx_;
            std::vector<hpx::execution::experimental::any_sender<>> senders_;

            void add_exception(std::exception_ptr p)
            {
                std::lock_guard<hpx::spinlock> l(mtx_);
                errors_.add(HPX_MOVE(p));
            }

            std::vector<hpx::execution::experimental::any_sender<>>
            drain_senders()
            {
                std::lock_guard<hpx::spinlock> l(mtx_);
                senders_drained_.store(true, std::memory_order_release);
                return HPX_MOVE(senders_);
            }

            void add_sender(hpx::execution::experimental::any_sender<> sender)
            {
                std::lock_guard<hpx::spinlock> l(mtx_);
                senders_.push_back(HPX_MOVE(sender));
            }

            bool has_senders() const
            {
                std::lock_guard<hpx::spinlock> l(mtx_);
                return !senders_.empty();
            }
        };

        inline hpx::execution::experimental::any_sender<>
        drain_task_group_senders(
            std::shared_ptr<task_group_shared_state> const& state)
        {
            namespace ex = hpx::execution::experimental;

            auto senders = state->drain_senders();
            if (senders.empty())
            {
                return ex::any_sender<>{ex::just()};
            }

            return ex::any_sender<>{ex::when_all_vector(HPX_MOVE(senders)) |
                ex::let_value(
                    [state]() { return drain_task_group_senders(state); })};
        }
    }    // namespace detail

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
            // make sure exceptions don't leave the latch in the wrong state
            if (state_->latch_.reset_if_needed_and_count_up(1, 1))
            {
                state_->has_arrived_.store(false, std::memory_order_release);
            }

            auto on_exit = hpx::experimental::scope_exit(
                [state = state_] { state->latch_.count_down(1); });

            hpx::parallel::execution::post(HPX_FORWARD(Executor, exec),
                [state = state_, on_exit = HPX_MOVE(on_exit),
                    f = HPX_FORWARD(F, f),
                    ... ts = HPX_FORWARD(Ts, ts)]() mutable {
                    // latch needs to be released before the lambda exits
                    auto _(HPX_MOVE(on_exit));

                    hpx::detail::try_catch_exception_ptr(
                        [&]() { HPX_INVOKE(f, ts...); },
                        [state](std::exception_ptr e) {
                            state->add_exception(HPX_MOVE(e));
                        });
                });
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

        // ---------------------------------------------------------------
        // P2300 Scheduler-based sender path
        // ---------------------------------------------------------------

        /// \brief Adds a task to compute \c f(ts...) on the given P2300
        ///        scheduler and returns immediately.
        ///
        /// Instead of eagerly posting work, this overload constructs a lazy
        /// sender graph \code ex::schedule(sched) | ex::then(f) \endcode and
        /// stores it in an internal sender vector.  The work is not submitted
        /// until \a wait() or \a wait_as_sender() is called.
        ///
        /// \tparam Scheduler  A type that models the P2300 \c scheduler
        ///                    concept.
        /// \tparam F          The type of the user defined function to invoke.
        /// \tparam Ts         The type of additional arguments used to
        ///                    invoke \c f().
        ///
        /// \param sched       The P2300 scheduler to use for execution.
        /// \param f           The user defined function to invoke inside the
        ///                    task group.
        /// \param ts          Additional arguments to use to invoke \c f().

        template <typename Scheduler, typename F, typename... Ts>
        // clang-format off
            requires (
                hpx::execution::experimental::is_scheduler_v<
                    std::decay_t<Scheduler>>
            )
        // clang-format on
        void run(Scheduler&& sched, F&& f, Ts&&... ts)
        {
            namespace ex = hpx::execution::experimental;

            state_->senders_drained_.store(false, std::memory_order_release);

            // Package the callable and arguments into a shared_ptr so that
            // they survive the lifetime of this call and can be shared
            // across the sender graph safely.
            auto shared_args = std::make_shared<
                std::tuple<std::decay_t<F>, std::decay_t<Ts>...>>(
                std::make_tuple(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));

            // Build a lazy sender: schedule on the given scheduler, then
            // invoke the callable with the captured arguments.  Exceptions
            // are caught and forwarded to the task_group's error list so
            // that they are aggregated and re-thrown from wait().
            auto sender = ex::schedule(HPX_FORWARD(Scheduler, sched)) |
                ex::then([state = state_,
                             shared_args = HPX_MOVE(shared_args)]() {
                    hpx::detail::try_catch_exception_ptr(
                        [&]() {
                            std::apply(
                                [](auto&& func, auto&&... args) {
                                    HPX_INVOKE(
                                        HPX_FORWARD(decltype(func), func),
                                        HPX_FORWARD(decltype(args), args)...);
                                },
                                HPX_MOVE(*shared_args));
                        },
                        [state](std::exception_ptr e) {
                            state->add_exception(HPX_MOVE(e));
                        });
                });

            state_->add_sender(ex::any_sender<>{HPX_MOVE(sender)});
        }

        /// \brief Returns a sender that completes when all tasks added via
        ///        the scheduler-based \a run() overload have finished.
        ///
        /// The returned sender joins all lazily accumulated senders using
        /// recursive draining.  The caller owns the returned sender and
        /// controls when to connect/start it (e.g. via \c sync_wait).
        ///
        /// \note  This method moves the internal sender vector, so the
        ///        \c task_group is left in a reusable (empty) state afterward.
        ///
        /// \returns A sender that completes with void once every accumulated
        ///          sender (including dynamically spawned ones) has completed.
        decltype(auto) wait_as_sender()
        {
            namespace ex = hpx::execution::experimental;

            state_->wait_called_.store(true, std::memory_order_release);

            return ex::just() | ex::let_value([state = state_]() {
                return detail::drain_task_group_senders(state);
            }) | ex::then([state = state_]() {
                if (state->errors_.size() != 0)
                {
                    throw state->errors_;
                }
            });
        }

        /// \brief Waits for all tasks in the group to complete or be cancelled.
        HPX_CORE_EXPORT void wait();

        /// \brief Adds an exception to this \c task_group
        HPX_CORE_EXPORT void add_exception(std::exception_ptr p);

    private:
        friend class serialization::access;

        static constexpr void serialize(
            serialization::input_archive&, unsigned const) noexcept
        {
        }
        HPX_CORE_EXPORT void serialize(
            serialization::output_archive&, unsigned const);

    private:
        std::shared_ptr<detail::task_group_shared_state> state_;
    };
}    // namespace hpx::experimental

namespace hpx::execution::experimental {

    using task_group HPX_DEPRECATED_V(1, 9,
        "hpx::execution:experimental::task_group is deprecated, use "
        "hpx::experimental::task_group instead") =
        hpx::experimental::task_group;
}    // namespace hpx::execution::experimental
