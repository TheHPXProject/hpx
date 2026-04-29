//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
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

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {
    namespace detail {

        ///////////////////////////////////////////////////////////////////////////
        // Operation state for sender compatibility
        HPX_CXX_CORE_EXPORT template <typename Receiver, typename Future>
        class as_sender_operation_state
        {
        private:
            using receiver_type = std::decay_t<Receiver>;
            using future_type = std::decay_t<Future>;
            using result_type = typename future_type::result_type;

        public:
            template <typename Receiver_>
            as_sender_operation_state(Receiver_&& r, future_type f)
              : receiver_(HPX_FORWARD(Receiver_, r))
              , future_(HPX_MOVE(f))
            {
            }

            as_sender_operation_state(as_sender_operation_state&&) = delete;
            as_sender_operation_state& operator=(
                as_sender_operation_state&&) = delete;
            as_sender_operation_state(
                as_sender_operation_state const&) = delete;
            as_sender_operation_state& operator=(
                as_sender_operation_state const&) = delete;

            void start() & noexcept
            {
                start_helper();
            }

        private:
            void start_helper() & noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        auto state = traits::detail::get_shared_state(future_);

                        if (!state)
                        {
                            HPX_THROW_EXCEPTION(hpx::error::no_state,
                                "as_sender_operation_state::start",
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

                            // execute_deferred might have made the future ready
                            if (!state->is_ready(std::memory_order_relaxed))
                            {
                                // The operation state has to be kept alive until
                                // set_value is called, which means that we don't
                                // need to move receiver and future into the
                                // on_completed callback.
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

            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver_;
            future_type future_;
        };

        HPX_CXX_CORE_EXPORT template <typename Future>
        struct as_sender_sender_base
        {
            using result_type = typename std::decay_t<Future>::result_type;

            std::decay_t<Future> future_;

            template <bool IsVoid, typename _result_type>
            struct set_value_void_checked
            {
                using type = hpx::execution::experimental::set_value_t(
                    _result_type);
            };

            template <typename _result_type>
            struct set_value_void_checked<true, _result_type>
            {
                using type = hpx::execution::experimental::set_value_t();
            };

            using completion_signatures =
                hpx::execution::experimental::completion_signatures<
                    typename set_value_void_checked<std::is_void_v<result_type>,
                        result_type>::type,
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr)>;
        };

        HPX_CXX_CORE_EXPORT template <typename Future>
        struct as_sender_sender;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct as_sender_sender<hpx::future<T>>
          : public as_sender_sender_base<hpx::future<T>>
        {
            using sender_concept = hpx::execution::experimental::sender_t;
            using future_type = hpx::future<T>;
            using base_type = as_sender_sender_base<hpx::future<T>>;
            using base_type::future_;

            template <typename Future,
                typename = std::enable_if_t<
                    !std::is_same_v<std::decay_t<Future>, as_sender_sender>>>
            explicit as_sender_sender(Future&& future)
              : base_type{HPX_FORWARD(Future, future)}
            {
            }

            as_sender_sender(as_sender_sender&&) = default;
            as_sender_sender& operator=(as_sender_sender&&) = default;
            as_sender_sender(as_sender_sender const&) = delete;
            as_sender_sender& operator=(as_sender_sender const&) = delete;

            template <typename Env>
            friend auto tag_invoke(
                get_completion_signatures_t, as_sender_sender const&, Env&&) ->
                typename base_type::completion_signatures;

            template <typename Receiver>
            auto connect(Receiver&& receiver) &&
            {
                return as_sender_operation_state<Receiver, future_type>{
                    HPX_FORWARD(Receiver, receiver), HPX_MOVE(future_)};
            }
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        struct as_sender_sender<hpx::shared_future<T>>
          : as_sender_sender_base<hpx::shared_future<T>>
        {
            using sender_concept = hpx::execution::experimental::sender_t;
            using future_type = hpx::shared_future<T>;
            using base_type = as_sender_sender_base<hpx::shared_future<T>>;
            using base_type::future_;

            template <typename Future,
                typename = std::enable_if_t<
                    !std::is_same_v<std::decay_t<Future>, as_sender_sender>>>
            explicit as_sender_sender(Future&& future)
              : base_type{HPX_FORWARD(Future, future)}
            {
            }

            as_sender_sender(as_sender_sender&&) = default;
            as_sender_sender& operator=(as_sender_sender&&) = default;
            as_sender_sender(as_sender_sender const&) = default;
            as_sender_sender& operator=(as_sender_sender const&) = default;

            template <typename Env>
            friend auto tag_invoke(
                get_completion_signatures_t, as_sender_sender const&, Env&&) ->
                typename base_type::completion_signatures;

            template <typename Receiver>
            auto connect(Receiver&& receiver) &&
            {
                return as_sender_operation_state<Receiver, future_type>{
                    HPX_FORWARD(Receiver, receiver), HPX_MOVE(future_)};
            }

            template <typename Receiver>
            auto connect(Receiver&& receiver) &
            {
                return as_sender_operation_state<Receiver, future_type>{
                    HPX_FORWARD(Receiver, receiver), future_};
            }
        };
    }    // namespace detail

    // The as_sender CPO can be used to adapt any HPX future as a sender. The
    // value provided by the future will be used to call set_value on the
    // connected receiver once the future has become ready. If the future is
    // exceptional, set_error will be invoked on the connected receiver.
    //
    // The difference to keep_future is that as_future propagates the value
    // stored in the future while keep_future will propagate the future instance
    // itself.
    HPX_CXX_CORE_EXPORT inline constexpr struct as_sender_t final
    {
        // clang-format off
        template <typename Future,
            HPX_CONCEPT_REQUIRES_(
                hpx::traits::is_future_v<std::decay_t<Future>>
            )>
        // clang-format on
        constexpr HPX_FORCEINLINE auto operator()(Future&& future) const
        {
            return detail::as_sender_sender<std::decay_t<Future>>(
                HPX_FORWARD(Future, future));
        }

        constexpr HPX_FORCEINLINE auto operator()() const
        {
            return detail::partial_algorithm<as_sender_t>{};
        }
    } as_sender{};
}    // namespace hpx::execution::experimental
