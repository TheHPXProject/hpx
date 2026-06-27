//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // Customization point for component capabilities
    namespace detail {

        struct supports_migration_helper
        {
            // by default, we return 'false' (component does not support
            // migration)
            template <typename Component>
            static constexpr bool call(wrap_int) noexcept
            {
                return false;
            }

            // forward the call if the component implements the function
            template <typename Component>
            static constexpr auto call(int) noexcept
                -> decltype(Component::supports_migration())
            {
                return Component::supports_migration();
            }
        };

        template <typename Component>
        constexpr bool call_supports_migration() noexcept
        {
            return supports_migration_helper::template call<Component>(0);
        }
    }    // namespace detail

    HPX_CXX_EXPORT template <typename Component, typename Enable = void>
    struct component_supports_migration
    {
        // returns whether target supports migration
        static constexpr bool call() noexcept
        {
            return detail::call_supports_migration<Component>();
        }
    };
}    // namespace hpx::traits
