//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file find_all_localities.hpp
/// \page hpx::find_root_locality, hpx::find_all_localities, hpx::find_remote_localities
/// \headerfile hpx/runtime.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming_base.hpp>

#include <vector>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Return the global id representing the root locality
    ///
    /// The function \a find_root_locality() can be used to retrieve the global
    /// id usable to refer to the root locality. The root locality is the
    /// locality where the main AGAS service is hosted.
    ///
    /// \param ec [in,out] this represents the error status on exit, if this
    ///           is pre-initialized to \a hpx#throws the function will throw
    ///           on error instead.
    ///
    /// \note     Generally, the id of a locality can be used for instance to
    ///           create new instances of components and to invoke plain actions
    ///           (global functions).
    ///
    /// \returns  The global id representing the root locality for this
    ///           application.
    ///
    /// \note     As long as \a ec is not pre-initialized to \a hpx::throws this
    ///           function doesn't throw but returns the result code using the
    ///           parameter \a ec. Otherwise it throws an instance of
    ///           hpx::exception.
    ///
    /// \note     This function will return meaningful results only if called
    ///           from an HPX-thread. It will return \a hpx::invalid_id
    ///           otherwise.
    ///
    /// \see      \a hpx::find_all_localities(), \a hpx::find_locality()
    HPX_CXX_EXPORT HPX_EXPORT hpx::id_type find_root_locality(
        error_code& ec = throws);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Return the list of global ids representing all localities
    ///        available to this application.
    ///
    /// The function \a find_all_localities() can be used to retrieve the
    /// global ids of all localities currently available to this application.
    ///
    /// \param ec [in,out] this represents the error status on exit, if this
    ///           is pre-initialized to \a hpx#throws the function will throw
    ///           on error instead.
    ///
    /// \note     Generally, the id of a locality can be used for instance to
    ///           create new instances of components and to invoke plain actions
    ///           (global functions).
    ///
    /// \returns  The global ids representing the localities currently
    ///           available to this application.
    ///
    /// \note     As long as \a ec is not pre-initialized to \a hpx::throws this
    ///           function doesn't throw but returns the result code using the
    ///           parameter \a ec. Otherwise it throws an instance of
    ///           hpx::exception.
    ///
    /// \note     This function will return meaningful results only if called
    ///           from an HPX-thread. It will return an empty vector otherwise.
    ///
    /// \see      \a hpx::find_here(), \a hpx::find_locality()
    HPX_CXX_EXPORT HPX_EXPORT std::vector<hpx::id_type> find_all_localities(
        error_code& ec = throws);

    /// \brief Return the list of locality ids of remote localities supporting
    ///        the given component type. By default this function will return
    ///        the list of all remote localities (all but the current locality).
    ///
    /// The function \a find_remote_localities() can be used to retrieve the
    /// global ids of all remote localities currently available to this
    /// application (i.e. all localities except the current one).
    ///
    /// \param ec [in,out] this represents the error status on exit, if this
    ///           is pre-initialized to \a hpx#throws the function will throw
    ///           on error instead.
    ///
    /// \note     Generally, the id of a locality can be used for instance to
    ///           create new instances of components and to invoke plain actions
    ///           (global functions).
    ///
    /// \returns  The global ids representing the remote localities currently
    ///           available to this application.
    ///
    /// \note     As long as \a ec is not pre-initialized to \a hpx::throws this
    ///           function doesn't throw but returns the result code using the
    ///           parameter \a ec. Otherwise it throws an instance of
    ///           hpx::exception.
    ///
    /// \note     This function will return meaningful results only if called
    ///           from an HPX-thread. It will return an empty vector otherwise.
    ///
    /// \see      \a hpx::find_here(), \a hpx::find_locality()
    HPX_CXX_EXPORT HPX_EXPORT std::vector<hpx::id_type> find_remote_localities(
        error_code& ec = throws);
}    // namespace hpx
