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
            void async_wait(std::chrono::steady_clock::duration duration,
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
                    stop_token_type stop_token =
                        ex::get_stop_token(ex::get_env(receiver_));
                    if (stop_token.stop_requested())
                    {
                        completed_ = true;
                        lock.unlock();
                        ex::set_stopped(HPX_MOVE(receiver_));
                        return;
                    }

                    timer_.async_wait(
                        expiry_, [state](std::error_code const& error) {
                            state->complete(error);
                        });

                    if constexpr (!ex::unstoppable_token<stop_token_type>)
                    {
                        stop_callback_.emplace(
                            HPX_MOVE(stop_token), on_stop{this});
                    }
                }
                catch (...)
                {
                    std::exception_ptr error = std::current_exception();
                    completed_ = true;
                    static_cast<void>(timer_.cancel());
                    lock.unlock();
                    ex::set_error(HPX_MOVE(receiver_), HPX_MOVE(error));
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

            // Registering a callback for an already stopped token may invoke
            // on_stop synchronously while start() holds this lock. The Asio
            // completion handler runs on an ordinary I/O thread, where
            // hpx::recursive_mutex cannot be used because no HPX execution
            // agent is installed.
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

        HPX_CXX_CORE_EXPORT template <typename Rep, typename Period>
        [[nodiscard]] std::chrono::steady_clock::duration
        to_steady_clock_duration(
            std::chrono::duration<Rep, Period> const duration) noexcept
        {
            using duration_type = std::chrono::steady_clock::duration;
            using floating_duration_type =
                std::chrono::duration<long double, duration_type::period>;

            floating_duration_type const converted =
                std::chrono::duration_cast<floating_duration_type>(duration);
            floating_duration_type const maximum((duration_type::max) ());
            floating_duration_type const minimum((duration_type::min) ());

            if (converted >= maximum)
            {
                return (duration_type::max) ();
            }
            if (converted <= minimum)
            {
                return (duration_type::min) ();
            }
            return duration_type(
                static_cast<duration_type::rep>(converted.count()));
        }
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
    [[nodiscard]] auto schedule_timer_after(::asio::io_context& context,
        std::chrono::duration<Rep, Period> const duration) noexcept
    {
        return detail::timer_sender<std::chrono::steady_clock::duration>(
            context, detail::to_steady_clock_duration(duration));
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
    HPX_CXX_CORE_EXPORT [[nodiscard]] inline auto schedule_timer_at(
        ::asio::io_context& context,
        std::chrono::steady_clock::time_point const time_point) noexcept
    {
        return detail::timer_sender<std::chrono::steady_clock::time_point>(
            context, time_point);
    }
}    // namespace hpx::io::experimental
