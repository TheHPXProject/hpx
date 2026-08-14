//  (C) Copyright 2005-7 Anthony Williams
//  (C) Copyright 2005 John Maddock
//  (C) Copyright 2011-2012 Vicente J. Botet Escriba
//  Copyright (c) 2013-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

/// \file once.hpp
/// \page hpx::once_flag, hpx::call_once
/// \headerfile hpx/mutex.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/synchronization/event.hpp>

#include <atomic>
#include <utility>

namespace hpx {

    /// \brief The class \c hpx::once_flag is a helper structure for \c hpx::call_once.
    ///        An object of type \c hpx::once_flag that is passed to multiple calls to
    ///        \c hpx::call_once allows those calls to coordinate with each other such
    ///        that only one of the calls will actually run to completion.
    ///        \c hpx::once_flag is neither copyable nor movable.
    HPX_CXX_CORE_EXPORT struct once_flag
    {
    public:
        once_flag(once_flag const&) = delete;
        once_flag(once_flag&&) = delete;
        once_flag& operator=(once_flag const&) = delete;
        once_flag& operator=(once_flag&&) = delete;

    public:
        /// \brief Constructs an \a once_flag object. The internal state is set to indicate
        ///        that no function has been called yet.
        once_flag() noexcept
          : status_(0)
        {
        }

    private:
        std::atomic<long> status_;
        lcos::local::event event_;

        template <typename F, typename... Args>
        friend void call_once(once_flag& flag, F&& f, Args&&... args);
    };

    ///////////////////////////////////////////////////////////////////////////
    /// \brief   Executes the Callable object \a f exactly once, even if called
    ///          concurrently, from several threads.
    /// \details In detail:
    ///          - If, by the time \a call_once is called, flag indicates that
    ///            \a f was already called, \a call_once returns right away
    ///            (such a call to \a call_once is known as passive).
    ///          - Otherwise, \a call_once invokes \c std::forward<Callable>(f)
    ///            with the arguments \c std::forward<Args>(args)... (as if by
    ///            \c hpx::invoke). Unlike the \c hpx::thread constructor or
    ///            \c hpx::async, the arguments are not moved or copied because
    ///            they don't need to be transferred to another thread of
    ///            execution. (such a call to \a call_once is known as active).
    ///             - If that invocation throws an exception, it is propagated
    ///               to the caller of \a call_once, and the flag is not flipped
    ///               so that another call will be attempted (such a call to \a
    ///               call_once is known as exceptional).
    ///             - If that invocation returns normally (such a call to \a
    ///               call_once is known as returning), the flag is flipped, and
    ///               all other calls to \a call_once with the same flag are
    ///               guaranteed to be passive.
    ///          All active calls on the same flag form a single total order
    ///          consisting of zero or more exceptional calls, followed by one
    ///          returning call. The end of each active call synchronizes-with
    ///          the next active call in that order. The return from the
    ///          returning call synchronizes-with the returns from all passive
    ///          calls on the same flag: this means that all concurrent calls to
    ///          \a call_once are guaranteed to observe any side-effects made by
    ///             the
    ///          active call, with no additional synchronization.
    ///
    /// \param flag    an object, for which exactly one function gets executed
    /// \param f       Callable object to invoke
    /// \param args    arguments to pass to the function
    ///
    /// \throws std::system_error if any condition prevents calls to \a
    ///         call_once from executing as specified or any exception thrown by
    ///         \a f
    ///
    /// \note If concurrent calls to \a call_once pass different functions \a f,
    ///       it is unspecified which f will be called. The selected function
    ///       runs in the same thread as the \a call_once invocation it was
    ///       passed to. Initialization of function-local statics is guaranteed
    ///       to occur only once even when called from multiple threads, and may
    ///       be more efficient than the equivalent code using \c
    ///       hpx::call_once. The POSIX equivalent of this function is \a
    ///       pthread_once.
    HPX_CXX_CORE_EXPORT template <typename F, typename... Args>
    void call_once(once_flag& flag, F&& f, Args&&... args)
    {
        // Try for a quick win: if the procedure has already been called
        // just skip through:
        constexpr long const function_complete_flag_value = 0xc15730e2;
        constexpr long const running_value = 0x7f0725e3;

        while (flag.status_.load(std::memory_order_acquire) !=
            function_complete_flag_value)
        {
            long status = 0;
            if (flag.status_.compare_exchange_strong(status, running_value))
            {
                try
                {
                    // reset event to ensure its usability in case the
                    // wrapped function was throwing an exception before
                    flag.event_.reset();

                    HPX_INVOKE(HPX_FORWARD(F, f), HPX_FORWARD(Args, args)...);

                    // set status to done, release waiting threads
                    flag.status_.store(function_complete_flag_value);
                    flag.event_.set();
                    break;
                }
                catch (...)
                {
                    // reset status to initial, release waiting threads
                    flag.status_.store(0);
                    flag.event_.set();

                    throw;
                }
            }

            // we're done if function was called
            if (status == function_complete_flag_value)
                break;

            // wait for the function finish executing
            flag.event_.wait();
        }
    }
}    // namespace hpx
