//  Copyright (c) 2019 Thomas Heller
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file source_location.hpp
/// \page HPX_CURRENT_SOURCE_LOCATION, hpx::source_location
/// \headerfile hpx/source_location.hpp

#pragma once

#include <hpx/config/export_definitions.hpp>

#include <iosfwd>
#include <source_location>

namespace hpx {

    /// This contains the location information where \a HPX_ASSERT has been
    /// called
    HPX_CXX_CORE_EXPORT using std::source_location;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ostream& operator<<(
        std::ostream& os, source_location const& loc);
}    // namespace hpx
