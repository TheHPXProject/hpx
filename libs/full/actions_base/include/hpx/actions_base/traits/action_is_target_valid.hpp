//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/type_support.hpp>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // Customization point for Action::component_type::is_target_valid
    namespace detail {

        struct is_target_valid_helper
        {
            // by default, we return true if the given id is not referring to a
            // locality
            template <typename Action>
            static bool call(wrap_int, hpx::id_type const& id) noexcept
            {
                // All component types requires valid id for its actions to be
                // invoked (by default)
                return !naming::is_locality(id.get_gid());
            }

            // forward the call if the component implements the function
            template <typename Action>
            static auto call(int, hpx::id_type const& id) noexcept
                -> decltype(Action::component_type::is_target_valid(id))
            {
                // by default, we forward this to the component type
                using component_type = typename Action::component_type;
                return component_type::is_target_valid(id);
            }
        };
    }    // namespace detail

    HPX_CXX_EXPORT template <typename Action, typename Enable = void>
    struct action_is_target_valid
    {
        static bool call(hpx::id_type const& id) noexcept
        {
            return detail::is_target_valid_helper::template call<Action>(0, id);
        }
    };
}    // namespace hpx::traits
// namespace hpx::traits
