//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <type_traits>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_EXPORT struct fixed_component_tag
        {
        };

        HPX_CXX_EXPORT struct component_tag
        {
        };

        HPX_CXX_EXPORT struct managed_component_tag
        {
        };

        HPX_CXX_EXPORT using simple_component_tag = component_tag;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component, typename Enable = void>
    struct is_component : std::false_type
    {
    };

    template <typename Component>
    struct is_component<Component const> : is_component<Component>
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    // Simple components are components
    template <typename Component>
    struct is_component<Component,
        std::enable_if_t<std::is_base_of_v<detail::component_tag, Component>>>
      : std::true_type
    {
    };

    // Fixed components are components
    template <typename Component>
    struct is_component<Component,
        std::enable_if_t<
            std::is_base_of_v<detail::fixed_component_tag, Component>>>
      : std::true_type
    {
    };

    // Managed components are components
    template <typename Component>
    struct is_component<Component,
        std::enable_if_t<
            std::is_base_of_v<detail::managed_component_tag, Component>>>
      : std::true_type
    {
    };

    HPX_CXX_EXPORT template <typename Component>
    inline constexpr bool is_component_v = is_component<Component>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T, typename Enable = void>
    struct is_component_or_component_array : is_component<T>
    {
    };

    template <typename T>
    struct is_component_or_component_array<T[]> : is_component<T>
    {
    };

    template <typename T, std::size_t N>
    struct is_component_or_component_array<T[N]> : is_component<T>
    {
    };

    HPX_CXX_EXPORT template <typename Component>
    inline constexpr bool is_component_or_component_array_v =
        is_component_or_component_array<Component>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component>
    struct is_fixed_component
      : std::is_base_of<traits::detail::fixed_component_tag, Component>
    {
    };

    template <typename Component>
    struct is_fixed_component<Component const> : is_fixed_component<Component>
    {
    };

    HPX_CXX_EXPORT template <typename Component>
    inline constexpr bool is_fixed_component_v =
        is_fixed_component<Component>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component>
    struct is_managed_component
      : std::is_base_of<traits::detail::managed_component_tag, Component>
    {
    };

    template <typename Component>
    struct is_managed_component<Component const>
      : is_managed_component<Component>
    {
    };

    HPX_CXX_EXPORT template <typename Component>
    inline constexpr bool is_managed_component_v =
        is_managed_component<Component>::value;

}    // namespace hpx::traits
