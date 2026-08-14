//  Copyright (c) 2018 Mikael Simberg
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx_finalize.hpp
/// \page hpx::suspend, hpx::resume
/// \headerfile hpx/init.hpp

#pragma once

#include <hpx/modules/errors.hpp>

/// \namespace hpx
namespace hpx {
    /// \brief Suspend the runtime system.
    ///
    /// The function \a hpx::suspend is used to suspend the HPX runtime system.
    /// It can only be used when running HPX on a single locality. It will block
    /// waiting for all thread pools to be empty. This function only be called
    /// when the runtime is running, or already suspended in which case this
    /// function will do nothing.
    ///
    /// \param ec [in,out] this represents the error status on exit, if this
    ///           is pre-initialized to \a hpx#throws the function will throw
    ///           on error instead.
    ///
    /// \returns  This function will always return zero.
    ///
    /// \note     As long as \a ec is not pre-initialized to \a hpx::throws this
    ///           function doesn't throw but returns the result code using the
    ///           parameter \a ec. Otherwise it throws an instance of
    ///           hpx::exception.
    HPX_CXX_EXPORT HPX_EXPORT int suspend(error_code& ec = throws);

    /// \brief Resume the HPX runtime system.
    ///
    /// The function \a hpx::resume is used to resume the HPX runtime system. It
    /// can only be used when running HPX on a single locality. It will block
    /// waiting for all thread pools to be resumed. This function only be called
    /// when the runtime suspended, or already running in which case this
    /// function will do nothing.
    ///
    /// \param ec [in,out] this represents the error status on exit, if this
    ///           is pre-initialized to \a hpx#throws the function will throw
    ///           on error instead.
    ///
    /// \returns  This function will always return zero.
    ///
    /// \note     As long as \a ec is not pre-initialized to \a hpx::throws this
    ///           function doesn't throw but returns the result code using the
    ///           parameter \a ec. Otherwise it throws an instance of
    ///           hpx::exception.
    HPX_CXX_EXPORT HPX_EXPORT int resume(error_code& ec = throws);
}    // namespace hpx
