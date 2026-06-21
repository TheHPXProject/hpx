//  Copyright (c) 2025 Shivansh Singh
//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file future_sender.hpp
/// \brief Adapts an hpx::future<T> as a P2300 stdexec sender.
///
/// Usage:
/// \code
///   hpx::future<int> f = hpx::async([]{ return 42; });
///   auto result = hpx::execution::experimental::as_sender(std::move(f))
///       | stdexec::then([](int x){ return x * 2; })
///       | stdexec::sync_wait();
/// \endcode

#pragma once

#include <hpx/config.hpp>
#include <hpx/futures/future.hpp>
#include <hpx/modules/execution_base.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // Operation state produced by connecting a future_sender to a receiver.
    //
    // The operation state captures the future and receiver, then registers a
    // continuation via future.then() when start() is invoked.  The future's
    // internal shared state keeps the operation alive until the continuation
    // fires.
    //
    // Design notes:
    //  - Not copyable: futures are move-only and the operation state must be
    //    pinned in memory for the duration of the async operation.
    //  - start() is noexcept: any exception thrown registering the continuation
    //    is caught and routed to set_error.
    //  - The receiver is captured by value; it is only moved into the
    //    continuation *inside* the lambda body (never moved into the capture)
    //    to avoid move-after-capture UB if future.then() itself throws.
    HPX_CXX_CORE_EXPORT template <typename Receiver, typename T>
    class future_sender_operation_state
    {
    public:
        using receiver_type = std::decay_t<Receiver>;

        template <typename Receiver_>
        future_sender_operation_state(Receiver_&& r, hpx::future<T> f)
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

#if !defined(__NVCC__) && !defined(__CUDACC__)
        ~future_sender_operation_state() = default;
#endif

        void start() & noexcept
        {
            start_helper();
        }

    private:
        // STANDARD 3: capture &os, move receiver inside the lambda body to
        // prevent UB if future_.then() throws before the continuation fires.
        void start_helper() & noexcept
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    // Register a continuation that fires when the future
                    // becomes ready.  We capture this by reference because
                    // the operation state is pinned in memory and outlives the
                    // continuation (P2300 §6.9.7 lifetime guarantee).
                    future_.then([this](hpx::future<T> f) {
                        hpx::detail::try_catch_exception_ptr(
                            [&]() {
                                if constexpr (std::is_void_v<T>)
                                {
                                    f.get();    // rethrow any stored exception
                                    hpx::execution::experimental::set_value(
                                        HPX_MOVE(receiver_));
                                }
                                else
                                {
                                    hpx::execution::experimental::set_value(
                                        HPX_MOVE(receiver_), f.get());
                                }
                            },
                            [&](std::exception_ptr ep) {
                                hpx::execution::experimental::set_error(
                                    HPX_MOVE(receiver_), HPX_MOVE(ep));
                            });
                    });
                },
                [&](std::exception_ptr ep) {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(receiver_), HPX_MOVE(ep));
                });
        }

        receiver_type receiver_;
        hpx::future<T> future_;
    };

    // Specialisation for future<void>
    HPX_CXX_CORE_EXPORT template <typename Receiver>
    class future_sender_operation_state<Receiver, void>
    {
    public:
        using receiver_type = std::decay_t<Receiver>;

        template <typename Receiver_>
        future_sender_operation_state(Receiver_&& r, hpx::future<void> f)
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

#if !defined(__NVCC__) && !defined(__CUDACC__)
        ~future_sender_operation_state() = default;
#endif

        void start() & noexcept
        {
            start_helper();
        }

    private:
        void start_helper() & noexcept
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    future_.then([this](hpx::future<void> f) {
                        hpx::detail::try_catch_exception_ptr(
                            [&]() {
                                f.get();
                                hpx::execution::experimental::set_value(
                                    HPX_MOVE(receiver_));
                            },
                            [&](std::exception_ptr ep) {
                                hpx::execution::experimental::set_error(
                                    HPX_MOVE(receiver_), HPX_MOVE(ep));
                            });
                    });
                },
                [&](std::exception_ptr ep) {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(receiver_), HPX_MOVE(ep));
                });
        }

        receiver_type receiver_;
        hpx::future<void> future_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // future_sender<T>: a P2300-compliant sender that wraps an hpx::future<T>.
    //
    // Completion signatures:
    //   set_value(T)              — future produced a value
    //   set_error(exception_ptr)  — future held an exception
    //   set_stopped()             — not produced; included for concept conformance
    //
    // Note: set_stopped() is advertised but never signalled; a cancelled future
    // delivers its cancellation as an exception (hpx::thread_interrupted) via
    // the set_error channel, matching the behaviour of as_sender.
    HPX_CXX_CORE_EXPORT template <typename T>
    struct future_sender
    {
        // STANDARD 8: must define sender_concept
        using sender_concept = hpx::execution::experimental::sender_t;

        // Completion signatures advertised to stdexec concept machinery.
        using completion_signatures =
            hpx::execution::experimental::completion_signatures<
                hpx::execution::experimental::set_value_t(T),
                hpx::execution::experimental::set_error_t(std::exception_ptr),
                hpx::execution::experimental::set_stopped_t()>;

        explicit future_sender(hpx::future<T>&& f) noexcept
          : future_(HPX_MOVE(f))
        {
        }

        // Move-only: futures cannot be copied
        future_sender(future_sender&&) = default;
        future_sender& operator=(future_sender&&) = default;
        future_sender(future_sender const&) = delete;
        future_sender& operator=(future_sender const&) = delete;

#if !defined(__NVCC__) && !defined(__CUDACC__)
        ~future_sender() = default;
#endif

        // get_completion_signatures tag_invoke (STANDARD 2: tag_invoke pattern)
        template <typename Env>
        friend auto tag_invoke(
            hpx::execution::experimental::get_completion_signatures_t,
            future_sender const&, Env&&) -> completion_signatures;

        // connect tag_invoke — returns the operation state (STANDARD 2)
        template <typename Receiver>
        friend auto tag_invoke(hpx::execution::experimental::connect_t,
            future_sender&& self, Receiver&& r)
        {
            return future_sender_operation_state<Receiver, T>{
                HPX_FORWARD(Receiver, r), HPX_MOVE(self.future_)};
        }

    private:
        hpx::future<T> future_;
    };

    // Specialisation for future<void>
    HPX_CXX_CORE_EXPORT template <>
    struct future_sender<void>
    {
        using sender_concept = hpx::execution::experimental::sender_t;

        using completion_signatures =
            hpx::execution::experimental::completion_signatures<
                hpx::execution::experimental::set_value_t(),
                hpx::execution::experimental::set_error_t(std::exception_ptr),
                hpx::execution::experimental::set_stopped_t()>;

        explicit future_sender(hpx::future<void>&& f) noexcept
          : future_(HPX_MOVE(f))
        {
        }

        future_sender(future_sender&&) = default;
        future_sender& operator=(future_sender&&) = default;
        future_sender(future_sender const&) = delete;
        future_sender& operator=(future_sender const&) = delete;

#if !defined(__NVCC__) && !defined(__CUDACC__)
        ~future_sender() = default;
#endif

        template <typename Env>
        friend auto tag_invoke(
            hpx::execution::experimental::get_completion_signatures_t,
            future_sender const&, Env&&) -> completion_signatures;

        template <typename Receiver>
        friend auto tag_invoke(hpx::execution::experimental::connect_t,
            future_sender&& self, Receiver&& r)
        {
            return future_sender_operation_state<Receiver, void>{
                HPX_FORWARD(Receiver, r), HPX_MOVE(self.future_)};
        }

    private:
        hpx::future<void> future_;
    };

}    // namespace hpx::execution::experimental
