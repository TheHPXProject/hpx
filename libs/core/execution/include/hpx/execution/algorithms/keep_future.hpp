//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/execution/algorithms/detail/partial_algorithm.hpp>
#include <hpx/execution/algorithms/detail/sync_wait_domain.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>

#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    namespace detail {

        template <typename OperationState>
        void start_sender_operation_state(OperationState& op_state) noexcept
        {
            if constexpr (requires { op_state.start(); })
            {
                op_state.start();
            }
            else
            {
                hpx::execution::experimental::start(op_state);
            }
        }

        HPX_CXX_CORE_EXPORT template <typename Receiver, typename Future>
        struct operation_state
        {
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;
            std::decay_t<Future> future;

            void start() & noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        // Keep the ready-future case cheap: if the shared
                        // state is already ready we can complete immediately
                        // instead of installing a callback that fires right
                        // away anyway.
                        if (future.is_ready())
                        {
                            hpx::execution::experimental::set_value(
                                HPX_MOVE(receiver), HPX_MOVE(future));
                            return;
                        }

                        auto state =
                            hpx::traits::detail::get_shared_state(future);

                        if (!state)
                        {
                            HPX_THROW_EXCEPTION(hpx::error::no_state,
                                "operation_state::start",
                                "the future has no valid shared state");
                        }

                        // The operation state has to be kept alive until
                        // set_value is called, which means that we don't need
                        // to move receiver and future into the on_completed
                        // callback.
                        state->set_on_completed([this]() mutable {
                            hpx::execution::experimental::set_value(
                                HPX_MOVE(receiver), HPX_MOVE(future));
                        });
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(receiver), HPX_MOVE(ep));
                    });
            }
        };

        template <typename Future, typename Scheduler>
        struct keep_future_continues_on_sender;

        template <typename Receiver, typename Future, typename Scheduler>
        struct keep_future_continues_on_operation_state
        {
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;
            std::decay_t<Future> future;
            std::decay_t<Scheduler> scheduler;
            using receiver_type = std::decay_t<Receiver>;
            using future_type = std::decay_t<Future>;
            using scheduler_type = std::decay_t<Scheduler>;

            struct scheduler_receiver
            {
                using receiver_concept =
                    hpx::execution::experimental::receiver_t;

                keep_future_continues_on_operation_state* op_state;

                [[nodiscard]] constexpr decltype(auto) get_env() const
                    noexcept(noexcept(hpx::execution::experimental::get_env(
                        op_state->receiver)))
                {
                    return hpx::execution::experimental::get_env(
                        op_state->receiver);
                }

                void set_value() && noexcept
                {
                    hpx::detail::try_catch_exception_ptr(
                        [&]() {
                            hpx::execution::experimental::set_value(
                                HPX_MOVE(op_state->receiver),
                                HPX_MOVE(op_state->future));
                        },
                        [&](std::exception_ptr ep) {
                            hpx::execution::experimental::set_error(
                                HPX_MOVE(op_state->receiver), HPX_MOVE(ep));
                        });
                }

                void set_error(std::exception_ptr ep) && noexcept
                {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(op_state->receiver), HPX_MOVE(ep));
                }

                void set_stopped() && noexcept
                {
                    hpx::execution::experimental::set_stopped(
                        HPX_MOVE(op_state->receiver));
                }
            };

            using schedule_sender_type =
                decltype(hpx::execution::experimental::schedule(
                    std::declval<scheduler_type&>()));
            using schedule_operation_state_type =
                decltype(hpx::execution::experimental::connect(
                    std::declval<schedule_sender_type&&>(),
                    std::declval<scheduler_receiver>()));

            std::optional<schedule_operation_state_type> schedule_op_state;

            void schedule_completion() && noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        auto schedule_sender =
                            hpx::execution::experimental::schedule(scheduler);
                        schedule_op_state.emplace(
                            hpx::execution::experimental::connect(
                                HPX_MOVE(schedule_sender),
                                scheduler_receiver{this}));
                        start_sender_operation_state(*schedule_op_state);
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(receiver), HPX_MOVE(ep));
                    });
            }

            void start() & noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        if (future.is_ready())
                        {
                            HPX_MOVE(*this).schedule_completion();
                            return;
                        }

                        auto state =
                            hpx::traits::detail::get_shared_state(future);

                        if (!state)
                        {
                            HPX_THROW_EXCEPTION(hpx::error::no_state,
                                "keep_future_continues_on_operation_state::"
                                "start",
                                "the future has no valid shared state");
                        }

                        state->set_on_completed([this]() mutable {
                            HPX_MOVE(*this).schedule_completion();
                        });
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(receiver), HPX_MOVE(ep));
                    });
            }
        };

        HPX_CXX_CORE_EXPORT template <typename Future>
        struct keep_future_sender_base
        {
            std::decay_t<Future> future;

            // Keep the environment structurally empty, but advertise the
            // HPX sync-wait domain so composed sender chains can route the
            // final blocking wait through the cooperative HPX-aware path.
            struct env_type : hpx::execution::experimental::empty_env
            {
                template <typename CPO>
                [[nodiscard]]
                static constexpr auto query(
                    hpx::execution::experimental::get_completion_domain_t<
                        CPO>) noexcept
                    -> hpx::execution::experimental::detail::sync_wait_domain
                {
                    return {};
                }
            };

            using completion_signatures =
                hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(
                        std::decay_t<Future>),
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr),
                    hpx::execution::experimental::set_stopped_t()>;

            constexpr env_type get_env() const noexcept
            {
                return {};
            }
        };

        HPX_CXX_CORE_EXPORT template <typename Future>
        struct keep_future_sender;

        template <typename T>
        struct keep_future_sender<hpx::future<T>>
          : public keep_future_sender_base<hpx::future<T>>
        {
            using sender_concept = hpx::execution::experimental::sender_t;
            using future_type = hpx::future<T>;
            using base_type = keep_future_sender_base<hpx::future<T>>;
            using base_type::future;

            template <typename Future,
                typename = std::enable_if_t<
                    !std::is_same_v<std::decay_t<Future>, keep_future_sender>>>
            explicit keep_future_sender(Future&& future)
              : base_type{HPX_FORWARD(Future, future)}
            {
            }

            keep_future_sender(keep_future_sender&&) = default;
            keep_future_sender& operator=(keep_future_sender&&) = default;
            keep_future_sender(keep_future_sender const&) = delete;
            keep_future_sender& operator=(keep_future_sender const&) = delete;

            template <typename Self, typename... Env>
            static consteval auto get_completion_signatures() noexcept ->
                typename base_type::completion_signatures
            {
                return {};
            }

            template <typename Receiver>
            operation_state<Receiver, future_type> connect(
                Receiver&& receiver) &&
            {
                return {HPX_FORWARD(Receiver, receiver), HPX_MOVE(future)};
            }
        };

        template <typename T>
        struct keep_future_sender<hpx::shared_future<T>>
          : keep_future_sender_base<hpx::shared_future<T>>
        {
            using sender_concept = hpx::execution::experimental::sender_t;
            using future_type = hpx::shared_future<T>;
            using base_type = keep_future_sender_base<hpx::shared_future<T>>;
            using base_type::future;

            template <typename Future,
                typename = std::enable_if_t<
                    !std::is_same_v<std::decay_t<Future>, keep_future_sender>>>
            explicit keep_future_sender(Future&& future)
              : base_type{HPX_FORWARD(Future, future)}
            {
            }

            keep_future_sender(keep_future_sender&&) = default;
            keep_future_sender& operator=(keep_future_sender&&) = default;
            keep_future_sender(keep_future_sender const&) = default;
            keep_future_sender& operator=(keep_future_sender const&) = default;

            template <typename Self, typename... Env>
            static consteval auto get_completion_signatures() noexcept ->
                typename base_type::completion_signatures
            {
                return {};
            }

            template <typename Receiver>
            operation_state<Receiver, future_type> connect(
                Receiver&& receiver) &&
            {
                return {HPX_FORWARD(Receiver, receiver), HPX_MOVE(future)};
            }

            template <typename Receiver>
            operation_state<Receiver, future_type> connect(
                Receiver&& receiver) &
            {
                return {HPX_FORWARD(Receiver, receiver), future};
            }
        };

        template <typename Future, typename Scheduler>
        struct keep_future_continues_on_sender
        {
            using sender_concept = hpx::execution::experimental::sender_t;
            using future_type = std::decay_t<Future>;
            using scheduler_type = std::decay_t<Scheduler>;

            future_type future;
            scheduler_type scheduler;

            struct env_type
            {
                scheduler_type scheduler;

                template <typename CPO>
                static constexpr auto query(
                    hpx::execution::experimental::get_completion_domain_t<
                        CPO>) noexcept
                    -> hpx::execution::experimental::detail::sync_wait_domain
                {
                    return {};
                }

                template <typename CPO>
                    requires(std::is_same_v<CPO,
                                 hpx::execution::experimental::set_value_t> ||
                        std::is_same_v<CPO,
                            hpx::execution::experimental::set_stopped_t>)
                constexpr auto query(
                    hpx::execution::experimental::get_completion_scheduler_t<
                        CPO>) const noexcept
                {
                    return scheduler;
                }
            };

            constexpr env_type get_env() const noexcept
            {
                return {scheduler};
            }

            using completion_signatures =
                hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(future_type),
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr),
                    hpx::execution::experimental::set_stopped_t()>;

            template <typename Self, typename... Env>
            static consteval auto get_completion_signatures() noexcept
                -> completion_signatures
            {
                return {};
            }

            template <typename Receiver>
            auto connect(Receiver&& receiver) &&
            {
                return keep_future_continues_on_operation_state<Receiver,
                    Future, Scheduler>{HPX_FORWARD(Receiver, receiver),
                    HPX_MOVE(future), HPX_MOVE(scheduler)};
            }

            template <typename Receiver>
            auto connect(Receiver&& receiver) &
            {
                return keep_future_continues_on_operation_state<Receiver,
                    Future, Scheduler>{
                    HPX_FORWARD(Receiver, receiver), future, scheduler};
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT inline constexpr struct keep_future_t final
    {
        // clang-format off
        template <typename Future,
            HPX_CONCEPT_REQUIRES_(
                hpx::traits::is_future_v<std::decay_t<Future>>
            )>
        // clang-format on
        constexpr HPX_FORCEINLINE auto operator()(Future&& future) const
        {
            return detail::keep_future_sender<std::decay_t<Future>>(
                HPX_FORWARD(Future, future));
        }
    } keep_future{};
}    // namespace hpx::execution::experimental
