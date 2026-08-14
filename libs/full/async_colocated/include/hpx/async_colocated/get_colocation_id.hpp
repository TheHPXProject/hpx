//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file get_colocation_id.hpp
/// \page hpx::get_colocation_id
/// \headerfile hpx/runtime.hpp

#pragma once

#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Return the id of the locality where the object referenced by the
    ///        given id is currently located on
    ///
    /// The function hpx::get_colocation_id() returns the id of the locality
    /// where the given object is currently located.
    ///
    /// \param id [in] The id of the object to locate.
    /// \param ec [in,out] this represents the error status on exit, if this
    ///           is pre-initialized to \a hpx#throws the function will throw
    ///           on error instead.
    ///
    /// \note     As long as \a ec is not pre-initialized to \a hpx::throws this
    ///           function doesn't throw but returns the result code using the
    ///           parameter \a ec. Otherwise it throws an instance of
    ///           hpx::exception.
    ///
    /// \see    \a hpx::get_colocation_id()
    HPX_CXX_EXPORT HPX_EXPORT hpx::id_type get_colocation_id(
        launch::sync_policy, hpx::id_type const& id, error_code& ec = throws);

    /// \brief Asynchronously return the id of the locality where the object
    ///        referenced by the given id is currently located on
    ///
    /// \param id [in] The id of the object to locate.
    ///
    /// \see    \a hpx::get_colocation_id(launch::sync_policy)
    HPX_CXX_EXPORT HPX_EXPORT hpx::future<hpx::id_type> get_colocation_id(
        hpx::id_type const& id);
}    // namespace hpx
