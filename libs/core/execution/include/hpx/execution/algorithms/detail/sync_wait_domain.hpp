//  Copyright (c) 2026 The HPX Authors
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution/algorithms/run_loop.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/synchronization.hpp>

#include <exception>
#include <mutex>
#include <optional>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

// HPX-aware sync_wait domain.
//
// stdexec::sync_wait normally blocks using OS condition variables via an
// internal run_loop. When called on an HPX worker thread, this can deadlock if
// completion requires more work on the same HPX thread pool, especially with
// --hpx:threads=1.
//
// sync_wait_domain customizes apply_sender(sync_wait_t, sndr) to use HPX-aware
// waiting with hpx::spinlock + hpx::lcos::local::detail::condition_variable.
// This suspends the HPX task cooperatively instead of OS-blocking the worker
// thread, allowing queued HPX work to continue running.
//
// Senders executing through HPX should expose this domain through
// get_completion_domain<set_value_t> so sync_wait routes here.

namespace hpx::execution::experimental::detail {

    HPX_CXX_CORE_EXPORT struct sync_wait_domain;

    // Marker for the set_stopped completion path.
    HPX_CXX_CORE_EXPORT template <typename T>
    struct stopped_t
    {
    };

    namespace sync_wait_detail {

        template <typename T>
        using attrs_completion_scheduler_t = std::decay_t<
            decltype(hpx::execution::experimental::get_completion_scheduler<
                hpx::execution::experimental::set_value_t>(
                hpx::execution::experimental::get_env(std::declval<T&>())))>;

        template <typename Sender, typename = void>
        struct can_attrs_completion_scheduler : std::false_type
        {
        };

        template <typename Sender>
        struct can_attrs_completion_scheduler<Sender,
            std::void_t<
                attrs_completion_scheduler_t<std::remove_cvref_t<Sender>>>>
          : std::integral_constant<bool,
                hpx::execution::experimental::scheduler<
                    attrs_completion_scheduler_t<std::remove_cvref_t<Sender>>>>
        {
        };
    }    // namespace sync_wait_detail

    HPX_CXX_CORE_EXPORT template <typename Sender, typename Enable = void>
    inline constexpr bool sync_wait_can_query_attrs_completion_scheduler_v =
        false;

    template <typename Sender>
    inline constexpr bool sync_wait_can_query_attrs_completion_scheduler_v<
        Sender,
        std::enable_if_t<
            sync_wait_detail::can_attrs_completion_scheduler<Sender>::value>> =
        true;

    HPX_CXX_CORE_EXPORT template <typename Sender, typename Enable = void>
    struct sync_wait_rcv_scheduler_impl
    {
        using type = std::decay_t<
            decltype(std::declval<hpx::execution::experimental::run_loop&>()
                    .get_scheduler())>;
    };

    template <typename Sender>
    struct sync_wait_rcv_scheduler_impl<Sender,
        std::enable_if_t<
            sync_wait_detail::can_attrs_completion_scheduler<Sender>::value>>
    {
        using type = sync_wait_detail::attrs_completion_scheduler_t<
            std::remove_cvref_t<Sender>>;
    };

    HPX_CXX_CORE_EXPORT template <typename Sender>
    using sync_wait_rcv_scheduler_t =
        sync_wait_rcv_scheduler_impl<Sender>::type;

    // Receiver env: exposes a stdexec run_loop scheduler via the standard
    // get_scheduler / get_delegation_scheduler queries so dependent senders
    // (let_value, let_error, etc.) can compute their completion signatures
    // against this environment. The run_loop is owned by shared_state and is
    // NEVER actually run; it exists purely as a type carrier required by
    // stdexec::sync_wait_t's constraint (sender_in<sync_wait::__env>).
    //
    // Modern P2300: Uses query() functions instead of old tag_invoke.
    //
    // IMPORTANT: We intentionally DO NOT provide a stop token.
    //
    // If we return never_stop_token, stdexec assumes the operation can never be
    // stopped, so it removes set_stopped from the completion signatures.
    //
    // But our code may still call set_stopped(). That creates type mismatches
    // and breaks things.
    //
    // By not exposing a stop token, stdexec keeps set_stopped in the completion
    // signatures, which matches our behavior.
    HPX_CXX_CORE_EXPORT template <typename Sender>
    struct sync_wait_rcv_env
    {
        using scheduler_type = sync_wait_rcv_scheduler_t<Sender>;

        scheduler_type sched_;

        constexpr explicit sync_wait_rcv_env(scheduler_type sched) noexcept
          : sched_(HPX_MOVE(sched))
        {
        }

        template <typename S>
        static sync_wait_rcv_env make(
            S&& sndr, hpx::execution::experimental::run_loop& fallback_loop)
        {
            if constexpr (sync_wait_can_query_attrs_completion_scheduler_v<
                              std::remove_cvref_t<S>>)
            {
                return sync_wait_rcv_env(
                    hpx::execution::experimental::get_completion_scheduler<
                        hpx::execution::experimental::set_value_t>(
                        hpx::execution::experimental::get_env(sndr)));
            }
            else
            {
                return sync_wait_rcv_env(fallback_loop.get_scheduler());
            }
        }

        [[nodiscard]]
        constexpr auto query(
            hpx::execution::experimental::get_scheduler_t) const noexcept
        {
            return sched_;
        }

        [[nodiscard]]
        constexpr auto query(
            hpx::execution::experimental::get_start_scheduler_t) const noexcept
        {
            return sched_;
        }

        [[nodiscard]]
        constexpr auto query(
            hpx::execution::experimental::get_delegation_scheduler_t)
            const noexcept
        {
            return sched_;
        }

        template <typename CPO>
        [[nodiscard]]
        static constexpr auto query(
            hpx::execution::experimental::get_completion_domain_t<CPO>) noexcept
            -> sync_wait_domain;
    };

    // Compute the single-value tuple type for a sender against the receiver
    // env, using only public stdexec APIs.
    HPX_CXX_CORE_EXPORT template <typename... Ts>
    using decayed_std_tuple = std::tuple<std::decay_t<Ts>...>;

    HPX_CXX_CORE_EXPORT template <typename Variant>
    struct first_alternative;

    template <typename T>
    struct first_alternative<std::variant<T>>
    {
        using type = T;
    };

    HPX_CXX_CORE_EXPORT template <typename Sender>
    using value_tuple_for_t =
        first_alternative<hpx::execution::experimental::value_types_of_t<Sender,
            sync_wait_rcv_env<Sender>, decayed_std_tuple, std::variant>>::type;

    HPX_CXX_CORE_EXPORT template <typename Sender>
    using into_variant_sender_t =
        std::remove_cvref_t<decltype(hpx::execution::experimental::into_variant(
            std::declval<Sender>()))>;

    HPX_CXX_CORE_EXPORT template <typename Sender>
    using value_variant_for_t = std::tuple_element_t<0,
        value_tuple_for_t<into_variant_sender_t<Sender>>>;

    HPX_CXX_CORE_EXPORT template <typename ValueTuple>
    struct shared_state
    {
        hpx::spinlock mtx;
        hpx::lcos::local::detail::condition_variable cv;
        hpx::execution::experimental::run_loop loop;
        bool done = false;
        std::variant<std::monostate, ValueTuple, std::exception_ptr,
            stopped_t<ValueTuple>>
            result;

        void notify_all(std::unique_lock<hpx::spinlock> l)
        {
            cv.notify_all(HPX_MOVE(l));
        }

        template <typename... Vs>
        void notify_value(Vs&&... vs) noexcept
        {
            try
            {
                std::unique_lock<hpx::spinlock> l(mtx);
                result.template emplace<ValueTuple>(HPX_FORWARD(Vs, vs)...);
                done = true;
                notify_all(HPX_MOVE(l));
            }
            catch (...)
            {
                std::unique_lock<hpx::spinlock> l(mtx);
                result.template emplace<std::exception_ptr>(
                    std::current_exception());
                done = true;
                notify_all(HPX_MOVE(l));
            }
        }

        template <typename E>
        void notify_error(E&& e) noexcept
        {
            std::unique_lock<hpx::spinlock> l(mtx);
            using err_t = std::decay_t<E>;
            if constexpr (std::is_same_v<err_t, std::exception_ptr>)
            {
                result.template emplace<std::exception_ptr>(HPX_FORWARD(E, e));
            }
            else if constexpr (std::is_same_v<err_t, std::error_code>)
            {
                result.template emplace<std::exception_ptr>(
                    std::make_exception_ptr(std::system_error(e)));
            }
            else
            {
                try
                {
                    throw HPX_FORWARD(E, e);
                }
                catch (...)
                {
                    result.template emplace<std::exception_ptr>(
                        std::current_exception());
                }
            }
            done = true;
            notify_all(HPX_MOVE(l));
        }

        void notify_stopped() noexcept
        {
            std::unique_lock<hpx::spinlock> l(mtx);
            result.template emplace<stopped_t<ValueTuple>>();
            done = true;
            notify_all(HPX_MOVE(l));
        }

        // Wait HPX-aware: yields the calling HPX task while waiting, does not
        // block the underlying OS thread.
        std::optional<ValueTuple> wait_get_value()
        {
            {
                std::unique_lock<hpx::spinlock> l(mtx);
                while (!done)
                {
                    cv.wait(l, "sync_wait_domain::wait_get_value");
                }
            }

            loop.finish();
            loop.run();

            if (auto* v = std::get_if<ValueTuple>(&result))
            {
                return std::optional<ValueTuple>(HPX_MOVE(*v));
            }
            if (auto* ep = std::get_if<std::exception_ptr>(&result))
            {
                auto e = HPX_MOVE(*ep);
                std::rethrow_exception(HPX_MOVE(e));
            }
            // set_stopped
            return std::optional<ValueTuple>{};
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Sender, typename ValueTuple>
    struct receiver
    {
        using receiver_concept = hpx::execution::experimental::receiver_t;

        shared_state<ValueTuple>* state;
        sync_wait_rcv_env<Sender> env_;

        template <typename... Vs>
        constexpr void set_value(Vs&&... vs) && noexcept
        {
            state->notify_value(HPX_FORWARD(Vs, vs)...);
        }

        template <typename E>
        constexpr void set_error(E&& e) && noexcept
        {
            state->notify_error(HPX_FORWARD(E, e));
        }

        constexpr void set_stopped() && noexcept
        {
            state->notify_stopped();
        }

        [[nodiscard]]
        constexpr sync_wait_rcv_env<Sender> get_env() const noexcept
        {
            return env_;
        }
    };

    // stdexec domain customizing only `apply_sender(sync_wait_t, ...)` to use
    // HPX-aware cooperative waiting.
    HPX_CXX_CORE_EXPORT struct sync_wait_domain
      : hpx::execution::experimental::default_domain
    {
        // P2300/P2855 customization: stdexec::sync_wait dispatches here when
        // the sender's completion domain is thread_pool_domain. We implement
        // sync_wait using HPX synchronization primitives so the calling HPX
        // task yields cooperatively rather than the OS thread being blocked,
        // avoiding deadlock with --hpx:threads=1.
        template <typename Sender>
            requires(hpx::execution::experimental::sender_in<Sender,
                sync_wait_rcv_env<Sender>>)
        auto apply_sender(hpx::execution::experimental::sync_wait_t,
            Sender&& sndr) const -> std::optional<value_tuple_for_t<Sender>>
        {
            using value_tuple_t = value_tuple_for_t<Sender>;

            shared_state<value_tuple_t> state;
            auto env = sync_wait_rcv_env<Sender>::make(sndr, state.loop);

            auto op_state =
                hpx::execution::experimental::connect(HPX_FORWARD(Sender, sndr),
                    receiver<Sender, value_tuple_t>{&state, HPX_MOVE(env)});
            hpx::execution::experimental::start(op_state);
            return state.wait_get_value();
        }

        template <typename Sender>
            requires(hpx::execution::experimental::sender_in<
                into_variant_sender_t<Sender>,
                sync_wait_rcv_env<into_variant_sender_t<Sender>>>)
        auto apply_sender(
            hpx::execution::experimental::sync_wait_with_variant_t,
            Sender&& sndr) const -> std::optional<value_variant_for_t<Sender>>
        {
            auto opt = apply_sender(hpx::execution::experimental::sync_wait_t{},
                hpx::execution::experimental::into_variant(
                    HPX_FORWARD(Sender, sndr)));
            if (!opt)
            {
                return std::nullopt;
            }
            return std::make_optional(std::get<0>(std::move(*opt)));
        }
    };

    template <typename Sender>
    template <typename CPO>
    constexpr auto sync_wait_rcv_env<Sender>::query(
        hpx::execution::experimental::get_completion_domain_t<CPO>) noexcept
        -> sync_wait_domain
    {
        return {};
    }
}    // namespace hpx::execution::experimental::detail

namespace hpx::execution::experimental {

    // Out-of-class definition for run_loop::env_t::query declared in
    // run_loop.hpp. Defined here (not there) because the body needs the
    // complete sync_wait_domain type. Templating on CPO defers instantiation
    // so the trailing-return forward decl in run_loop.hpp is sufficient
    // at declaration site.
    template <typename CPO>
    auto run_loop::env_t::query(get_completion_domain_t<CPO>) noexcept
        -> detail::sync_wait_domain
    {
        return {};
    }
}    // namespace hpx::execution::experimental
