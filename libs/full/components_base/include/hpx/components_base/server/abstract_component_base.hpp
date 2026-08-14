//  Copyright (c) 2012 Bryce Adelstein-Lelbach
//  Copyright (c) 2013-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/component_type.hpp>
#include <hpx/components_base/components_base_fwd.hpp>
#include <hpx/components_base/server/component.hpp>
#include <hpx/components_base/traits/is_component.hpp>

namespace hpx::components {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component>
    // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
    class abstract_component_base : traits::detail::component_tag
    {
    public:
        using wrapping_type = component<Component>;
        using this_component_type = Component;
        using base_type_holder = Component;

        virtual ~abstract_component_base() = default;

        static constexpr component_type get_component_type() noexcept
        {
            return hpx::components::get_component_type<wrapping_type>();
        }

        static constexpr void set_component_type(component_type t)
        {
            hpx::components::set_component_type<wrapping_type>(t);
        }

        virtual naming::address get_current_address() const
        {
            return naming::address(
                naming::get_gid_from_locality_id(agas::get_locality_id()),
                components::get_component_type<Component>(),
                const_cast<Component*>(static_cast<Component const*>(this)));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component, typename Wrapper>
    // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
    class abstract_managed_component_base
      : private traits::detail::managed_component_tag
    {
    public:
        using wrapping_type = managed_component<Component, Wrapper>;
        using this_component_type = Component;
        using base_type_holder = Component;

        virtual ~abstract_managed_component_base() = default;

        static constexpr component_type get_component_type() noexcept
        {
            return hpx::components::get_component_type<wrapping_type>();
        }

        static constexpr void set_component_type(component_type t)
        {
            hpx::components::set_component_type<wrapping_type>(t);
        }

        virtual naming::address get_current_address() const
        {
            return naming::address(
                naming::get_gid_from_locality_id(agas::get_locality_id()),
                components::get_component_type<Component>(),
                const_cast<Component*>(static_cast<Component const*>(this)));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component>
    // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
    class abstract_fixed_component_base
      : private traits::detail::fixed_component_tag
    {
    public:
        using wrapping_type = fixed_component<Component>;
        using this_component_type = Component;
        using base_type_holder = Component;

        virtual ~abstract_fixed_component_base() = default;

        static constexpr component_type get_component_type() noexcept
        {
            return hpx::components::get_component_type<wrapping_type>();
        }

        static constexpr void set_component_type(component_type t)
        {
            hpx::components::set_component_type<wrapping_type>(t);
        }

        virtual naming::address get_current_address() const
        {
            return naming::address(
                naming::get_gid_from_locality_id(agas::get_locality_id()),
                components::get_component_type<Component>(),
                const_cast<Component*>(static_cast<Component const*>(this)));
        }
    };
}    // namespace hpx::components
