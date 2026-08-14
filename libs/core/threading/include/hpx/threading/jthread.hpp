//  Copyright (c) 2020-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file jthread.hpp
/// \page hpx::jthread
/// \headerfile hpx/jthread.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/threading/thread.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    /// The class \a jthread represents a single thread of execution. It has the
    /// same general behavior as \a hpx::thread, except that \a jthread
    /// automatically rejoins on destruction, and can be cancelled/stopped in
    /// certain situations. Threads begin execution immediately upon
    /// construction of the associated thread object (pending any OS scheduling
    /// delays), starting at the top-level function provided as a constructor
    /// argument. The return value of the top-level function is ignored and if
    /// it terminates by throwing an exception, \a hpx::terminate is called. The
    /// top-level function may communicate its return value or an exception to
    /// the caller via \a hpx::promise or by modifying shared variables (which
    /// may require synchronization, see \a hpx::mutex and \a hpx::atomic)
    /// Unlike \a hpx::thread, the jthread logically holds an internal private
    /// member of type \a hpx::stop_source, which maintains a shared stop-state.
    /// The \a jthread constructor accepts a function that takes a \a
    /// hpx::stop_token as its first argument, which will be passed in by the
    /// jthread from its internal \a stop_source. This allows the function to
    /// check if stop has been requested during its execution, and return if it
    /// has.
    /// \a hpx::jthread objects may also be in the state that does not
    /// represent any thread (after default construction, move from, detach, or
    /// join), and a thread of execution may be not associated with any \a
    /// jthread objects (after detach). No two \a hpx::jthread objects may
    /// represent the same thread of execution; \a hpx::jthread is not \a
    /// CopyConstructible or \a CopyAssignable, although it is \a
    /// MoveConstructible and \a MoveAssignable.
    HPX_CXX_CORE_EXPORT class jthread
    {
    private:
        template <typename F, typename... Ts>
        static void invoke(
            std::false_type, F&& f, stop_token&& /* st */, Ts&&... ts)
        {
            // started thread does not expect a stop token:
            HPX_INVOKE(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        static void invoke(std::true_type, F&& f, stop_token&& st, Ts&&... ts)
        {
            // pass the stop_token as first argument to the started thread:
            HPX_INVOKE(HPX_FORWARD(F, f), HPX_MOVE(st), HPX_FORWARD(Ts, ts)...);
        }

    public:
        // types
        using id = thread::id;
        using native_handle_type = thread::native_handle_type;

        // 32.4.3.1, constructors, move, and assignment

        // Effects: Constructs a jthread object that does not represent a
        //      thread of execution
        //
        // Ensures: get_id() == id() is true and ssource_.stop_possible() is
        //      false.
        jthread() noexcept
          : ssource_{nostopstate}
        {
        }

        // Requires: F and each T in Ts meet the Cpp17MoveConstructible
        //      requirements. Either
        //
        //      INVOKE(decay-copy(HPX_FORWARD(F, f)), get_stop_token(),
        //             decay-copy(HPX_FORWARD(Ts, ts))...)
        //
        //      is a valid expression or
        //
        //      INVOKE(decay-copy(HPX_FORWARD(F, f)),
        //             decay-copy(HPX_FORWARD(Ts, ts))...)
        //
        //      is a valid expression.
        //
        // Constraints: remove_cvref_t<F> is not the same type as jthread.
        //
        // Effects: Initializes ssource_ and constructs an object of type
        //      jthread. The new thread of execution executes
        //
        //      INVOKE(decay-copy(HPX_FORWARD(F, f)), get_stop_token(),
        //             decay-copy(HPX_FORWARD(Ts, ts))...)
        //
        //      if that expression is well-formed, otherwise
        //
        //      INVOKE(decay-copy(HPX_FORWARD(F, f)),
        //             decay-copy(HPX_FORWARD(Ts, ts))...)
        //
        //      with the calls to decay-copy being evaluated in the
        //      constructing thread. Any return value from this invocation
        //      is ignored. If the INVOKE expression exits via an exception,
        //      terminate is called.
        //
        // Synchronization: The completion of the invocation of the
        //      constructor synchronizes with the beginning of the invocation
        //      of the copy of f.
        //
        // Ensures: get_id() != id() is true and ssource_.stop_possible() is
        //      true and *this represents the newly started thread.
        //
        // Throws: system_error if unable to start the new thread.
        //
        // Error conditions:
        //      resource_unavailable_try_again - the system lacked the
        //          necessary resources to create another thread, or the
        //          system-imposed limit on the number of threads in a process
        //          would be exceeded.
        //
        template <typename F, typename... Ts>
            requires(!std::same_as<std::decay_t<F>, jthread>)
        explicit jthread(F&& f, Ts&&... ts)
          : ssource_{}    // initialize stop_source
          , thread_{
                // lambda called in the thread
                [](stop_token st, F&& f, Ts&&... ts) -> void {
                    // perform tasks of the thread
                    using use_stop_token =
                        typename is_invocable<F, stop_token, Ts...>::type;

                    jthread::invoke(use_stop_token{}, HPX_FORWARD(F, f),
                        HPX_MOVE(st), HPX_FORWARD(Ts, ts)...);
                },
                // not captured due to possible races if immediately set
                ssource_.get_token(), HPX_FORWARD(F, f),    // pass callable
                HPX_FORWARD(Ts, ts)...    // pass arguments for callable
            }
        {
        }

        // Effects: If joinable() is true, calls request_stop() and then join().
        ~jthread()
        {
            if (joinable())
            {
                // if not joined/detached, signal stop and wait for end:
                request_stop();
                join();
            }
        }

        jthread(jthread const&) = delete;

        // Effects: Constructs an object of type jthread from x, and sets
        //      x to a default constructed state.
        //
        // Ensures: x.get_id() == id() and get_id() returns the value of
        //      x.get_id() prior to the start of construction. ssource_ has
        //      the value of x.ssource_ prior to the start of construction
        //      and x.ssource_.stop_possible() is false.
        //
        jthread(jthread&& x) noexcept = default;

        jthread& operator=(jthread const&) = delete;

        // Effects: If joinable() is true, calls request_stop() and then join().
        //      Assigns the state of x to *this and sets x to a default
        //      constructed state.
        //
        // Ensures: x.get_id() == id() and get_id() returns the value of
        //      x.get_id() prior to the assignment. ssource_ has the value of
        //      x.ssource_ prior to the assignment and x.ssource_.stop_possible()
        //      is false.
        //
        // Returns: *this.
        /// \brief moves the jthread object
        jthread& operator=(jthread&&) noexcept = default;

        // 32.4.3.2, members

        // Effects: Exchanges the values of *this and x.
        /// \brief swaps two jthread objects
        void swap(jthread& t) noexcept
        {
            using std::swap;
            swap(ssource_, t.ssource_);
            swap(thread_, t.thread_);
        }

        // Returns: get_id() != id().
        /// \brief checks whether the thread is joinable, i.e. potentially running
        ///        in parallel context
        [[nodiscard]] bool joinable() const noexcept
        {
            return thread_.joinable();
        }

        // Effects: Blocks until the thread represented by *this has
        //      completed.
        //
        // Synchronization: The completion of the thread represented
        //      by *this synchronizes with the corresponding successful
        //      join() return.
        //
        // Ensures: The thread represented by *this has completed.
        //      get_id() == id().
        //
        // Throws: system_error when an exception is required.
        //
        // Error conditions:
        //      - resource_deadlock_would_occur - if deadlock is detected
        //          or get_id() == this_thread_::get_id().
        //      - no_such_process - if the thread is not valid.
        //      - invalid_argument - if the thread is not joinable.
        /// \brief waits for the thread to finish its execution
        void join()
        {
            thread_.join();
        }

        // Effects: The thread represented by *this continues execution
        //      without the calling thread blocking. When detach() returns,
        //      *this no longer represents the possibly continuing thread
        //      of execution. When the thread previously represented by
        //      *this ends execution, the implementation shall release
        //      any owned resources.
        //
        // Ensures: get_id() == id().
        //
        // Throws: system_error when an exception is required
        //
        // Error conditions:
        //      - no_such_process - if the thread is not valid.
        //      - invalid_argument - if the thread is not joinable.
        /// \brief permits the thread to execute independently of  the thread handle
        void detach()
        {
            thread_.detach();
        }

        // Returns: A default constructed id object if *this does not
        //      represent a thread, otherwise this_thread_::get_id() for
        //      the thread of execution represented by *this.
        /// \brief returns the id of the thread
        [[nodiscard]] id get_id() const noexcept
        {
            return thread_.get_id();
        }

        // The presence of native_handle() and its semantic is
        //      implementation-defined.
        /// \brief returns the underlying implementation-defined thread handle
        [[nodiscard]] native_handle_type native_handle() const
        {
            return thread_.native_handle();
        }

        // 32.4.3.2, stop token handling

        // Effects: Equivalent to: return ssource_;
        /// \brief returns a stop_source object associated with the shared stop
        ///        state of the thread
        [[nodiscard]] stop_source get_stop_source() noexcept
        {
            return ssource_;
        }

        // Effects: Equivalent to: return ssource_.get_token();
        /// \brief returns a stop_token associated with the shared stop state of
        ///        the thread
        [[nodiscard]] stop_token get_stop_token() const noexcept
        {
            return ssource_.get_token();
        }

        // Effects: Equivalent to: return ssource_.request_stop();
        /// \brief requests execution stop via the shared stop state of the thread
        bool request_stop() const noexcept
        {
            return ssource_.request_stop();
        }

        // 32.4.3.5, static members

        // Returns: thread::hardware_concurrency().
        /// \brief returns the number of concurrent threads supported by the implementation
        [[nodiscard]] static unsigned int hardware_concurrency() noexcept
        {
            return hpx::threads::hardware_concurrency();
        }

    private:
        stop_source ssource_;     // stop_source for started thread
        hpx::thread thread_{};    // started thread (if any)
    };

    // 32.4.3.4, specialized algorithms

    // Effects: Equivalent to: x.swap(y).
    HPX_CXX_CORE_EXPORT inline void swap(jthread& lhs, jthread& rhs) noexcept
    {
        lhs.swap(rhs);
    }
}    // namespace hpx
