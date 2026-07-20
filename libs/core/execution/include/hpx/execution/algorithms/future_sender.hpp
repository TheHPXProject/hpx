//  Copyright (c) 2025 Shivansh Singh
//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file future_sender.hpp
/// \brief Implementation details for adapting HPX futures as P2300 senders.
///
/// Usage:
/// \code
///   hpx::future<int> f = hpx::async([]{ return 42; });
///   auto result = hpx::execution::experimental::as_sender(std::move(f))
///       | hpx::execution::experimental::then([](int x){ return x * 2; })
///       | hpx::execution::experimental::sync_wait();
/// \endcode

#pragma once

#include <hpx/config.hpp>
#include <hpx/futures/future.hpp>
#include <hpx/futures/traits/future_traits.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/futures.hpp>

#include <atomic>
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental::detail {

    template <typename Result>
    struct future_sender_completion_signatures
    {
        using type = hpx::execution::experimental::completion_signatures<
            hpx::execution::experimental::set_value_t(Result),
            hpx::execution::experimental::set_error_t(std::exception_ptr)>;
    };

    template <>
    struct future_sender_completion_signatures<void>
    {
        using type = hpx::execution::experimental::completion_signatures<
            hpx::execution::experimental::set_value_t(),
            hpx::execution::experimental::set_error_t(std::exception_ptr)>;
    };

    template <typename Result>
    struct future_sender_continues_on_completion_signatures
    {
        using type = hpx::execution::experimental::completion_signatures<
            hpx::execution::experimental::set_value_t(Result),
            hpx::execution::experimental::set_error_t(std::exception_ptr),
            hpx::execution::experimental::set_stopped_t()>;
    };

    template <>
    struct future_sender_continues_on_completion_signatures<void>
    {
        using type = hpx::execution::experimental::completion_signatures<
            hpx::execution::experimental::set_value_t(),
            hpx::execution::experimental::set_error_t(std::exception_ptr),
            hpx::execution::experimental::set_stopped_t()>;
    };

    template <typename OperationState>
    void start_future_sender_operation_state(OperationState& op_state) noexcept
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

    template <typename Receiver, typename Future>
    class future_sender_operation_state
    {
    public:
        using receiver_type = std::decay_t<Receiver>;
        using future_type = std::decay_t<Future>;
        using result_type =
            typename hpx::traits::future_traits<future_type>::type;

        template <typename Receiver_>
        future_sender_operation_state(Receiver_&& r, future_type f)
          : receiver_(HPX_FORWARD(Receiver_, r))
          , future_(HPX_MOVE(f))
        {
        }

        future_sender_operation_state(
            future_sender_operation_state const&) = delete;
        future_sender_operation_state& operator=(
            future_sender_operation_state const&) = delete;
        future_sender_operation_state(future_sender_operation_state&&) = delete;
        future_sender_operation_state& operator=(
            future_sender_operation_state&&) = delete;

        void start() & noexcept
        {
            start_helper();
        }

    private:
        void start_helper() & noexcept
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    auto state = hpx::traits::detail::get_shared_state(future_);

                    if (!state)
                    {
                        HPX_THROW_EXCEPTION(hpx::error::no_state,
                            "future_sender_operation_state::start",
                            "the future has no valid shared state");
                    }

                    auto on_completed = [this]() mutable {
                        if (future_.has_value())
                        {
                            if constexpr (std::is_void_v<result_type>)
                            {
                                hpx::execution::experimental::set_value(
                                    HPX_MOVE(receiver_));
                            }
                            else
                            {
                                hpx::execution::experimental::set_value(
                                    HPX_MOVE(receiver_), future_.get());
                            }
                        }
                        else if (future_.has_exception())
                        {
                            hpx::execution::experimental::set_error(
                                HPX_MOVE(receiver_),
                                future_.get_exception_ptr());
                        }
                    };

                    if (!state->is_ready(std::memory_order_relaxed))
                    {
                        state->execute_deferred();

                        if (!state->is_ready(std::memory_order_relaxed))
                        {
                            state->set_on_completed(HPX_MOVE(on_completed));
                        }
                        else
                        {
                            on_completed();
                        }
                    }
                    else
                    {
                        on_completed();
                    }
                },
                [&](std::exception_ptr ep) {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(receiver_), HPX_MOVE(ep));
                });
        }

        receiver_type receiver_;
        future_type future_;
    };

    template <typename Future, typename Scheduler>
    struct future_sender_continues_on_sender;

    template <typename T>
    inline constexpr bool is_future_sender_v = false;

    template <typename Receiver, typename Future, typename Scheduler>
    struct future_sender_continues_on_operation_state
    {
        using receiver_type = std::decay_t<Receiver>;
        using future_type = std::decay_t<Future>;
        using scheduler_type = std::decay_t<Scheduler>;
        using result_type =
            typename hpx::traits::future_traits<future_type>::type;

        HPX_NO_UNIQUE_ADDRESS receiver_type receiver_;
        future_type future_;
        HPX_NO_UNIQUE_ADDRESS scheduler_type scheduler_;

        struct scheduler_receiver
        {
            using receiver_concept = hpx::execution::experimental::receiver_t;

            future_sender_continues_on_operation_state* op_state_;

            [[nodiscard]] constexpr decltype(auto) get_env() const
                noexcept(noexcept(hpx::execution::experimental::get_env(
                    op_state_->receiver_)))
            {
                return hpx::execution::experimental::get_env(
                    op_state_->receiver_);
            }

            void set_value() && noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        if (op_state_->future_.has_value())
                        {
                            if constexpr (std::is_void_v<result_type>)
                            {
                                hpx::execution::experimental::set_value(
                                    HPX_MOVE(op_state_->receiver_));
                            }
                            else
                            {
                                hpx::execution::experimental::set_value(
                                    HPX_MOVE(op_state_->receiver_),
                                    op_state_->future_.get());
                            }
                        }
                        else if (op_state_->future_.has_exception())
                        {
                            hpx::execution::experimental::set_error(
                                HPX_MOVE(op_state_->receiver_),
                                op_state_->future_.get_exception_ptr());
                        }
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(op_state_->receiver_), HPX_MOVE(ep));
                    });
            }

            void set_error(std::exception_ptr ep) && noexcept
            {
                hpx::execution::experimental::set_error(
                    HPX_MOVE(op_state_->receiver_), HPX_MOVE(ep));
            }

            void set_stopped() && noexcept
            {
                hpx::execution::experimental::set_stopped(
                    HPX_MOVE(op_state_->receiver_));
            }
        };

        using schedule_sender_type =
            decltype(hpx::execution::experimental::schedule(
                std::declval<scheduler_type&>()));
        using schedule_operation_state_type =
            decltype(hpx::execution::experimental::connect(
                std::declval<schedule_sender_type&&>(),
                std::declval<scheduler_receiver>()));

        std::optional<schedule_operation_state_type> schedule_op_state_;

        void schedule_completion() & noexcept
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    auto schedule_sender =
                        hpx::execution::experimental::schedule(scheduler_);
                    schedule_op_state_.emplace(
                        hpx::execution::experimental::connect(
                            HPX_MOVE(schedule_sender),
                            scheduler_receiver{this}));
                    start_future_sender_operation_state(*schedule_op_state_);
                },
                [&](std::exception_ptr ep) {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(receiver_), HPX_MOVE(ep));
                });
        }

        void start() & noexcept
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    auto state = hpx::traits::detail::get_shared_state(future_);

                    if (!state)
                    {
                        HPX_THROW_EXCEPTION(hpx::error::no_state,
                            "future_sender_continues_on_operation_state::"
                            "start",
                            "the future has no valid shared state");
                    }

                    if (!state->is_ready(std::memory_order_relaxed))
                    {
                        state->execute_deferred();

                        if (!state->is_ready(std::memory_order_relaxed))
                        {
                            state->set_on_completed(
                                [this]() mutable { schedule_completion(); });
                        }
                        else
                        {
                            schedule_completion();
                        }
                    }
                    else
                    {
                        schedule_completion();
                    }
                },
                [&](std::exception_ptr ep) {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(receiver_), HPX_MOVE(ep));
                });
        }
    };

    template <typename Future, typename Scheduler>
    struct future_sender_continues_on_sender
    {
        using sender_concept = hpx::execution::experimental::sender_t;
        using future_type = std::decay_t<Future>;
        using scheduler_type = std::decay_t<Scheduler>;
        using result_type =
            typename hpx::traits::future_traits<future_type>::type;

        future_type future_;
        HPX_NO_UNIQUE_ADDRESS scheduler_type scheduler_;

        struct env_type
        {
            HPX_NO_UNIQUE_ADDRESS scheduler_type scheduler_;

            template <typename CPO>
            constexpr auto query(
                hpx::execution::experimental::get_completion_domain_t<CPO>)
                const noexcept(noexcept(
                    hpx::execution::experimental::get_completion_domain<CPO>(
                        scheduler_))) -> decltype(hpx::execution::experimental::
                        get_completion_domain<CPO>(scheduler_))
            {
                return hpx::execution::experimental::get_completion_domain<CPO>(
                    scheduler_);
            }

            template <typename CPO>
                requires(std::is_same_v<CPO,
                             hpx::execution::experimental::set_value_t> ||
                    std::is_same_v<CPO,
                        hpx::execution::experimental::set_stopped_t>)
            constexpr auto query(
                hpx::execution::experimental::get_completion_scheduler_t<CPO>)
                const noexcept
            {
                return scheduler_;
            }
        };

        using completion_signatures =
            typename future_sender_continues_on_completion_signatures<
                result_type>::type;

        template <typename... Env>
        constexpr auto get_completion_signatures(Env&&...) const noexcept
            -> completion_signatures
        {
            return {};
        }

        constexpr env_type get_env() const noexcept
        {
            return {scheduler_};
        }

        template <typename Receiver>
        auto connect(Receiver&& r) &&
        {
            return future_sender_continues_on_operation_state<Receiver, Future,
                Scheduler>{HPX_FORWARD(Receiver, r), HPX_MOVE(future_),
                HPX_MOVE(scheduler_), std::nullopt};
        }

        template <typename Receiver>
        auto connect(Receiver&& r) &
        {
            return future_sender_continues_on_operation_state<Receiver, Future,
                Scheduler>{
                HPX_FORWARD(Receiver, r), future_, scheduler_, std::nullopt};
        }
    };

    template <typename Future>
    struct future_sender
    {
        using sender_concept = hpx::execution::experimental::sender_t;
        using future_type = std::decay_t<Future>;
        using result_type =
            typename hpx::traits::future_traits<future_type>::type;

        using completion_signatures =
            typename future_sender_completion_signatures<result_type>::type;

        explicit future_sender(future_type f) noexcept
          : future_(HPX_MOVE(f))
        {
        }

        future_sender(future_sender&&) = default;
        future_sender& operator=(future_sender&&) = default;
        future_sender(future_sender const&) = default;
        future_sender& operator=(future_sender const&) = default;

        constexpr future_type& get_future() & noexcept
        {
            return future_;
        }

        constexpr future_type const& get_future() const& noexcept
        {
            return future_;
        }

        constexpr future_type&& get_future() && noexcept
        {
            return HPX_MOVE(future_);
        }

        template <typename... Env>
        constexpr auto get_completion_signatures(Env&&...) const noexcept
            -> completion_signatures
        {
            return {};
        }

        template <typename Receiver>
        auto connect(Receiver&& r) &&
        {
            return future_sender_operation_state<Receiver, future_type>{
                HPX_FORWARD(Receiver, r), HPX_MOVE(future_)};
        }

        template <typename Receiver>
        auto connect(Receiver&& r) &
        {
            return future_sender_operation_state<Receiver, future_type>{
                HPX_FORWARD(Receiver, r), future_};
        }

    private:
        future_type future_;
    };

    template <typename Future>
    inline constexpr bool is_future_sender_v<future_sender<Future>> = true;

}    // namespace hpx::execution::experimental::detail
