//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/execution_base.hpp>

#include <chrono>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <system_error>
#include <type_traits>
#include <utility>

namespace hpx::io::experimental {

    namespace detail {

        namespace ex = hpx::execution::experimental;

        HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT timer_operation
        {
        public:
            using callback_type = std::function<void(std::error_code const&)>;

            explicit timer_operation(::asio::io_context& context);
            ~timer_operation();

            timer_operation(timer_operation const&) = delete;
            timer_operation(timer_operation&&) = delete;
            timer_operation& operator=(timer_operation const&) = delete;
            timer_operation& operator=(timer_operation&&) = delete;

            void async_wait(std::chrono::steady_clock::time_point time_point,
                callback_type callback);
            [[nodiscard]] std::exception_ptr cancel() noexcept;

            [[nodiscard]] static bool is_operation_aborted(
                std::error_code const& error) noexcept;

        private:
            struct impl;
            std::unique_ptr<impl> impl_;
        };

        HPX_CXX_CORE_EXPORT template <typename Receiver, typename Expiry>
        struct timer_state
          : std::enable_shared_from_this<timer_state<Receiver, Expiry>>
        {
            using stop_token_type = ex::stop_token_of_t<ex::env_of_t<Receiver>>;

            struct on_stop
            {
                timer_state* state;

                void operator()() const noexcept
                {
                    state->request_stop();
                }
            };

            using stop_callback_type =
                ex::stop_callback_for_t<stop_token_type, on_stop>;

            template <typename Receiver_>
            timer_state(::asio::io_context& context, Expiry expiry,
                Receiver_&& receiver)
              : timer_(context)
              , expiry_(expiry)
              , receiver_(HPX_FORWARD(Receiver_, receiver))
            {
            }

            void start() noexcept
            {
                // Keep the state alive if a completion destroys the operation
                // state before this function returns.
                auto const state = this->shared_from_this();
                std::unique_lock lock(mutex_);

                try
                {
                    std::chrono::steady_clock::time_point time_point;
                    if constexpr (std::is_same_v<Expiry,
                                      std::chrono::steady_clock::time_point>)
                    {
                        time_point = expiry_;
                    }
                    else
                    {
                        time_point = std::chrono::steady_clock::now() + expiry_;
                    }

                    timer_.async_wait(
                        time_point, [state](std::error_code const& error) {
                            state->complete(error);
                        });

                    stop_callback_.emplace(
                        ex::get_stop_token(ex::get_env(receiver_)),
                        on_stop{this});
                }
                catch (...)
                {
                    completed_ = true;
                    lock.unlock();
                    ex::set_error(
                        HPX_MOVE(receiver_), std::current_exception());
                }
            }

            void request_stop() noexcept
            {
                std::lock_guard lock(mutex_);
                if (completed_)
                {
                    return;
                }

                std::exception_ptr error = timer_.cancel();
                if (error)
                {
                    cancellation_error_ = HPX_MOVE(error);
                }
            }

            void complete(std::error_code const& error) noexcept
            {
                std::unique_lock lock(mutex_);
                if (completed_)
                {
                    return;
                }

                completed_ = true;
                std::exception_ptr cancellation_error =
                    HPX_MOVE(cancellation_error_);
                lock.unlock();

                // A concurrently executing stop callback may be waiting for
                // mutex_. Destroying it while holding the mutex could
                // deadlock because stop-callback destruction waits for an
                // active callback to return.
                stop_callback_.reset();

                if (cancellation_error)
                {
                    ex::set_error(
                        HPX_MOVE(receiver_), HPX_MOVE(cancellation_error));
                }
                else if (!error)
                {
                    ex::set_value(HPX_MOVE(receiver_));
                }
                else if (timer_operation::is_operation_aborted(error))
                {
                    ex::set_stopped(HPX_MOVE(receiver_));
                }
                else
                {
                    ex::set_error(HPX_MOVE(receiver_), make_exception(error));
                }
            }

        private:
            static std::exception_ptr make_exception(
                std::error_code const& error) noexcept
            {
                try
                {
                    return std::make_exception_ptr(std::system_error(error));
                }
                catch (...)
                {
                    return std::current_exception();
                }
            }

            timer_operation timer_;
            Expiry expiry_;
            Receiver receiver_;
            std::recursive_mutex mutex_;
            std::optional<stop_callback_type> stop_callback_;
            std::exception_ptr cancellation_error_;
            bool completed_ = false;
        };

        HPX_CXX_CORE_EXPORT template <typename Receiver, typename Expiry>
        struct timer_operation_state
        {
            template <typename Receiver_>
            timer_operation_state(::asio::io_context& context, Expiry expiry,
                Receiver_&& receiver)
              : state_(std::make_shared<timer_state<Receiver, Expiry>>(
                    context, expiry, HPX_FORWARD(Receiver_, receiver)))
            {
            }

            timer_operation_state(timer_operation_state const&) = delete;
            timer_operation_state(timer_operation_state&&) = delete;
            timer_operation_state& operator=(
                timer_operation_state const&) = delete;
            timer_operation_state& operator=(timer_operation_state&&) = delete;

            void start() & noexcept
            {
                state_->start();
            }

        private:
            std::shared_ptr<timer_state<Receiver, Expiry>> state_;
        };

        HPX_CXX_CORE_EXPORT template <typename Expiry>
        struct timer_sender
        {
            using sender_concept = hpx::execution::experimental::sender_t;
            using completion_signatures =
                hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(),
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr),
                    hpx::execution::experimental::set_stopped_t()>;

            timer_sender(::asio::io_context& context, Expiry expiry) noexcept
              : context_(context)
              , expiry_(expiry)
            {
            }

            template <typename Receiver>
            auto connect(Receiver&& receiver) const
            {
                return timer_operation_state<std::decay_t<Receiver>, Expiry>(
                    context_.get(), expiry_, HPX_FORWARD(Receiver, receiver));
            }

        private:
            std::reference_wrapper<::asio::io_context> context_;
            Expiry expiry_;
        };
    }    // namespace detail

    /// Return a sender that completes after the supplied duration has elapsed.
    ///
    /// The timer starts when the sender's operation state is started. Successful
    /// expiry produces \c set_value(), a stop request produces \c set_stopped(),
    /// and other timer failures produce \c set_error(std::exception_ptr). The
    /// supplied I/O context must be run until the operation completes.
    ///
    /// \param context The Asio I/O context on which the timer is run.
    /// \param duration The duration to wait after the operation is started.
    ///
    /// \returns A sender representing the timer operation.
    HPX_CXX_CORE_EXPORT template <typename Rep, typename Period>
    [[nodiscard]] auto schedule_after(::asio::io_context& context,
        std::chrono::duration<Rep, Period> const duration) noexcept
    {
        return detail::timer_sender<std::chrono::steady_clock::duration>(
            context,
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                duration));
    }

    /// Return a sender that completes at the supplied time point.
    ///
    /// Successful expiry produces \c set_value(), a stop request produces
    /// \c set_stopped(), and other timer failures produce
    /// \c set_error(std::exception_ptr). The supplied I/O context must be run
    /// until the operation completes.
    ///
    /// \param context The Asio I/O context on which the timer is run.
    /// \param time_point The absolute time at which the sender should complete.
    ///
    /// \returns A sender representing the timer operation.
    HPX_CXX_CORE_EXPORT [[nodiscard]] inline auto schedule_at(
        ::asio::io_context& context,
        std::chrono::steady_clock::time_point const time_point) noexcept
    {
        return detail::timer_sender<std::chrono::steady_clock::time_point>(
            context, time_point);
    }
}    // namespace hpx::io::experimental
