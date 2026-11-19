//  Copyright (c) 2026 Nithurshen
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>

namespace hpx::traits {

    /// Evaluates to true if the component type is registered by another
    /// module than the one being compiled. A library that registers a
    /// component specializes this trait for its consumers (see
    /// \a HPX_COMPONENT_REGISTERED_ELSEWHERE), which turns a second
    /// registration of the component into a compile time error.
    HPX_CXX_EXPORT template <typename Component, typename Enable = void>
    struct component_registered_elsewhere : std::false_type
    {
    };

    HPX_CXX_EXPORT template <typename Component>
    inline constexpr bool component_registered_elsewhere_v =
        component_registered_elsewhere<Component>::value;
}    // namespace hpx::traits
