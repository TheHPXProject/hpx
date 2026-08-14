//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file get_num_localities.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <cstdint>

namespace hpx {

    /// \brief Asynchronously return the number of localities which are
    ///        currently registered for the running application.
    ///
    /// The function \a get_num_localities asynchronously returns the
    /// number of localities currently connected to the console which support
    /// the creation of the given component type. The returned future represents
    /// the actual result.
    ///
    /// \param t  The component type for which the number of connected
    ///           localities should be retrieved.
    ///
    /// \note     This function will return meaningful results only if called
    ///           from an HPX-thread. It will return 0 otherwise.
    ///
    /// \see      \a hpx::find_all_localities, \a hpx::get_num_localities
    HPX_CXX_EXPORT HPX_EXPORT hpx::future<std::uint32_t> get_num_localities(
        components::component_type t);

    /// \brief Synchronously return the number of localities which are
    ///        currently registered for the running application.
    ///
    /// The function \a get_num_localities returns the number of localities
    /// currently connected to the console which support the creation of the
    /// given component type. The returned future represents the actual result.
    ///
    /// \param t  The component type for which the number of connected
    ///           localities should be retrieved.
    /// \param ec [in,out] this represents the error status on exit, if this
    ///           is pre-initialized to \a hpx#throws the function will throw
    ///           on error instead.
    ///
    /// \note     This function will return meaningful results only if called
    ///           from an HPX-thread. It will return 0 otherwise.
    ///
    /// \see      \a hpx::find_all_localities, \a hpx::get_num_localities
    HPX_CXX_EXPORT HPX_EXPORT std::uint32_t get_num_localities(
        launch::sync_policy, components::component_type t,
        error_code& ec = throws);
}    // namespace hpx
