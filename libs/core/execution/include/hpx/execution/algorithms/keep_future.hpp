//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/execution/algorithms/detail/partial_algorithm.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Receiver, typename Future>
        struct operation_state
        {
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;
            std::decay_t<Future> future;

            void start() & noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
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

        HPX_CXX_CORE_EXPORT template <typename Future>
        struct keep_future_sender_base
        {
            std::decay_t<Future> future;
            using completion_signatures =
                hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(
                        std::decay_t<Future>),
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr)>;
        };

        HPX_CXX_CORE_EXPORT template <typename Future>
        struct keep_future_sender;

        HPX_CXX_CORE_EXPORT template <typename T>
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

            template <typename Env>
            friend auto tag_invoke(
                get_completion_signatures_t, keep_future_sender const&, Env&&)
                -> typename base_type::completion_signatures;

            template <typename Receiver>
            operation_state<Receiver, future_type> connect(
                Receiver&& receiver) &&
            {
                return {HPX_FORWARD(Receiver, receiver), HPX_MOVE(future)};
            }
        };

        HPX_CXX_CORE_EXPORT template <typename T>
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

            template <typename Env>
            friend auto tag_invoke(
                get_completion_signatures_t, keep_future_sender const&, Env&&)
                -> typename base_type::completion_signatures;

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
