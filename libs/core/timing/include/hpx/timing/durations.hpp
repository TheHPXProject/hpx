//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file durations.hpp
/// \page hpx::chrono::picoseconds
/// \headerfile hpx/chrono.hpp

#pragma once

#include <hpx/config.hpp>

#include <chrono>
#include <cstdint>
#include <ratio>

namespace hpx::chrono {

    /// A duration in picoseconds, for times below a nanosecond, for instance
    /// the run time of a loop body (see hpx::timed_task). std::chrono has
    /// the ratio (std::pico) but no alias for it.
    HPX_CXX_CORE_EXPORT using picoseconds =
        std::chrono::duration<std::int64_t, std::pico>;
}    // namespace hpx::chrono
