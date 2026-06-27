//  Copyright (c) 2023-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <utility>

namespace hpx::traits {

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_pair_v = false;

    template <typename T1, typename T2>
    inline constexpr bool is_pair_v<std::pair<T1, T2>> = true;
}    // namespace hpx::traits
