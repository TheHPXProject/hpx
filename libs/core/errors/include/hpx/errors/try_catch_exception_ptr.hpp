//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <exception>
#include <type_traits>
#include <utility>

namespace hpx::detail {

    /// Helper function for a try-catch block where what would normally go in
    /// the catch block should be called after the catch block. This is useful
    /// for situations where the catch-block may yield, since the catch block
    /// should be started and ended on the same worker thread (with yielding and
    /// stealing, the catch block may end on a different worker thread than
    /// where it was started). Because of this, the helper's catch block only
    /// stores the exception pointer, and forwards it outside the catch block.
    ///
    /// Do not replace uses of try_catch_exception_ptr with a plain try-catch
    /// without ensuring that the catch-block can never yield.
    ///
    /// Note: Windows does not seem to have problems resuming a catch block on a
    /// different worker thread, but we use this nonetheless on Windows since it
    /// doesn't hurt.
    ///
    /// \tparam ExceptionType The type of exception to catch. Defaults to
    ///         \a void, in which case any exception is caught and captured as
    ///         a \a std::exception_ptr via \a std::current_exception(). If a
    ///         concrete type is given, only exceptions matching
    ///         \a ExceptionType per C++ catch-clause matching rules (i.e. the
    ///         thrown object's type is \a ExceptionType, or an unambiguous
    ///         accessible public base thereof, modulo cv-qualification) are
    ///         caught and forwarded by value. \a ExceptionType must be
    ///         default-constructible and copy-assignable when a concrete type
    ///         is used.
    /// \tparam TryCallable A callable type with no parameters, invoked inside
    ///         the try block.
    /// \tparam CatchCallable A callable type taking a single parameter of type
    ///         \a std::exception_ptr (when \a ExceptionType is \a void) or of
    ///         type \a ExceptionType (otherwise), invoked with the captured
    ///         exception after the try-catch block has completed.
    ///
    /// \param t The callable to invoke inside the try block. Its return value
    ///          (if the try block does not throw) is returned directly from
    ///          \a try_catch_exception_ptr.
    /// \param c The callable to invoke with the captured exception if \a t
    ///          throws a matching exception. Its return value is returned from
    ///          \a try_catch_exception_ptr in that case.
    ///
    /// \return The result of invoking \a t() if no matching exception is
    ///         thrown; otherwise, the result of invoking \a c() with the
    ///         captured exception. Because the function returns
    ///         \a decltype(auto), every return statement in both branches
    ///         must deduce to the exact same type (after reference/cv folding);
    ///         the two return types are not merely required to be convertible
    ///         to one another, as a mismatch is ill-formed and fails to
    ///         compile.
    ///
    /// \note When \a ExceptionType is not \a void, only exceptions matching
    ///       that type (per catch-clause matching rules) are handled; any other
    ///       exception propagates normally out of
    ///       \a try_catch_exception_ptr instead of being forwarded to \a c.
    ///
    /// \note Because \a catch (ExceptionType const&) also matches exceptions
    ///       whose dynamic type publicly derives from \a ExceptionType, the
    ///       assignment \c e \c = \c caught in the typed branch may copy-assign
    ///       a derived object into a base-typed \a e. Any state introduced only
    ///       by the derived type -- for example diagnostic information mixed in
    ///       via \a hpx::exception_info (stack traces, thrown file/line, etc.)
    ///       -- is silently dropped in that case. If \a c needs access to such
    ///       diagnostics, capture \a std::current_exception() separately (e.g.
    ///       by using the default \a ExceptionType = void overload) instead of
    ///       relying on the typed value alone.
    HPX_CXX_CORE_EXPORT template <typename ExceptionType = void,
        typename TryCallable, typename CatchCallable>
    HPX_FORCEINLINE decltype(auto) try_catch_exception_ptr(
        TryCallable&& t, CatchCallable&& c)
    {
        if constexpr (std::is_void_v<ExceptionType>)
        {
            std::exception_ptr ep;
            try
            {
                return t();
            }
            catch (...)
            {
                ep = std::current_exception();
            }
            return c(HPX_MOVE(ep));
        }
        else
        {
            ExceptionType e;
            try
            {
                return t();
            }
            catch (ExceptionType const& caught)
            {
                e = caught;
            }
            return c(HPX_MOVE(e));
        }
    }
}    // namespace hpx::detail
