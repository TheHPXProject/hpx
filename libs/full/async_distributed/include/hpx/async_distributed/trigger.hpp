//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/async_distributed/continuation_fwd.hpp>

#include <cstdlib>
#include <exception>
#include <utility>

namespace hpx::actions {

    namespace detail {

        HPX_CXX_EXPORT template <typename Result, typename RemoteResult>
        void trigger_error(typed_continuation<Result, RemoteResult> const& cont,
            std::exception_ptr const& ex) noexcept
        {
            hpx::detail::try_catch_exception_ptr(
                [&] {
                    // make sure hpx::exceptions are propagated back to the client
                    cont.trigger_error(ex);
                },
                [&](std::exception_ptr const&) {
#if defined(HPX_HAVE_FORCE_DISCONNECT)
                    if (parcelset::locality_was_disconnected(
                            naming::get_locality_id_from_id(cont.get_id())))
                    {
                        // ignore any errors as locality is now unreachable
                        return;
                    }
#endif
                    std::abort();    // nothing we can do here
                });
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Invoke \a f with the given arguments and forward its result to
    ///        \a cont.
    ///
    /// On success, the result of invoking \a f is passed to
    /// \a cont.trigger_value(). If invoking \a f throws, the exception is
    /// forwarded to the destination instead by calling
    /// \a cont.trigger_error() so that hpx::exceptions are propagated back
    /// to the client.
    ///
    /// \param cont The continuation that either the result or an exception
    ///             is delivered to.
    /// \param f    The callable to invoke.
    /// \param vs   The arguments to invoke \a f with.
    HPX_CXX_EXPORT template <typename Result, typename RemoteResult, typename F,
        typename... Ts>
    void trigger(typed_continuation<Result, RemoteResult>&& cont, F&& f,
        Ts&&... vs) noexcept
    {
        hpx::detail::try_catch_exception_ptr(
            [&] {
                cont.trigger_value(
                    HPX_INVOKE(HPX_FORWARD(F, f), HPX_FORWARD(Ts, vs)...));
            },
            [&](std::exception_ptr const& ep) {
                // make sure hpx::exceptions are propagated back to the client
                detail::trigger_error(cont, ep);
            });
    }

    /// \brief Invoke \a f with the given arguments and notify \a cont of
    ///        completion. Overload for when the return type is "void" aka
    ///        util::unused_type.
    ///
    /// On success, \a cont.trigger() is called to signal completion. If
    /// invoking \a f throws, the exception is forwarded to the destination
    /// instead by calling \a cont.trigger_error() so that hpx::exceptions are
    /// propagated back to the client.
    ///
    /// \param cont The continuation that completion or an exception is
    ///             delivered to.
    /// \param f    The callable to invoke.
    /// \param vs   The arguments to invoke \a f with.
    HPX_CXX_EXPORT template <typename Result, typename F, typename... Ts>
    void trigger(typed_continuation<Result, util::unused_type>&& cont, F&& f,
        Ts&&... vs) noexcept
    {
        hpx::detail::try_catch_exception_ptr(
            [&] {
                HPX_INVOKE(HPX_FORWARD(F, f), HPX_FORWARD(Ts, vs)...);
                cont.trigger();
            },
            [&](std::exception_ptr const& ep) {
                // make sure hpx::exceptions are propagated back to the client
                detail::trigger_error(cont, ep);
            });
    }
}    // namespace hpx::actions
