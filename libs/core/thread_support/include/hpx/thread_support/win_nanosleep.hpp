//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)
#include <ctime>

namespace hpx::util {

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT int win_nanosleep(
        std::timespec const& delay);

    HPX_CXX_CORE_EXPORT inline constexpr std::timespec wait_0ns = {.tv_sec = 0,
        .tv_nsec = 0};
    HPX_CXX_CORE_EXPORT inline constexpr std::timespec wait_1000ns = {
        .tv_sec = 0,
        .tv_nsec = 1000};
}    // namespace hpx::util

#endif
