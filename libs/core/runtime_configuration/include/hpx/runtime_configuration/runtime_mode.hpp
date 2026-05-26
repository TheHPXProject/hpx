//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file runtime_mode.hpp
/// \page hpx::runtime_mode
/// \headerfile hpx/init.hpp

#pragma once

#include <hpx/config.hpp>
#include <cstdint>
#include <string>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    /// A HPX runtime can be executed in two different modes: console mode
    /// and worker mode.
    HPX_CXX_CORE_EXPORT enum class runtime_mode : std::int8_t {
        invalid = -1,
        console = 0,     ///< The runtime is the console locality
        worker = 1,      ///< The runtime is a worker locality
        connect = 2,     ///< The runtime is a worker locality
                         ///< connecting late
        local = 3,       ///< The runtime is fully local
        default_ = 4,    ///< The runtime mode will be determined
                         ///< based on the command line arguments
        last = default_
    };

    /// Get the readable string representing the name of the given runtime_mode
    /// constant.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* get_runtime_mode_name(
        runtime_mode state) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Returns the internal representation (runtime_mode constant) from
    /// the readable string representing the name.
    ///
    /// This represents the internal representation from the readable string
    /// representing the name.
    ///
    /// \param mode this represents the runtime mode
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT runtime_mode get_runtime_mode_from_name(
        std::string const& mode);
}    // namespace hpx
