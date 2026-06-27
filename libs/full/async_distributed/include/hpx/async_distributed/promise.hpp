//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2016      Thomas Heller
//  Copyright (c) 2011      Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/detail/promise_base.hpp>

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::distributed {

    ///////////////////////////////////////////////////////////////////////////
    /// A promise can be used by a single \a thread to invoke a
    /// (remote) action and wait for the result. The result is expected to be
    /// sent back to the promise using the LCO's set_event action
    ///
    /// A promise is one of the simplest synchronization primitives
    /// provided by HPX. It allows to synchronize on a eager evaluated remote
    /// operation returning a result of the type \a Result. The \a promise
    /// allows to synchronize exactly one \a thread (the one passed during
    /// construction time).
    ///
    /// \code
    ///     // Create the promise (the expected result is a id_type)
    ///     hpx::distributed::promise<hpx::id_type> p;
    ///
    ///     // Get the associated future
    ///     future<hpx::id_type> f = p.get_future();
    ///
    ///     // initiate the action supplying the promise as a
    ///     // continuation
    ///     apply<some_action>(new continuation(p.get_id()), ...);
    ///
    ///     // Wait for the result to be returned, yielding control
    ///     // in the meantime.
    ///     hpx::id_type result = f.get();
    ///     // ...
    /// \endcode
    ///
    /// \tparam Result   The template parameter \a Result defines the type this
    ///                  promise is expected to return from
    ///                  \a promise#get.
    /// \tparam RemoteResult The template parameter \a RemoteResult defines the
    ///                  type this promise is expected to receive
    ///                  from the remote action.
    ///
    /// \note            The action executed by the promise must return a value
    ///                  of a type convertible to the type as specified by the
    ///                  template parameter \a RemoteResult
    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Result, typename RemoteResult>
    class promise
      : public lcos::detail::promise_base<Result, RemoteResult,
            lcos::detail::promise_data<Result>>
    {
        using base_type = lcos::detail::promise_base<Result, RemoteResult,
            lcos::detail::promise_data<Result>>;

    public:
        /// \brief constructs a promise object and a shared state.
        promise() = default;

        /// \brief    constructs a promise object and a shared state. The
        ///           constructor uses the allocator \a to allocate the memory
        ///           for the shared state.
        template <typename Allocator>
        promise(std::allocator_arg_t, Allocator const& a)
          : base_type(std::allocator_arg, a)
        {
        }

        /// \brief   constructs a new promise object and transfers ownership of
        ///          the shared state of other (if any) to the newly-
        ///          constructed object.
        /// \post    other has no shared state.
        promise(promise&& other) noexcept = default;

        /// \brief Abandons any shared state
        ~promise() = default;

        /// \brief   Abandons any shared state (30.6.4) and then as if
        ///          promise(HPX_MOVE(other)).swap(*this).
        /// \returns *this.
        promise& operator=(promise&& other) noexcept = default;

        /// \brief   Exchanges the shared state of *this and other.
        /// \post    *this has the shared state (if any) that other had
        ///          prior to the call to swap. other has the shared state
        ///          (if any) that *this had prior to the call to swap.
        void swap(promise& other) noexcept
        {
            base_type::swap(other);
        }

        /// \returns  true only if *this refers to a shared state.
        using base_type::valid;

        /// \returns   A future<Result> object with the same shared state as *this.
        /// \throws    future_error if *this has no shared state or if get_future
        ///            or get_shared_future has already been called on a promise
        ///            with the same shared state as *this.
        ///            future_already_retrieved if get_future or get_shared_future
        ///            has already been called on a promise with the same shared
        ///            state as *this.
        ///            no_state if *this has no shared state.
        using base_type::get_future;

        /// \returns    A shared_future<Result> object with the same shared state
        ///             as *this.
        /// \throws     future_error if *this has no shared state or if
        ///             get_shared_future has already been called on a promise
        ///             with the same shared state as *this.
        ///             future_already_retrieved if get_shared_future has already
        ///             been called on a promise with the same shared state as
        ///             *this.
        ///             no_state if *this has no shared state.
        using base_type::get_shared_future;

        /// \brief       Effects atomically stores the value r in the shared state
        ///              and makes that state ready (30.6.4).
        /// \throws      future_error if its shared state already has a stored value
        ///              if shared state has no stored value exception is raised.
        ///              promise_already_satisfied if its shared state already has a
        ///              stored value or exception.
        ///              no_state if *this has no shared state.
        using base_type::set_value;

        /// \brief       Effects atomically stores the exception pointer p in the
        ///              shared state and makes that state ready (30.6.4).
        /// \throws      future_error if its shared state already has a stored value
        ///              if shared state has no stored value exception is raised.
        ///              promise_already_satisfied if its shared state already has a
        ///              stored value or exception.
        ///              no_state if *this has no shared state.
        using base_type::set_exception;
    };

    template <>
    class promise<void, hpx::util::unused_type>
      : public lcos::detail::promise_base<void, hpx::util::unused_type,
            lcos::detail::promise_data<void>>
    {
        using base_type = lcos::detail::promise_base<void,
            hpx::util::unused_type, lcos::detail::promise_data<void>>;

    public:
        /// \brief constructs a promise object and a shared state.
        promise() = default;

        /// \brief     constructs a promise object and a shared state. The
        ///            constructor uses the allocator \a to allocate the memory
        ///            for the shared state.
        template <typename Allocator>
        promise(std::allocator_arg_t, Allocator const& a)
          : base_type(std::allocator_arg, a)
        {
        }

        /// \brief   constructs a new promise object and transfers ownership of
        ///          the shared state of other (if any) to the newly-
        ///          constructed object.
        /// \post    other has no shared state.
        promise(promise&& other) noexcept = default;

        /// \brief   Abandons any shared state
        ~promise() = default;

        /// \brief   Abandons any shared state (30.6.4) and then as if
        ///          promise(HPX_MOVE(other)).swap(*this).
        /// \returns *this.
        promise& operator=(promise&& other) noexcept = default;

        /// \brief         Exchanges the shared state of *this and other.
        /// \post          *this has the shared state (if any) that other had
        ///                prior to the call to swap. other has the shared state
        ///                (if any) that *this had prior to the call to swap.
        void swap(promise& other) noexcept
        {
            base_type::swap(other);
        }

        /// \returns    true only if *this refers to a shared state.
        using base_type::valid;

        /// \returns    A future<Result> object with the same shared state as *this.
        /// \throws     future_error if *this has no shared state or if get_future
        ///             or get_shared_future has already been called on a promise
        ///             with the same shared state as *this.
        ///             future_already_retrieved if get_future or get_shared_future
        ///             has already been called on a promise with the same shared
        ///             state as *this.
        ///             no_state if *this has no shared state.
        using base_type::get_future;

        /// \returns   A shared_future<Result> object with the same shared state
        ///            as *this.
        /// \throws    future_error if *this has no shared state or if
        ///            get_shared_future has already been called on a promise
        ///            with the same shared state as *this.
        ///            future_already_retrieved if get_shared_future has already
        ///            been called on a promise with the same shared state as
        ///            *this.
        ///            no_state if *this has no shared state.
        using base_type::get_shared_future;

        /// \brief     atomically stores the value r in the shared state and makes
        ///            that state ready (30.6.4).
        ///
        /// \throws    future_error if its shared state already has a stored value.
        ///            if shared state has no stored value exception is raised.
        ///            promise_already_satisfied if its shared state already
        ///            has a stored value or exception.
        ///            no_state if *this has no shared state.
        void set_value()
        {
            base_type::set_value(hpx::util::unused);
        }

        /// \brief     atomically stores the exception pointer p in the shared
        ///            state and makes that state ready (30.6.4).
        /// \throws    future_error if its shared state already has a stored value.
        ///            if shared state has no stored value exception is raised.
        ///            promise_already_satisfied if its shared state already has a
        ///            stored value or exception.
        ///            no_state if *this has no shared state.
        using base_type::set_exception;
    };

    HPX_CXX_EXPORT template <typename Result, typename RemoteResult>
    void swap(promise<Result, RemoteResult>& x,
        promise<Result, RemoteResult>& y) noexcept
    {
        x.swap(y);
    }
}    // namespace hpx::distributed

namespace std {

    /// Requires: Allocator shall be an allocator (17.6.3.5)
    template <typename R, typename Allocator>
    struct uses_allocator<hpx::distributed::promise<R>, Allocator>
      : std::true_type
    {
    };
}    // namespace std
