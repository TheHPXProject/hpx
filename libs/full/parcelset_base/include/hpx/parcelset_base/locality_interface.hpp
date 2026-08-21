//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/errors.hpp>

#include <hpx/parcelset_base/parcelset_base_fwd.hpp>

#include <cstdint>
#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset {

    HPX_CXX_EXPORT HPX_EXPORT parcelset::parcel create_parcel();

    HPX_CXX_EXPORT HPX_EXPORT locality create_locality(std::string const& name);

    HPX_CXX_EXPORT HPX_EXPORT bool locality_was_disconnected(std::uint32_t id);

    ///////////////////////////////////////////////////////////////////////////
    // initialize locality interface function wrappers
    HPX_CXX_EXPORT struct locality_interface_functions& locality_init();
}    // namespace hpx::parcelset

#endif
