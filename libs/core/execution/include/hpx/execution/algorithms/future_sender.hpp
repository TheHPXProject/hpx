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
        void complete_receiver()
        {
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
                    HPX_MOVE(receiver_), future_.get_exception_ptr());
            }
        }

        void complete_receiver_noexcept() noexcept
        {
            hpx::detail::try_catch_exception_ptr([&]() { complete_receiver(); },
                [&](std::exception_ptr ep) {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(receiver_), HPX_MOVE(ep));
                });
        }

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

                    if (state->is_ready(std::memory_order_relaxed))
                    {
                        complete_receiver();
                        return;
                    }

                    state->execute_deferred();

                    if (!state->is_ready(std::memory_order_relaxed))
                    {
                        state->set_on_completed(
                            [this]() mutable { complete_receiver_noexcept(); });
                    }
                    else
                    {
                        complete_receiver();
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
    struct future_sender_with_scheduler
    {
        using sender_concept = hpx::execution::experimental::sender_t;
        using future_type = std::decay_t<Future>;
        using scheduler_type = std::decay_t<Scheduler>;
        using result_type =
            typename hpx::traits::future_traits<future_type>::type;

        future_type future_;
        HPX_NO_UNIQUE_ADDRESS scheduler_type scheduler_;

        struct env
        {
            scheduler_type scheduler_;

            template <typename CPO>
                requires(std::is_same_v<CPO,
                             hpx::execution::experimental::set_value_t> ||
                    std::is_same_v<CPO,
                        hpx::execution::experimental::set_error_t>)
            constexpr auto query(
                hpx::execution::experimental::get_completion_scheduler_t<CPO>)
                const noexcept
            {
                return scheduler_;
            }

            constexpr auto query(
                hpx::execution::experimental::get_domain_t) const noexcept
            {
                return hpx::execution::experimental::get_domain(scheduler_);
            }
        };

        using completion_signatures =
            typename future_sender_completion_signatures<result_type>::type;

        template <typename... Env>
        constexpr auto get_completion_signatures(Env&&...) const noexcept
            -> completion_signatures
        {
            return {};
        }

        constexpr env get_env() const noexcept
        {
            return {scheduler_};
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

}    // namespace hpx::execution::experimental::detail
