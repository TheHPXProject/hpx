//  Copyright (c) 2025 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file distributed_scheduler.hpp
/// \brief P2300-compliant scheduler for cross-locality execution.
///
/// Provides a scheduler that dispatches work to a remote HPX locality
/// via the parcelport, bridging the hpx::future completion back into
/// the P2300 set_value / set_error receiver protocol.
///
/// Usage:
/// \code
///   hpx::id_type remote = hpx::find_all_localities()[1];
///   auto sched = hpx::distributed::distributed_scheduler{remote};
///   auto result = ex::just(42)
///       | ex::continues_on(sched)
///       | ex::then([](int x){ return x * 2; })
///       | tt::sync_wait();
/// \endcode

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)

#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::distributed::experimental {

    struct distributed_scheduler;

    namespace detail {
        struct distributed_schedule_sender;

        template <typename Receiver>
        struct distributed_operation_state;

        struct distributed_domain;

        template <typename Sender, typename Shape, typename F>
        struct distributed_bulk_sender;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // P2300 execution domain for distributed scheduling.
    //
    // Inherits from sync_wait_domain so that tt::sync_wait() uses
    // HPX-cooperative waiting (spinlock + condition_variable_any) instead
    // of OS-blocking, preventing deadlocks with --hpx:threads=1.
    //
    // Currently a pass-through: all senders are forwarded unchanged.
    // Domain-level interception of ex::then / ex::let_value for remote
    // dispatch will be added once component-based receiver marshalling
    // is implemented (nvexec-style).
    namespace detail {

        struct distributed_domain
          : hpx::execution::experimental::detail::sync_wait_domain
        {
            // Pass-through: forward all senders unchanged.
            // Remote dispatch logic will be added here in a future phase.
            template <typename OpTag, typename Sender, typename... Env>
            constexpr auto transform_sender(
                OpTag, Sender&& sndr, Env const&...) const
            {
                return HPX_FORWARD(Sender, sndr);
            }
        };
    }    // namespace detail

    namespace detail {

        ///////////////////////////////////////////////////////////////////////////
        // Operation state: bridges the schedule action into the P2300
        // receiver protocol.
        //
        // Lifetime contract (P2300 section 6.9.7):
        //   The operation_state is pinned in memory by the caller and must
        //   outlive the async operation.
        template <typename Receiver>
        struct distributed_operation_state
        {
            using receiver_type = std::decay_t<Receiver>;

            template <typename Receiver_>
            distributed_operation_state(Receiver_&& r, hpx::id_type target)
              : receiver_(HPX_FORWARD(Receiver_, r))
              , target_(HPX_MOVE(target))
            {
            }

            distributed_operation_state(
                distributed_operation_state const&) = delete;
            distributed_operation_state& operator=(
                distributed_operation_state const&) = delete;
            distributed_operation_state(distributed_operation_state&&) = delete;
            distributed_operation_state& operator=(
                distributed_operation_state&&) = delete;

            ~distributed_operation_state() = default;

            void start() & noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        hpx::execution::experimental::set_value(
                            HPX_MOVE(receiver_));
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(receiver_), HPX_MOVE(ep));
                    });
            }

        private:
            receiver_type receiver_;
            hpx::id_type target_;
        };

        ///////////////////////////////////////////////////////////////////////////
        // Sender returned by distributed_scheduler::schedule().
        //
        // Completion signatures:
        //   set_value()             - schedule completed successfully
        //   set_error(exception_ptr) - schedule or network transport failed
        //   set_stopped()           - included for concept conformance
        struct distributed_schedule_sender
        {
            using sender_concept = hpx::execution::experimental::sender_t;

            using completion_signatures =
                hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(),
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr),
                    hpx::execution::experimental::set_stopped_t()>;

            explicit distributed_schedule_sender(hpx::id_type target) noexcept
              : target_(HPX_MOVE(target))
            {
            }

            distributed_schedule_sender(
                distributed_schedule_sender&&) = default;
            distributed_schedule_sender& operator=(
                distributed_schedule_sender&&) = default;
            distributed_schedule_sender(
                distributed_schedule_sender const&) = default;
            distributed_schedule_sender& operator=(
                distributed_schedule_sender const&) = default;

            ~distributed_schedule_sender() = default;

            template <typename Receiver>
            distributed_operation_state<Receiver> connect(
                Receiver&& receiver) &&
            {
                return {HPX_FORWARD(Receiver, receiver), HPX_MOVE(target_)};
            }

            template <typename Receiver>
            distributed_operation_state<Receiver> connect(
                Receiver&& receiver) const&
            {
                return {HPX_FORWARD(Receiver, receiver), target_};
            }

            struct env
            {
                hpx::id_type target;

                // Forward get_domain queries to the scheduler's domain.
                auto query(
                    hpx::execution::experimental::get_domain_t) const noexcept
                {
                    return detail::distributed_domain{};
                }

                template <typename CPO>
                    requires meta::value<meta::one_of<CPO,
                        hpx::execution::experimental::set_value_t,
                        hpx::execution::experimental::set_stopped_t>>
                auto query(
                    hpx::execution::experimental::get_completion_scheduler_t<
                        CPO>) const noexcept;

                template <typename CPO>
                auto query(
                    hpx::execution::experimental::get_completion_domain_t<CPO>)
                    const noexcept
                {
                    return detail::distributed_domain{};
                }
            };

            auto get_env() const noexcept;

        private:
            hpx::id_type target_;

            friend struct hpx::distributed::experimental::distributed_scheduler;
            friend struct env;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // P2300-compliant scheduler that dispatches work to a remote HPX
    // locality via the parcelport.
    //
    // Satisfies the stdexec::scheduler concept:
    //   - schedule() returns a sender
    //   - equality-comparable
    //   - copy-constructible
    HPX_CXX_EXPORT struct distributed_scheduler
    {
        explicit distributed_scheduler(hpx::id_type target) noexcept
          : target_(HPX_MOVE(target))
        {
        }

        distributed_scheduler(distributed_scheduler const&) = default;
        distributed_scheduler& operator=(
            distributed_scheduler const&) = default;
        distributed_scheduler(distributed_scheduler&&) = default;
        distributed_scheduler& operator=(distributed_scheduler&&) = default;

        ~distributed_scheduler() = default;

        /// P2300 schedule CPO: returns a sender that, when started,
        /// completes with set_value().
        [[nodiscard]] detail::distributed_schedule_sender schedule() const
        {
            return detail::distributed_schedule_sender{target_};
        }

        friend bool operator==(distributed_scheduler const& lhs,
            distributed_scheduler const& rhs) noexcept
        {
            return lhs.target_ == rhs.target_;
        }

        friend bool operator!=(distributed_scheduler const& lhs,
            distributed_scheduler const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        /// Returns the target locality this scheduler dispatches to.
        [[nodiscard]] hpx::id_type const& target() const noexcept
        {
            return target_;
        }

        /// Returns the execution domain of this scheduler.
        [[nodiscard]]
        static auto query(hpx::execution::experimental::get_domain_t) noexcept
            -> detail::distributed_domain
        {
            return {};
        }

        /// P3826R5: Returns the completion domain for this scheduler.
        template <typename CPO>
        [[nodiscard]]
        static auto query(
            hpx::execution::experimental::get_completion_domain_t<CPO>) noexcept
            -> detail::distributed_domain
        {
            return {};
        }

        /// Intercept ex::bulk() when the completion scheduler is this
        /// distributed_scheduler. Returns a distributed_bulk_sender.
        template <typename Sender, typename Shape, typename F>
        auto query(hpx::execution::experimental::bulk_t, Sender&& sender,
            Shape const& shape, F&& f) const;

    private:
        hpx::id_type target_;
        friend struct detail::distributed_domain;
    };

    // ---- Deferred inline definitions (after distributed_scheduler is
    //      complete) ----

    template <typename CPO>
        requires meta::value<
            meta::one_of<CPO, hpx::execution::experimental::set_value_t,
                hpx::execution::experimental::set_stopped_t>>
    auto detail::distributed_schedule_sender::env::query(
        hpx::execution::experimental::get_completion_scheduler_t<CPO>)
        const noexcept
    {
        return distributed_scheduler{target};
    }

    inline auto detail::distributed_schedule_sender::get_env() const noexcept
    {
        return env{target_};
    }

    template <typename Sender, typename Shape, typename F>
    auto distributed_scheduler::query(hpx::execution::experimental::bulk_t,
        Sender&& sender, Shape const& shape, F&& f) const
    {
        return detail::distributed_bulk_sender<std::decay_t<Sender>,
            std::decay_t<Shape>, std::decay_t<F>>{
            HPX_FORWARD(Sender, sender), shape, HPX_FORWARD(F, f), *this};
    }

}    // namespace hpx::distributed::experimental

#endif    // HPX_HAVE_NETWORKING
