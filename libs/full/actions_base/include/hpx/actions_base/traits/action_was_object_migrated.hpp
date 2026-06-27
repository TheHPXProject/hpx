//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/naming_base.hpp>

#include <type_traits>
#include <utility>

namespace hpx::traits {

    namespace detail {

        HPX_HAS_MEMBER_XXX_TRAIT_DEF(supports_migration)

        template <typename Component, typename Enable = void>
        struct does_support_migration : std::false_type
        {
        };

        template <typename Component>
        struct does_support_migration<Component,
            std::enable_if_t<has_supports_migration_v<Component>>>
          : std::bool_constant<Component::supports_migration()>
        {
        };

        template <typename Component>
        inline constexpr bool does_support_migration_v =
            does_support_migration<Component>::value;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Customization point for action capabilities
    HPX_CXX_EXPORT template <typename Action, typename Enable = void>
    struct action_was_object_migrated
    {
        // returns whether target was migrated to another locality
        static std::pair<bool, components::pinned_ptr> call(
            [[maybe_unused]] hpx::naming::gid_type const& id,
            [[maybe_unused]] naming::address_type lva)
        {
            if constexpr (detail::does_support_migration_v<
                              typename Action::component_type>)
            {
                using component_type = typename Action::component_type;
                auto result = component_type::was_object_migrated(id, lva);
                HPX_ASSERT((result.first || result.second) &&
                    !(result.first && result.second));
                return result;
            }
            else
            {
                return std::make_pair(false, components::pinned_ptr());
            }
        }

        static std::pair<bool, components::pinned_ptr> call(
            [[maybe_unused]] hpx::id_type const& id,
            [[maybe_unused]] naming::address_type lva)
        {
            if constexpr (detail::does_support_migration_v<
                              typename Action::component_type>)
            {
                using component_type = typename Action::component_type;
                auto result =
                    component_type::was_object_migrated(id.get_gid(), lva);
                HPX_ASSERT((result.first || result.second) &&
                    !(result.first && result.second));
                return result;
            }
            else
            {
                return std::make_pair(false, components::pinned_ptr());
            }
        }
    };
}    // namespace hpx::traits
