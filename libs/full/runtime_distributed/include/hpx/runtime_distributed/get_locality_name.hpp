//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/runtime_distributed/get_locality_name.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <string>

namespace hpx {

    /// \fn future<std::string> get_locality_name(hpx::id_type const& id)
    ///
    /// \brief Return the name of the referenced locality.
    ///
    /// This function returns a future referring to the name for the locality
    /// of the given id.
    ///
    /// \param id [in] The global id of the locality for which the name should
    ///           be retrieved
    ///
    /// \returns  This function returns the name for the locality of the given
    ///           id. The name is retrieved from the underlying networking layer
    ///           and may be different for different parcel ports.
    ///
    /// \see      \a std::string get_locality_name()
    HPX_CXX_EXPORT HPX_EXPORT future<std::string> get_locality_name(
        hpx::id_type const& id);
}    // namespace hpx
