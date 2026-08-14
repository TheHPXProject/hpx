//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file wait_all_for.hpp
/// \page hpx::wait_all_for, hpx::wait_all_for_nothrow, hpx::wait_all_for_n, hpx::wait_all_for_n_nothrow
/// \headerfile hpx/future.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    /// The function \a wait_all_for is an operator allowing to join on the
    /// result of all given futures, similar to \a wait_all. It differs from
    /// \a wait_all in that it will not suspend indefinitely if one or more of
    /// the given futures never become ready; instead it returns as soon as
    /// either all futures have become ready, or the given timeout has elapsed,
    /// whichever happens first.
    ///
    /// \param timeout  The maximum duration to wait for all the given
    ///                 futures to become ready.
    /// \param first    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all_for should wait.
    /// \param last     The iterator pointing to the element after the last one
    ///                 of a sequence of \a future or \a shared_future objects
    ///                 for which \a wait_all_for should wait.
    ///
    /// \return         Returns \a hpx::future_status::ready if all the
    ///                 given futures have become ready before \a timeout has
    ///                 elapsed, and \a hpx::future_status::timeout otherwise.
    ///
    /// \note The function \a wait_all_for returns after all futures have become
    ///       ready, or after the given timeout has expired, whichever comes
    ///       first. All input futures are still valid after
    ///       \a wait_all_for returns, independently of the returned status,
    ///       and can be inspected (e.g. using \a is_ready()) to determine which
    ///       of the given futures have become ready.
    ///
    /// \note           The function wait_all_for will rethrow any exceptions
    ///                 captured by the futures while becoming ready, as long as
    ///                 all the   given futures became ready before the timeout
    ///                 elapsed. If this behavior is undesirable, use
    ///                 \a wait_all_for_nothrow instead.
    template <typename InputIter>
    hpx::future_status wait_all_for(hpx::chrono::steady_duration const& timeout,
        InputIter first, InputIter last);

    /// The function \a wait_all_for is an operator allowing to join on the
    /// result of all given futures, similar to \a wait_all. It differs from
    /// \a wait_all in that it will not suspend indefinitely if one or more of
    /// the given futures never become ready; instead it returns as soon as
    /// either all futures have become ready, or the given timeout has elapsed,
    /// whichever happens first.
    ///
    /// \param timeout  The maximum duration to wait for all the given
    ///                 futures to become ready.
    /// \param futures  A vector or array holding an arbitrary amount of
    ///                 \a future or \a shared_future objects for which
    ///                 \a wait_all_for should wait.
    ///
    /// \return         Returns \a hpx::future_status::ready if all the
    ///                 given futures have become ready before \a timeout has
    ///                 elapsed, and \a hpx::future_status::timeout otherwise.
    ///
    /// \note The function \a wait_all_for returns after all futures have become
    ///       ready, or after the given timeout has expired, whichever comes
    ///       first. All input futures are still valid after
    ///       \a wait_all_for returns, independently of the returned status,
    ///       and can be inspected (e.g. using \a is_ready()) to determine which
    ///       of the given futures have become ready.
    ///
    /// \note           The function wait_all_for will rethrow any exceptions
    ///                 captured by the futures while becoming ready, as long as
    ///                 all the given futures became ready before the timeout
    ///                 elapsed. If this behavior is undesirable, use
    ///                 \a wait_all_for_nothrow instead.
    ///
    /// \note   The function wait_all_for returns after all futures have become
    ///         ready, or after the given timeout has expired, whichever comes
    ///         first. All input futures are still valid after wait_all_for
    ///         returns, independently of the returned status, and can be
    ///         inspected (e.g. using is_ready()) to determine which of the
    ///         given futures have become ready.
    ///
    /// \note   The caller is responsible for keeping the \a futures container
    ///         itself (and, for the iterator-range overload, the underlying
    ///         sequence) alive, unmoved, and unmodified until it is done
    ///         inspecting the futures' state. If the timeout expires before all
    ///         futures become ready, wait_all_for may leave asynchronous
    ///         continuations attached to the not-yet-ready futures; destroying,
    ///         moving, or reallocating the container before those futures
    ///         settle results in undefined behavior.
    template <typename R>
    hpx::future_status wait_all_for(hpx::chrono::steady_duration const& timeout,
        std::vector<future<R>> const& futures);

    /// The function \a wait_all_for is an operator allowing to join on the
    /// result of all given futures, similar to \a wait_all. It differs from
    /// \a wait_all in that it will not suspend indefinitely if one or more of
    /// the given futures never become ready; instead it returns as soon as
    /// either all futures have become ready, or the given timeout has
    /// elapsed, whichever happens first.
    ///
    /// \param timeout  The maximum duration to wait for all the given
    ///                 futures to become ready.
    /// \param futures  A vector or array holding an arbitrary amount of
    ///                 \a future or \a shared_future objects for which
    ///                 \a wait_all_for should wait.
    ///
    /// \return         Returns \a hpx::future_status::ready if all the
    ///                 given futures have become ready before \a timeout has
    ///                 elapsed, and \a hpx::future_status::timeout otherwise.
    ///
    /// \note The function \a wait_all_for returns after all futures have
    ///       become ready, or after the given timeout has expired, whichever
    ///       comes first. All input futures are still valid after
    ///       \a wait_all_for returns, independently of the returned status,
    ///       and can be inspected (e.g. using \a is_ready()) to determine
    ///       which of the given futures have become ready.
    ///
    /// \note           The function wait_all_for will rethrow any exceptions
    ///                 captured by the futures while becoming ready, as long
    ///                 as all the given futures became ready before the
    ///                 timeout elapsed. If this behavior is undesirable, use
    ///                 \a wait_all_for_nothrow instead.
    ///
    /// \note   The function wait_all_for returns after all futures have become
    ///         ready, or after the given timeout has expired, whichever comes
    ///         first. All input futures are still valid after wait_all_for
    ///         returns, independently of the returned status, and can be
    ///         inspected (e.g. using is_ready()) to determine which of the
    ///         given futures have become ready.
    ///
    /// \note   The caller is responsible for keeping the \a futures container
    ///         itself (and, for the iterator-range overload, the underlying
    ///         sequence) alive, unmoved, and unmodified until it is done
    ///         inspecting the futures' state. If the timeout expires before all
    ///         futures become ready, wait_all_for may leave asynchronous
    ///         continuations attached to the not-yet-ready futures; destroying,
    ///         moving, or reallocating the container before those futures
    ///         settle results in undefined behavior.
    template <typename R, std::size_t N>
    hpx::future_status wait_all_for(hpx::chrono::steady_duration const& timeout,
        std::array<future<R>, N> const& futures);

    /// The function \a wait_all_for is an operator allowing to join on the
    /// result of the given future, similar to \a wait_all. It differs from
    /// \a wait_all in that it will not suspend indefinitely if the given
    /// future never becomes ready; instead it returns as soon as either the
    /// future has become ready, or the given timeout has elapsed, whichever
    /// happens first.
    ///
    /// \param timeout  The maximum duration to wait for the given future to
    ///                 become ready.
    /// \param f        A \a future or \a shared_future for which
    ///                 \a wait_all_for should wait.
    ///
    /// \return         Returns \a hpx::future_status::ready if the given
    ///                 future has become ready before \a timeout has
    ///                 elapsed, and \a hpx::future_status::timeout otherwise.
    ///
    /// \note The function \a wait_all_for returns after the future has
    ///       become ready, or after the given timeout has expired, whichever
    ///       comes first. The input future is still valid after
    ///       \a wait_all_for returns, independently of the returned status.
    ///
    /// \note           The function wait_all_for will rethrow any exceptions
    ///                 captured by the future while becoming ready, as long
    ///                 as the given future became ready before the timeout
    ///                 elapsed. If this behavior is undesirable, use
    ///                 \a wait_all_for_nothrow instead.
    ///
    template <typename T>
    hpx::future_status wait_all_for(
        hpx::chrono::steady_duration const& timeout, hpx::future<T> const& f);

    /// The function \a wait_all_for is an operator allowing to join on the
    /// result of all given futures, similar to \a wait_all. It differs from
    /// \a wait_all in that it will not suspend indefinitely if one or more of
    /// the given futures never become ready; instead it returns as soon as
    /// either all futures have become ready, or the given timeout has
    /// elapsed, whichever happens first.
    ///
    /// \param timeout  The maximum duration to wait for all the given
    ///                 futures to become ready.
    /// \param futures  An arbitrary number of \a future or \a shared_future
    ///                 objects, possibly holding different types for which
    ///                 \a wait_all_for should wait.
    ///
    /// \return         Returns \a hpx::future_status::ready if all the
    ///                 given futures have become ready before \a timeout has
    ///                 elapsed, and \a hpx::future_status::timeout otherwise.
    ///
    /// \note The function \a wait_all_for returns after all futures have
    ///       become ready, or after the given timeout has expired, whichever
    ///       comes first. All input futures are still valid after
    ///       \a wait_all_for returns, independently of the returned status,
    ///       and can be inspected (e.g. using \a is_ready()) to determine
    ///       which of the given futures have become ready.
    ///
    /// \note           The function wait_all_for will rethrow any exceptions
    ///                 captured by the futures while becoming ready, as long
    ///                 as all the given futures became ready before the
    ///                 timeout elapsed. If this behavior is undesirable, use
    ///                 \a wait_all_for_nothrow instead.
    ///
    template <typename... T>
    hpx::future_status wait_all_for(
        hpx::chrono::steady_duration const& timeout, T const&... futures);

    /// The function \a wait_all_for_n is an operator allowing to join on the
    /// result of all given futures, similar to \a wait_all_n. It differs from
    /// \a wait_all_n in that it will not suspend indefinitely if one or
    /// more of the given futures never become ready; instead it returns as soon
    /// as either all futures have become ready, or the given timeout has
    /// elapsed, whichever happens first.
    ///
    /// \param timeout  The maximum duration to wait for all the given
    ///                 futures to become ready.
    /// \param begin    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all_for_n should wait.
    /// \param count    The number of elements in the sequence starting at
    ///                 \a first.
    ///
    /// \return         Returns \a hpx::future_status::ready if all the
    ///                 given futures have become ready before \a timeout has
    ///                 elapsed, and \a hpx::future_status::timeout otherwise.
    ///
    /// \note The function \a wait_all_for_n returns after all futures have
    ///       become ready, or after the given timeout has expired, whichever
    ///       comes first. All input futures are still valid after
    ///       \a wait_all_for_n returns, independently of the returned
    ///       status, and can be inspected (e.g. using \a is_ready()) to
    ///       determine which of the given futures have become ready.
    ///
    /// \note           The function wait_all_for_n will rethrow any
    ///                 exceptions captured by the futures while becoming ready,
    ///                 as long as all the given futures became ready before the
    ///                 timeout elapsed. If this behavior is undesirable, use \a
    ///                 wait_all_for_n_nothrow instead.
    template <typename InputIter>
    hpx::future_status wait_all_for_n(
        hpx::chrono::steady_duration const& timeout, InputIter begin,
        std::size_t count);

    /// The type \a wait_all_for_nothrow_result is the return type of all
    /// overloads of \a wait_all_for_nothrow and \a wait_all_for_n_nothrow. It
    /// reports both whether the wait operation completed before the given
    /// timeout elapsed, and whether any of the waited-on futures held an
    /// exception once they became ready.
    ///
    /// \note   Unlike \a wait_all_for (and \a wait_all_for_n), the
    ///         \a wait_all_for_nothrow family of functions never rethrows
    ///         exceptions captured by the given futures. Instead, the presence
    ///         of any such exception is reported through the
    ///         \a has_exceptional_results member of the returned
    ///         \a wait_all_for_nothrow_result object, allowing the caller to
    ///         decide how (or whether) to handle it.
    ///
    struct wait_all_for_nothrow_result
    {
        /// Indicates whether all the given futures became ready before the
        /// specified timeout elapsed.
        ///
        /// \details    Set to \a hpx::future_status::ready if all the given
        ///             futures became ready before the timeout elapsed, and to
        ///             \a hpx::future_status::timeout otherwise.
        hpx::future_status status;

        /// Indicates whether any of the given futures held an exception once it
        /// became ready.
        ///
        /// \details    Set to \a true if at least one of the futures that
        ///             became ready (before the timeout elapsed) had captured
        ///             an exception, and \a false otherwise. This flag is only
        ///             meaningful with respect to the futures that actually
        ///             became ready; futures that are still pending when the
        ///             timeout expires are not reflected here.
        bool has_exceptional_results;
    };

    /// The function \a wait_all_for_nothrow is an operator allowing to join on
    /// the result of all given futures, similar to \a wait_all_for. It differs
    /// from \a wait_all_for in that it will not rethrow any exceptions captured
    /// by the given futures while they became ready; instead it returns as soon
    /// as either all futures have become ready, or the given timeout has
    /// elapsed, whichever happens first.
    ///
    /// \param timeout  The maximum duration to wait for all the given
    ///                 futures to become ready.
    /// \param first    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all_for_nothrow should wait.
    /// \param last     The iterator pointing to the element after the last one
    ///                 of a sequence of \a future or \a shared_future objects
    ///                 for which \a wait_all_for_nothrow should wait.
    ///
    /// \return         Returns a \a wait_all_for_nothrow_result object whose
    ///                 \a status member is \a hpx::future_status::ready if all
    ///                 the given futures have become ready before \a timeout
    ///                 has elapsed, and \a hpx::future_status::timeout
    ///                 otherwise; and whose \a has_exceptional_results member
    ///                 indicates whether any of the futures that became ready
    ///                 had captured an exception.
    ///
    /// \note The function \a wait_all_for_nothrow returns after all futures
    ///       have become ready, or after the given timeout has expired,
    ///       whichever comes first. All input futures are still valid after
    ///       \a wait_all_for_nothrow returns, independently of the returned
    ///       status, and can be inspected (e.g. using \a is_ready()) to
    ///       determine which of the given futures have become ready.
    ///
    /// \note           Unlike \a wait_all_for_nothrow, this function will not
    ///                 rethrow any exceptions captured by the futures while
    ///                 becoming ready. Any such exceptions are not rethrown by
    ///                 this call.
    template <typename InputIter>
    hpx::wait_all_for_nothrow_result wait_all_for_nothrow(
        hpx::chrono::steady_duration const& timeout, InputIter first,
        InputIter last);

    /// The function \a wait_all_for_nothrow is an operator allowing to join on
    /// the result of all given futures, similar to \a wait_all_for. It differs
    /// from \a wait_all_for in that it will not rethrow any exceptions captured
    /// by the given futures while they became ready; instead it returns as soon
    /// as either all futures have become ready, or the given timeout has
    /// elapsed, whichever happens first.
    ///
    /// \param timeout  The maximum duration to wait for all the given
    ///                 futures to become ready.
    /// \param futures  A vector or array holding an arbitrary amount of
    ///                 \a future or \a shared_future objects for which
    ///                 \a wait_all_for_nothrow should wait.
    ///
    /// \return         Returns a \a wait_all_for_nothrow_result object whose
    ///                 \a status member is \a hpx::future_status::ready if all
    ///                 the given futures have become ready before \a timeout
    ///                 has elapsed, and \a hpx::future_status::timeout
    ///                 otherwise; and whose \a has_exceptional_results member
    ///                 indicates whether any of the futures that became ready
    ///                 had captured an exception.
    ///
    /// \note The function \a wait_all_for_nothrow returns after all futures
    ///       have become ready, or after the given timeout has expired,
    ///       whichever comes first. All input futures are still valid after
    ///       \a wait_all_for_nothrow returns, independently of the returned
    ///       status, and can be inspected (e.g. using \a is_ready()) to
    ///       determine which of the given futures have become ready.
    ///
    /// \note           Unlike \a wait_all_for, this function will not
    ///                 rethrow any exceptions captured by the futures while
    ///                 becoming ready. Any such exceptions are not rethrown by
    ///                 this call.
    ///
    /// \note   The function wait_all_for_nothrow returns after all futures have
    ///         become ready, or after the given timeout has expired, whichever
    ///         comes first. All input futures are still valid after
    ///         wait_all_for returns, independently of the returned status, and
    ///         can be inspected (e.g. using is_ready()) to determine which of
    ///         the given futures have become ready.
    ///
    /// \note   The caller is responsible for keeping the \a futures container
    ///         itself (and, for the iterator-range overload, the underlying
    ///         sequence) alive, unmoved, and unmodified until it is done
    ///         inspecting the futures' state. If the timeout expires before all
    ///         futures become ready, wait_all_for may leave asynchronous
    ///         continuations attached to the not-yet-ready futures; destroying,
    ///         moving, or reallocating the container before those futures
    ///         settle results in undefined behavior.
    template <typename R>
    hpx::wait_all_for_nothrow_result wait_all_for_nothrow(
        hpx::chrono::steady_duration const& timeout,
        std::vector<future<R>> const& futures);

    /// The function \a wait_all_for_nothrow is an operator allowing to join on
    /// the result of all given futures, similar to \a wait_all_for. It differs
    /// from \a wait_all_for in that it will not rethrow any exceptions captured
    /// by the given futures while they became ready; instead it returns as soon
    /// as either all futures have become ready, or the given timeout has
    /// elapsed, whichever happens first.
    ///
    /// \param timeout  The maximum duration to wait for all the given
    ///                 futures to become ready.
    /// \param futures  A vector or array holding an arbitrary amount of
    ///                 \a future or \a shared_future objects for which
    ///                 \a wait_all_for_nothrow should wait.
    ///
    /// \return         Returns a \a wait_all_for_nothrow_result object whose
    ///                 \a status member is \a hpx::future_status::ready if all
    ///                 the given futures have become ready before \a timeout
    ///                 has elapsed, and \a hpx::future_status::timeout
    ///                 otherwise; and whose \a has_exceptional_results member
    ///                 indicates whether any of the futures that became ready
    ///                 had captured an exception.
    ///
    /// \note The function \a wait_all_for_nothrow returns after all futures
    ///       have become ready, or after the given timeout has expired,
    ///       whichever comes first. All input futures are still valid after
    ///       \a wait_all_for_nothrow returns, independently of the returned
    ///       status, and can be inspected (e.g. using \a is_ready()) to
    ///       determine which of the given futures have become ready.
    ///
    /// \note           Unlike \a wait_all_for, this function will not
    ///                 rethrow any exceptions captured by the futures while
    ///                 becoming ready. Any such exceptions are not rethrown by
    ///                 this call.
    ///
    /// \note   The function wait_all_for_nothrow returns after all futures have
    ///         become ready, or after the given timeout has expired, whichever
    ///         comes first. All input futures are still valid after
    ///         wait_all_for returns, independently of the returned status, and
    ///         can be inspected (e.g. using is_ready()) to determine which of
    ///         the given futures have become ready.
    ///
    /// \note   The caller is responsible for keeping the \a futures container
    ///         itself (and, for the iterator-range overload, the underlying
    ///         sequence) alive, unmoved, and unmodified until it is done
    ///         inspecting the futures' state. If the timeout expires before all
    ///         futures become ready, wait_all_for may leave asynchronous
    ///         continuations attached to the not-yet-ready futures; destroying,
    ///         moving, or reallocating the container before those futures
    ///         settle results in undefined behavior.
    template <typename R, std::size_t N>
    hpx::wait_all_for_nothrow_result wait_all_for_nothrow(
        hpx::chrono::steady_duration const& timeout,
        std::array<future<R>, N> const& futures);

    /// The function \a wait_all_for_nothrow is an operator allowing to join on
    /// the result of the given future, similar to \a wait_all_for. It differs
    /// from \a wait_all_for in that it will not rethrow any exceptions captured
    /// by the given future while it became ready; instead it returns as soon as
    /// either the future has become ready, or the given timeout has elapsed,
    /// whichever happens first.
    ///
    /// \param timeout  The maximum duration to wait for the given future to
    ///                 become ready.
    /// \param f        A \a future or \a shared_future for which
    ///                 \a wait_all_for_nothrow should wait.
    ///
    /// \return         Returns a \a wait_all_for_nothrow_result object whose
    ///                 \a status member is \a hpx::future_status::ready if all
    ///                 the given futures have become ready before \a timeout
    ///                 has elapsed, and \a hpx::future_status::timeout
    ///                 otherwise; and whose \a has_exceptional_results member
    ///                 indicates whether any of the futures that became ready
    ///                 had captured an exception.
    ///
    /// \note The function \a wait_all_for_nothrow returns after the future has
    ///       become ready, or after the given timeout has expired, whichever
    ///       comes first. The input future is still valid after
    ///       \a wait_all_for_nothrow returns, independently of the returned
    ///       status.
    ///
    /// \note           Unlike \a wait_all_for, this function will not
    ///                 rethrow any exceptions captured by the future while
    ///                 becoming ready. Any such exception is silently
    ///                 discarded.
    ///
    template <typename T>
    hpx::wait_all_for_nothrow_result wait_all_for_nothrow(
        hpx::chrono::steady_duration const& timeout, hpx::future<T> const& f);

    /// The function \a wait_all_for_nothrow is an operator allowing to join on
    /// the result of all given futures, similar to \a wait_all_for. It differs
    /// from \a wait_all_for in that it will not rethrow any exceptions captured
    /// by the given futures while they became ready; instead it returns as soon
    /// as either all futures have become ready, or the given timeout has
    /// elapsed, whichever happens first.
    ///
    /// \param timeout  The maximum duration to wait for all the given
    ///                 futures to become ready.
    /// \param futures  An arbitrary number of \a future or \a shared_future
    ///                 objects, possibly holding different types for which
    ///                 \a wait_all_for_nothrow should wait.
    ///
    /// \return         Returns a \a wait_all_for_nothrow_result object whose
    ///                 \a status member is \a hpx::future_status::ready if all
    ///                 the given futures have become ready before \a timeout
    ///                 has elapsed, and \a hpx::future_status::timeout
    ///                 otherwise; and whose \a has_exceptional_results member
    ///                 indicates whether any of the futures that became ready
    ///                 had captured an exception.
    ///
    /// \note The function \a wait_all_for_nothrow returns after all futures
    ///       have become ready, or after the given timeout has expired,
    ///       whichever comes first. All input futures are still valid after
    ///       \a wait_all_for_nothrow returns, independently of the returned
    ///       status, and can be inspected (e.g. using \a is_ready()) to
    ///       determine which of the given futures have become ready.
    ///
    /// \note           Unlike \a wait_all_for, this function will not
    ///                 rethrow any exceptions captured by the futures while
    ///                 becoming ready. Any such exceptions are not rethrown by
    ///                 this call.
    ///
    template <typename... T>
    hpx::wait_all_for_nothrow_result wait_all_for_nothrow(
        hpx::chrono::steady_duration const& timeout, T const&... futures);

    /// The function \a wait_all_for_n_nothrow is an operator allowing to join
    /// on the result of all given futures, similar to \a wait_all_n. It differs
    /// from \a wait_all_for_n in that it will not rethrow any exceptions
    /// captured by the given futures while they became ready; instead it
    /// returns as soon as either all futures have become ready, or the given
    /// timeout has elapsed, whichever happens first.
    ///
    /// \param timeout  The maximum duration to wait for all the given
    ///                 futures to become ready.
    /// \param begin    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all_for_n_nothrow should wait.
    /// \param count    The number of elements in the sequence starting at
    ///                 \a first.
    ///
    /// \return         Returns a \a wait_all_for_nothrow_result object whose
    ///                 \a status member is \a hpx::future_status::ready if all
    ///                 the given futures have become ready before \a timeout
    ///                 has elapsed, and \a hpx::future_status::timeout
    ///                 otherwise; and whose \a has_exceptional_results member
    ///                 indicates whether any of the futures that became ready
    ///                 had captured an exception.
    ///
    /// \note The function \a wait_all_for_n_nothrow returns after all futures
    ///       have become ready, or after the given timeout has expired,
    ///       whichever comes first. All input futures are still valid after \a
    ///       wait_all_for_n_nothrow returns, independently of the returned
    ///       status, and can be inspected (e.g. using
    ///       \a is_ready()) to determine which of the given futures have
    ///       become ready.
    ///
    /// \note           Unlike \a wait_all_for_n, this function will not
    ///                 rethrow any exceptions captured by the futures while
    ///                 becoming ready. Any such exceptions are not rethrown by
    ///                 this call.
    template <typename InputIter>
    hpx::wait_all_for_nothrow_result wait_all_for_n_nothrow(
        hpx::chrono::steady_duration const& timeout, InputIter begin,
        std::size_t count);

}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/async_combinators/detail/throw_if_exceptional.hpp>
#include <hpx/async_combinators/wait_all.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/type_support.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
#if !defined(HPX_INTEL_VERSION)
#define HPX_WAIT_ALL_FOR_FORCEINLINE HPX_FORCEINLINE
#else
#define HPX_WAIT_ALL_FOR_FORCEINLINE
#endif

namespace hpx {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename Tuple>
        struct wait_all_for_frame
          : wait_all_frame<Tuple, wait_all_for_frame<Tuple>>
        {
            using base_type = wait_all_frame<Tuple, wait_all_for_frame<Tuple>>;

            explicit wait_all_for_frame(Tuple const& t)
              : base_type(t)
            {
            }

            explicit wait_all_for_frame(Tuple&& t) noexcept
              : base_type(HPX_MOVE(t))
            {
            }

            wait_all_for_frame(wait_all_for_frame const&) = delete;
            wait_all_for_frame(wait_all_for_frame&&) = delete;

            wait_all_for_frame& operator=(wait_all_for_frame const&) = delete;
            wait_all_for_frame& operator=(wait_all_for_frame&&) = delete;

            // Same as wait_all(), except that the final suspend is bounded by a
            // given timeout instead of waiting indefinitely. Note that the
            // individual input futures are only ever inspected/attached-to,
            // never moved-from, so they remain valid (and can be queried with
            // is_ready()) whether the combined wait finishes because all them
            // became ready or because the timeout elapsed first.
            hpx::future_status wait_all_for(
                hpx::chrono::steady_duration const& timeout)
            {
                // Every fresh invocation of wait_all_for() must reset the flag.
                continue_waiting_.store(true, std::memory_order_release);

                this->base_type::template do_await<0>();

                // If there are still futures which are not ready, suspend and wait,
                // but no longer than the given timeout. Note that this does not
                // cancel/abandon the still outstanding futures; they simply
                // continue to be resolved in the background and can still be
                // queried by the caller after this function returns.
                if (!this->is_ready(std::memory_order_relaxed))
                {
                    hpx::future_status const status =
                        this->wait_until(timeout.from_now());
                    if (status == hpx::future_status::timeout)
                    {
                        // this will prevent pending continuations from
                        // continuing through the sequence of futures
                        continue_waiting_.store(
                            false, std::memory_order_release);
                    }
                    return status;
                }

                return hpx::future_status::ready;
            }

            bool continue_waiting() const noexcept
            {
                return continue_waiting_.load(std::memory_order_acquire);
            }

        private:
            std::atomic<bool> continue_waiting_{true};
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct wait_all_for_nothrow_result
    {
        hpx::future_status status;
        bool has_exceptional_results;
    };

    HPX_CXX_CORE_EXPORT inline constexpr struct wait_all_for_nothrow_t final
    {
    private:
        template <typename Future>
        static wait_all_for_nothrow_result wait_all_for_nothrow_impl(
            std::vector<Future> const& values,
            hpx::chrono::steady_duration const& timeout)
        {
            if (!values.empty())
            {
                using result_type = hpx::tuple<std::vector<Future> const&>;
                using frame_type = hpx::detail::wait_all_for_frame<result_type>;

                result_type data(values);

                // frame is initialized with initial reference count
                hpx::intrusive_ptr<frame_type> frame(
                    new frame_type(data), false);

                auto status = frame->wait_all_for(timeout);
                return {.status = status,
                    .has_exceptional_results =
                        frame->has_exceptional_results()};
            }
            return {.status = hpx::future_status::ready,
                .has_exceptional_results = false};
        }

        template <typename Future>
        static wait_all_for_nothrow_result wait_all_for_nothrow_impl(
            std::vector<Future>&& values,
            hpx::chrono::steady_duration const& timeout)
        {
            if (!values.empty())
            {
                using result_type = hpx::tuple<std::vector<Future>>;
                using frame_type = hpx::detail::wait_all_for_frame<result_type>;

                result_type data(HPX_MOVE(values));

                // frame is initialized with initial reference count
                hpx::intrusive_ptr<frame_type> frame(
                    new frame_type(data), false);

                auto status = frame->wait_all_for(timeout);
                return {.status = status,
                    .has_exceptional_results =
                        frame->has_exceptional_results()};
            }
            return {.status = hpx::future_status::ready,
                .has_exceptional_results = false};
        }

    public:
        template <typename Future>
        wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const& timeout,
            std::vector<Future> const& values) const
        {
            return wait_all_for_nothrow_t::wait_all_for_nothrow_impl(
                values, timeout);
        }

        template <typename Future>
        wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const& timeout,
            std::vector<Future>&& values) const
        {
            return wait_all_for_nothrow_t::wait_all_for_nothrow_impl(
                HPX_MOVE(values), timeout);
        }

        template <typename Future>
        HPX_WAIT_ALL_FOR_FORCEINLINE wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const& timeout,
            std::vector<Future>& values) const
        {
            return wait_all_for_nothrow_t::wait_all_for_nothrow_impl(
                const_cast<std::vector<Future> const&>(values), timeout);
        }

        template <typename Future, std::size_t N>
        static wait_all_for_nothrow_result wait_all_for_nothrow_impl(
            std::array<Future, N> const& values,
            hpx::chrono::steady_duration const& timeout)
        {
            using result_type = hpx::tuple<std::array<Future, N> const&>;
            using frame_type = hpx::detail::wait_all_for_frame<result_type>;

            result_type data(values);

            // frame is initialized with initial reference count
            hpx::intrusive_ptr<frame_type> frame(new frame_type(data), false);

            auto status = frame->wait_all_for(timeout);
            return {.status = status,
                .has_exceptional_results = frame->has_exceptional_results()};
        }

        template <typename Future, std::size_t N>
        static wait_all_for_nothrow_result wait_all_for_nothrow_impl(
            std::array<Future, N>&& values,
            hpx::chrono::steady_duration const& timeout)
        {
            using result_type = hpx::tuple<std::array<Future, N>>;
            using frame_type = hpx::detail::wait_all_for_frame<result_type>;

            result_type data(HPX_MOVE(values));

            // frame is initialized with initial reference count
            hpx::intrusive_ptr<frame_type> frame(new frame_type(data), false);

            auto status = frame->wait_all_for(timeout);
            return {.status = status,
                .has_exceptional_results = frame->has_exceptional_results()};
        }

        template <typename Future, std::size_t N>
        wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const& timeout,
            std::array<Future, N> const& values) const
        {
            return wait_all_for_nothrow_t::wait_all_for_nothrow_impl(
                values, timeout);
        }

        template <typename Future, std::size_t N>
        wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const& timeout,
            std::array<Future, N>&& values) const
        {
            return wait_all_for_nothrow_t::wait_all_for_nothrow_impl(
                HPX_MOVE(values), timeout);
        }

        template <typename Future, std::size_t N>
        HPX_WAIT_ALL_FOR_FORCEINLINE wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const& timeout,
            std::array<Future, N>& values) const
        {
            return wait_all_for_nothrow_t::wait_all_for_nothrow_impl(
                const_cast<std::array<Future, N> const&>(values), timeout);
        }

        template <typename Iterator>
            requires(hpx::traits::is_iterator_v<Iterator>)
        wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const& timeout, Iterator begin,
            Iterator end) const
        {
            if (begin == end)
            {
                return {.status = hpx::future_status::ready,
                    .has_exceptional_results = false};
            }

            auto values = traits::acquire_shared_state<Iterator>()(begin, end);
            return wait_all_for_nothrow_t::wait_all_for_nothrow_impl(
                HPX_MOVE(values), timeout);
        }

        HPX_WAIT_ALL_FOR_FORCEINLINE wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const&) const noexcept
        {
            return {.status = hpx::future_status::ready,
                .has_exceptional_results = false};
        }

        template <typename... Ts>
        wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const& timeout, Ts const&... ts) const
        {
            if constexpr (sizeof...(Ts) != 0)
            {
                using result_type =
                    hpx::tuple<traits::detail::shared_state_ptr_for_t<Ts>...>;
                using frame_type = hpx::detail::wait_all_for_frame<result_type>;

                result_type values =
                    result_type(hpx::traits::detail::get_shared_state(ts)...);

                // frame is initialized with initial reference count
                hpx::intrusive_ptr<frame_type> frame(
                    new frame_type(HPX_MOVE(values)), false);

                auto status = frame->wait_all_for(timeout);
                return {.status = status,
                    .has_exceptional_results =
                        frame->has_exceptional_results()};
            }
            else
            {
                return {.status = hpx::future_status::ready,
                    .has_exceptional_results = false};
            }
        }

        template <typename T>
        HPX_WAIT_ALL_FOR_FORCEINLINE wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const& timeout,
            hpx::future<T> const& f) const
        {
            auto status = f.wait_for(timeout);
            return {
                .status = status, .has_exceptional_results = f.has_exception()};
        }

        template <typename T>
        HPX_WAIT_ALL_FOR_FORCEINLINE wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const& timeout,
            hpx::shared_future<T> const& f) const
        {
            auto status = f.wait_for(timeout);
            return {
                .status = status, .has_exceptional_results = f.has_exception()};
        }
    } wait_all_for_nothrow{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_all_for_t final
    {
        template <typename Future>
        HPX_WAIT_ALL_FOR_FORCEINLINE hpx::future_status operator()(
            hpx::chrono::steady_duration const& timeout,
            std::vector<Future> const& values) const
        {
            auto const [status, has_exceptional_results] =
                hpx::wait_all_for_nothrow(timeout, values);
            if (has_exceptional_results)
            {
                hpx::detail::throw_if_exceptional(values);
            }
            return status;
        }

        template <typename Future>
        HPX_WAIT_ALL_FOR_FORCEINLINE hpx::future_status operator()(
            hpx::chrono::steady_duration const& timeout,
            std::vector<Future>& values) const
        {
            auto const [status, has_exceptional_results] =
                hpx::wait_all_for_nothrow(
                    timeout, const_cast<std::vector<Future> const&>(values));
            if (has_exceptional_results)
            {
                hpx::detail::throw_if_exceptional(values);
            }
            return status;
        }

        template <typename Future, std::size_t N>
        HPX_WAIT_ALL_FOR_FORCEINLINE hpx::future_status operator()(
            hpx::chrono::steady_duration const& timeout,
            std::array<Future, N> const& values) const
        {
            auto const [status, has_exceptional_results] =
                hpx::wait_all_for_nothrow(timeout, values);
            if (has_exceptional_results)
            {
                hpx::detail::throw_if_exceptional(values);
            }
            return status;
        }

        template <typename Future, std::size_t N>
        HPX_WAIT_ALL_FOR_FORCEINLINE hpx::future_status operator()(
            hpx::chrono::steady_duration const& timeout,
            std::array<Future, N>& values) const
        {
            auto const [status, has_exceptional_results] =
                hpx::wait_all_for_nothrow(
                    timeout, const_cast<std::array<Future, N> const&>(values));
            if (has_exceptional_results)
            {
                hpx::detail::throw_if_exceptional(values);
            }
            return status;
        }

        template <typename Iterator>
            requires(hpx::traits::is_iterator_v<Iterator>)
        hpx::future_status operator()(
            hpx::chrono::steady_duration const& timeout, Iterator begin,
            Iterator end) const
        {
            if (begin == end)
            {
                return hpx::future_status::ready;
            }

            auto values = traits::acquire_shared_state<Iterator>()(begin, end);
            auto const [status, has_exceptional_results] =
                hpx::wait_all_for_nothrow(timeout, HPX_MOVE(values));
            if (has_exceptional_results)
            {
                hpx::detail::throw_if_exceptional(begin, end);
            }
            return status;
        }

        HPX_WAIT_ALL_FOR_FORCEINLINE hpx::future_status operator()(
            hpx::chrono::steady_duration const&) const noexcept
        {
            return hpx::future_status::ready;
        }

        template <typename... Ts>
        hpx::future_status operator()(
            hpx::chrono::steady_duration const& timeout, Ts const&... ts) const
        {
            auto const [status, has_exceptional_results] =
                hpx::wait_all_for_nothrow(timeout, ts...);
            if (has_exceptional_results)
            {
                hpx::detail::throw_if_exceptional(ts...);
            }
            return status;
        }

        template <typename T>
        HPX_WAIT_ALL_FOR_FORCEINLINE hpx::future_status operator()(
            hpx::chrono::steady_duration const& timeout,
            hpx::future<T> const& f) const
        {
            auto const [status, has_exceptional_results] =
                hpx::wait_all_for_nothrow(timeout, f);
            if (has_exceptional_results)
            {
                hpx::detail::throw_if_exceptional(f);
            }
            return status;
        }

        template <typename T>
        HPX_WAIT_ALL_FOR_FORCEINLINE hpx::future_status operator()(
            hpx::chrono::steady_duration const& timeout,
            hpx::shared_future<T> const& f) const
        {
            auto const [status, has_exceptional_results] =
                hpx::wait_all_for_nothrow(timeout, f);
            if (has_exceptional_results)
            {
                hpx::detail::throw_if_exceptional(f);
            }
            return status;
        }
    } wait_all_for{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_all_for_n_nothrow_t final
    {
        template <typename Iterator>
            requires(hpx::traits::is_iterator_v<Iterator>)
        wait_all_for_nothrow_result operator()(
            hpx::chrono::steady_duration const& timeout, Iterator begin,
            std::size_t count) const
        {
            if (count == 0)
            {
                return {.status = hpx::future_status::ready,
                    .has_exceptional_results = false};
            }

            auto values =
                traits::acquire_shared_state<Iterator>()(begin, count);
            return hpx::wait_all_for_nothrow(timeout, HPX_MOVE(values));
        }
    } wait_all_for_n_nothrow{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct wait_all_for_n_t final
    {
        template <typename Iterator>
            requires(hpx::traits::is_iterator_v<Iterator>)
        hpx::future_status operator()(
            hpx::chrono::steady_duration const& timeout, Iterator begin,
            std::size_t count) const
        {
            if (count == 0)
            {
                return hpx::future_status::ready;
            }

            auto values =
                traits::acquire_shared_state<Iterator>()(begin, count);
            auto const [status, has_exceptional_results] =
                hpx::wait_all_for_nothrow(timeout, HPX_MOVE(values));
            if (has_exceptional_results)
            {
                hpx::detail::throw_if_exceptional(begin, count);
            }
            return status;
        }
    } wait_all_for_n{};
}    // namespace hpx

#undef HPX_WAIT_ALL_FOR_FORCEINLINE

#endif    // DOXYGEN
