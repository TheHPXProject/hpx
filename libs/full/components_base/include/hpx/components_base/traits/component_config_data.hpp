//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // Customization point for component config data injection
    HPX_CXX_EXPORT template <typename Component, typename Enable = void>
    struct component_config_data
    {
        // by default no additional config data is injected into the factory
        static constexpr char const* call() noexcept
        {
            return nullptr;
        }
    };
}    // namespace hpx::traits
