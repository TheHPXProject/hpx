//  Copyright (c) 2025 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file thread_pool_continues_on_sender.hpp
/// \brief Domain-optimized continues_on sender for HPX thread pool schedulers.
///
/// When stdexec::continues_on is applied with an HPX thread_pool_scheduler,
/// the generic stdexec implementation creates two operation states: one for
/// the source sender and one for the scheduler hop (schedule_from). This
/// double-state overhead adds ~1 us per task in microbenchmarks.
///
/// This header provides a custom sender that replaces the generic path by:
///   1. Connecting the source sender to a lightweight receiver that captures
///      the downstream receiver and the scheduler.
///   2. On set_value: if we are already on the same HPX thread pool,
///      forwarding inline; otherwise, posting a single HPX task to forward
///      the values.
///
/// This eliminates the intermediate __storage_for_t, the second operation
/// state, and the run_loop overhead.

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental::detail {

    // Forward declarations
    template <typename Sender, typename Scheduler>
    struct thread_pool_continues_on_sender;

    template <typename Receiver, typename Scheduler>
    struct continues_on_receiver;

    template <typename Sender, typename Receiver, typename Scheduler>
    struct continues_on_operation_state;

    ///////////////////////////////////////////////////////////////////////////
    // continues_on_receiver: receives completion from the source sender
    // and either forwards inline (if on the target HPX thread pool) or
    // posts to the thread pool (if not on the target pool).
    //
    // This is the core optimization: instead of stdexec's two-phase approach
    // (store completion in __storage -> schedule -> forward), we post a
    // single HPX task that carries the values directly to the downstream
    // receiver.
    template <typename Receiver, typename Scheduler>
    struct continues_on_receiver
    {
        using receiver_concept = hpx::execution::experimental::receiver_t;

        HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver_;
        HPX_NO_UNIQUE_ADDRESS std::decay_t<Scheduler> scheduler_;

        template <typename Receiver_, typename Scheduler_>
        continues_on_receiver(Receiver_&& r, Scheduler_&& s)
          : receiver_(HPX_FORWARD(Receiver_, r))
          , scheduler_(HPX_FORWARD(Scheduler_, s))
        {
        }

        continues_on_receiver(continues_on_receiver&&) = default;
        continues_on_receiver& operator=(continues_on_receiver&&) = default;
        continues_on_receiver(continues_on_receiver const&) = default;
        continues_on_receiver& operator=(
            continues_on_receiver const&) = default;

        ~continues_on_receiver() = default;

        // set_value: The source sender completed with values.
        //
        // Fast path: if the current HPX thread belongs to the same thread
        // pool as the scheduler's target pool, forward the values inline
        // -- P2300 only requires execution on the correct resource, not
        // necessarily a new OS thread.
        //
        // Slow path: if we are NOT on the target HPX pool (different pool,
        // or an external OS thread), post a task to the target pool that
        // forwards the values to the downstream receiver.
        template <typename... Ts>
        void set_value(Ts&&... ts) && noexcept
        {
            // Check if we are already running on the target pool.
            auto* self = hpx::threads::get_self_ptr();
            if (self != nullptr)
            {
                auto* target_pool = scheduler_.get_thread_pool();
                auto* current_pool = hpx::this_thread::get_pool();
                if (current_pool == target_pool)
                {
                    // Fast path: same pool -- forward inline.
                    // set_value is noexcept per P2300, no try-catch needed.
                    hpx::execution::experimental::set_value(
                        HPX_MOVE(receiver_), HPX_FORWARD(Ts, ts)...);
                    return;
                }
            }

            // Slow path: not on target pool -- schedule onto it.
            //
            // Capture receiver_ by reference in the outer lambda so
            // that if scheduler_.execute() throws synchronously, the
            // catch block can still deliver set_error on the valid
            // receiver. Use hpx::bind_back to safely capture the
            // forwarded values by value instead of C++20 pack
            // init-capture.
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    scheduler_.execute(
                        [this,
                            f = hpx::bind_back(
                                hpx::execution::experimental::set_value,
                                HPX_FORWARD(Ts, ts)...)]() mutable {
                            HPX_MOVE(f)(HPX_MOVE(receiver_));
                        });
                },
                [&](std::exception_ptr ep) {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(receiver_), HPX_MOVE(ep));
                });
        }

        // set_error: Forward errors directly. Errors are delivered on
        // whichever context the source sender completed -- this matches
        // stdexec semantics (errors bypass the scheduling hop).
        void set_error(std::exception_ptr ep) && noexcept
        {
            hpx::execution::experimental::set_error(
                HPX_MOVE(receiver_), HPX_MOVE(ep));
        }

        // set_stopped: Forward the stopped signal directly.
        void set_stopped() && noexcept
        {
            hpx::execution::experimental::set_stopped(HPX_MOVE(receiver_));
        }

        // get_env: forward from downstream receiver so dependent senders
        // can query stop tokens, allocators, etc.
        decltype(auto) get_env() const noexcept
        {
            return hpx::execution::experimental::get_env(receiver_);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // continues_on_operation_state: connects the source sender to our
    // optimized receiver, and delegates start() to the resulting inner
    // operation state.
    //
    // Single operation state -- this is the key difference from generic
    // stdexec which creates TWO operation states (one for source, one for
    // the schedule hop).
    template <typename Sender, typename Receiver, typename Scheduler>
    struct continues_on_operation_state
    {
        using receiver_type =
            continues_on_receiver<std::decay_t<Receiver>, Scheduler>;

        using inner_op_state_type =
            hpx::execution::experimental::connect_result_t<Sender,
                receiver_type>;

        std::optional<inner_op_state_type> inner_op_;

        template <typename Sender_, typename Receiver_, typename Scheduler_>
        continues_on_operation_state(
            Sender_&& sndr, Receiver_&& rcvr, Scheduler_&& sched)
        {
            inner_op_.emplace(hpx::util::detail::with_result_of([&]() {
                return hpx::execution::experimental::connect(
                    HPX_FORWARD(Sender_, sndr),
                    receiver_type{HPX_FORWARD(Receiver_, rcvr),
                        HPX_FORWARD(Scheduler_, sched)});
            }));
        }

        continues_on_operation_state(continues_on_operation_state&&) = delete;
        continues_on_operation_state(
            continues_on_operation_state const&) = delete;
        continues_on_operation_state& operator=(
            continues_on_operation_state&&) = delete;
        continues_on_operation_state& operator=(
            continues_on_operation_state const&) = delete;

        ~continues_on_operation_state() = default;

        void start() & noexcept
        {
            hpx::execution::experimental::start(*inner_op_);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // thread_pool_continues_on_sender: a P2300-compliant sender that wraps
    // a source sender and an HPX thread pool scheduler, implementing
    // continues_on with a single operation state instead of stdexec's
    // double-state generic path.
    //
    // Completion signatures are dynamically computed from the source
    // sender's signatures, augmented with set_error(exception_ptr) for the
    // scheduling hop.
    template <typename Sender, typename Scheduler>
    struct thread_pool_continues_on_sender
    {
        using sender_concept = hpx::execution::experimental::sender_t;

        HPX_NO_UNIQUE_ADDRESS std::decay_t<Sender> sender_;
        HPX_NO_UNIQUE_ADDRESS std::decay_t<Scheduler> scheduler_;

        template <typename Sender_, typename Scheduler_>
        thread_pool_continues_on_sender(Sender_&& sndr, Scheduler_&& sched)
          : sender_(HPX_FORWARD(Sender_, sndr))
          , scheduler_(HPX_FORWARD(Scheduler_, sched))
        {
        }

        thread_pool_continues_on_sender(
            thread_pool_continues_on_sender&&) = default;
        thread_pool_continues_on_sender& operator=(
            thread_pool_continues_on_sender&&) = default;
        thread_pool_continues_on_sender(
            thread_pool_continues_on_sender const&) = default;
        thread_pool_continues_on_sender& operator=(
            thread_pool_continues_on_sender const&) = default;

        ~thread_pool_continues_on_sender() = default;

        // Completion signatures: dynamically computed from the source
        // sender's signatures. We forward all value/error/stopped channels
        // from the source sender, augmented with set_error(exception_ptr)
        // for the scheduling hop.
        template <typename Self, typename... Env>
        static consteval auto get_completion_signatures() noexcept
        {
            // Get source sender's completion signatures, then add our
            // additional error channel via transform_completion_signatures.
            auto child_sigs =
                hpx::execution::experimental::get_completion_signatures<Sender,
                    Env...>();

            return exec::transform_completion_signatures(child_sigs,
                exec::keep_completion<
                    hpx::execution::experimental::set_value_t>{},
                exec::keep_completion<
                    hpx::execution::experimental::set_error_t>{},
                exec::keep_completion<
                    hpx::execution::experimental::set_stopped_t>{},
                hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr)>{});
        }

        // connect: produce the single-state operation state.
        template <typename Receiver>
        auto connect(Receiver&& receiver) &&
        {
            return continues_on_operation_state<Sender, std::decay_t<Receiver>,
                Scheduler>{HPX_MOVE(sender_), HPX_FORWARD(Receiver, receiver),
                HPX_MOVE(scheduler_)};
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) const&
        {
            return continues_on_operation_state<Sender, std::decay_t<Receiver>,
                Scheduler>{
                sender_, HPX_FORWARD(Receiver, receiver), scheduler_};
        }

        // Sender environment: expose the scheduler as the completion
        // scheduler for set_value, and forward the source sender's domain.
        struct env
        {
            std::decay_t<Scheduler> sched_;

            template <typename CPO>
                requires meta::value<
                    meta::one_of<CPO, set_value_t, set_stopped_t>>
            auto query(
                hpx::execution::experimental::get_completion_scheduler_t<CPO>)
                const noexcept
            {
                return sched_;
            }

            auto query(
                hpx::execution::experimental::get_domain_t) const noexcept
            {
                return hpx::execution::experimental::get_domain(sched_);
            }

            template <typename CPO>
            auto query(
                hpx::execution::experimental::get_completion_domain_t<CPO>)
                const noexcept
            {
                return sched_.query(
                    hpx::execution::experimental::get_completion_domain_t<
                        CPO>{});
            }
        };

        constexpr auto get_env() const noexcept
        {
            return env{scheduler_};
        }
    };

}    // namespace hpx::execution::experimental::detail
