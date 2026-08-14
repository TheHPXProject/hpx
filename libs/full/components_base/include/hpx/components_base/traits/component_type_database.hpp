//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstdint>

namespace hpx::components {

    HPX_CXX_EXPORT using component_type = std::int32_t;
}    // namespace hpx::components

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component, typename Enable = void>
    struct component_type_database
    {
        // components::component_invalid is defined as -1
        inline static components::component_type value = -1;

        HPX_ALWAYS_EXPORT static components::component_type get() noexcept;
        HPX_ALWAYS_EXPORT static void set(components::component_type);
    };

    template <typename Component, typename Enable>
    struct component_type_database<Component const, Enable>
      : component_type_database<Component, Enable>
    {
    };
}    // namespace hpx::traits
