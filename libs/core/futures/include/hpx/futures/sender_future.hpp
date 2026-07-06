//  Copyright (c) 2025 Shivansh Singh
//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file sender_future.hpp
/// \brief Converts any P2300 stdexec sender into an hpx::future<T>.
///
/// Usage:
/// \code
///   // Explicit T:
///   auto sender = stdexec::just(42) | stdexec::then([](int x){ return x; });
///   hpx::future<int> f =
///       hpx::execution::experimental::as_future<int>(std::move(sender));
///   int val = f.get();   // == 42
/// \endcode

#pragma once

#include <hpx/config.hpp>
// clang-format off
#include <hpx/futures/promise.hpp>
#include <hpx/futures/future.hpp>
// clang-format on
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // internal receiver that bridges a P2300 sender into an hpx::promise.
        //
        // When the connected sender completes:
        //  - set_value   -> fulfils the promise with the value
        //  - set_error   -> stores the exception in the promise
        //  - set_stopped -> stores hpx::thread_interrupted in the promise
        //
        // The promise is shared via shared_ptr so both the receiver and the
        // returned future can outlive each other safely.  No raw new/delete.
        HPX_CXX_CORE_EXPORT template <typename T>
        struct sender_future_receiver
        {
            // STANDARD 8: every receiver must define receiver_concept
            using receiver_concept = hpx::execution::experimental::receiver_t;

            explicit sender_future_receiver(
                std::shared_ptr<hpx::promise<T>> p) noexcept
              : promise_(HPX_MOVE(p))
            {
            }

            sender_future_receiver(sender_future_receiver&&) = default;
            sender_future_receiver& operator=(
                sender_future_receiver&&) = default;
            sender_future_receiver(sender_future_receiver const&) = default;
            sender_future_receiver& operator=(
                sender_future_receiver const&) = default;

#if !defined(__NVCC__) && !defined(__CUDACC__)
            ~sender_future_receiver() = default;
#endif

            // set_value: forward the value into the promise (STANDARD 3: no
            // move-into-capture UB; capture lambda captures ref to promise_)
            template <typename U>
            void set_value(U&& val) && noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() { promise_->set_value(HPX_FORWARD(U, val)); },
                    [&](std::exception_ptr ep) {
                        promise_->set_exception(HPX_MOVE(ep));
                    });
            }

            // set_error: store the exception in the promise
            void set_error(std::exception_ptr ep) && noexcept
            {
                promise_->set_exception(HPX_MOVE(ep));
            }

            // set_stopped: convert stopped signal to hpx::thread_interrupted
            void set_stopped() && noexcept
            {
                promise_->set_exception(
                    std::make_exception_ptr(hpx::thread_interrupted{}));
            }

            // get_env: return an empty environment (no scheduler affinity)
            hpx::execution::experimental::empty_env get_env() const noexcept
            {
                return {};
            }

        private:
            std::shared_ptr<hpx::promise<T>> promise_;
        };

        // Specialisation for void-valued senders
        HPX_CXX_CORE_EXPORT template <>
        struct sender_future_receiver<void>
        {
            using receiver_concept = hpx::execution::experimental::receiver_t;

            explicit sender_future_receiver(
                std::shared_ptr<hpx::promise<void>> p) noexcept
              : promise_(HPX_MOVE(p))
            {
            }

            sender_future_receiver(sender_future_receiver&&) = default;
            sender_future_receiver& operator=(
                sender_future_receiver&&) = default;
            sender_future_receiver(sender_future_receiver const&) = default;
            sender_future_receiver& operator=(
                sender_future_receiver const&) = default;

#if !defined(__NVCC__) && !defined(__CUDACC__)
            ~sender_future_receiver() = default;
#endif

            void set_value() && noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() { promise_->set_value(); },
                    [&](std::exception_ptr ep) {
                        promise_->set_exception(HPX_MOVE(ep));
                    });
            }

            void set_error(std::exception_ptr ep) && noexcept
            {
                promise_->set_exception(HPX_MOVE(ep));
            }

            void set_stopped() && noexcept
            {
                promise_->set_exception(
                    std::make_exception_ptr(hpx::thread_interrupted{}));
            }

            hpx::execution::experimental::empty_env get_env() const noexcept
            {
                return {};
            }

        private:
            std::shared_ptr<hpx::promise<void>> promise_;
        };

        ///////////////////////////////////////////////////////////////////////
        // Heap-allocated wrapper that keeps the operation state alive until the
        // sender signals completion.  Constructs the op_state in-place via
        // guaranteed copy elision (C++17) since P2300 operation states are
        // immovable.
        template <typename Sender, typename Receiver>
        struct op_state_holder
        {
            template <typename Sender_>
            op_state_holder(Sender_&& s, Receiver r)
              : op_state_(hpx::execution::experimental::connect(
                    HPX_FORWARD(Sender_, s), HPX_MOVE(r)))
            {
            }

            op_state_holder(op_state_holder const&) = delete;
            op_state_holder& operator=(op_state_holder const&) = delete;
            op_state_holder(op_state_holder&&) = delete;
            op_state_holder& operator=(op_state_holder&&) = delete;

#if !defined(__NVCC__) && !defined(__CUDACC__)
            ~op_state_holder() = default;
#endif

            void start() noexcept
            {
                hpx::execution::experimental::start(op_state_);
            }

        private:
            using op_state_type =
                decltype(hpx::execution::experimental::connect(
                    std::declval<Sender>(), std::declval<Receiver>()));
            op_state_type op_state_;
        };

    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // as_future<T>: converts a P2300 sender into an hpx::future<T>.
    //
    // T must be specified explicitly because the futures module cannot pull in
    // the execution module's single_result/value_types_of_t utilities without
    // creating a circular dependency (STANDARD 4).
    //
    // Implementation:
    //   1. Creates a shared hpx::promise<T> and retrieves a future from it.
    //   2. Connects the sender to a sender_future_receiver<T>.
    //   3. Heap-allocates the operation state inside a shared_ptr to keep it
    //      pinned in memory until the sender completes.
    //   4. Calls start() on the operation state (noexcept per P2300 6.9.7).
    //   5. Attaches a continuation that releases the operation state after the
    //      future resolves.
    //
    // No raw new/delete: all storage is managed by shared_ptr.
    //
    // STANDARD 4: This header does NOT include executor_scheduler.hpp,
    // parallel_executor.hpp, or any execution algorithm detail headers.
    template <typename T, typename Sender,
        typename = std::enable_if_t<
            hpx::execution::experimental::is_sender_v<std::decay_t<Sender>>>>
    hpx::future<T> as_future(Sender&& sender)
    {
        using receiver_type = detail::sender_future_receiver<T>;
        using holder_type = detail::op_state_holder<Sender, receiver_type>;

        // Build promise + future pair
        auto promise_ptr = std::make_shared<hpx::promise<T>>();
        hpx::future<T> fut = promise_ptr->get_future();

        // Connect sender to receiver; heap-allocate to keep op_state pinned.
        // The op_state is constructed in-place inside the holder.
        auto holder_ptr = std::make_shared<holder_type>(
            HPX_FORWARD(Sender, sender), receiver_type{promise_ptr});

        // start() is noexcept per P2300 6.9.7
        holder_ptr->start();

        // Keep holder alive through the future's continuation; the holder
        // destructs only after the future value has been read.
        return fut.then([holder_ptr](hpx::future<T> f) mutable -> T {
            holder_ptr.reset();
            return f.get();
        });
    }

}    // namespace hpx::execution::experimental
