//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // control the way managed_components are constructed
    HPX_CXX_EXPORT struct construct_with_back_ptr
    {
    };

    HPX_CXX_EXPORT struct construct_without_back_ptr
    {
    };

    HPX_CXX_EXPORT template <typename T, typename Enable = void>
    struct managed_component_ctor_policy
    {
        using type = construct_without_back_ptr;
    };

    template <typename Component>
    struct managed_component_ctor_policy<Component,
        std::void_t<typename Component::has_managed_component_base>>
    {
        using type = typename Component::ctor_policy;
    };

    HPX_CXX_EXPORT template <typename T>
    using managed_component_ctor_policy_t =
        typename managed_component_ctor_policy<T>::type;

    ///////////////////////////////////////////////////////////////////////////
    // control the way managed_components are destructed
    HPX_CXX_EXPORT struct managed_object_is_lifetime_controlled
    {
    };

    HPX_CXX_EXPORT struct managed_object_controls_lifetime
    {
    };

    HPX_CXX_EXPORT template <typename T, typename Enable = void>
    struct managed_component_dtor_policy
    {
        using type = managed_object_controls_lifetime;
    };

    template <typename Component>
    struct managed_component_dtor_policy<Component,
        std::void_t<typename Component::has_managed_component_base>>
    {
        using type = typename Component::dtor_policy;
    };

    HPX_CXX_EXPORT template <typename T>
    using managed_component_dtor_policy_t =
        typename managed_component_dtor_policy<T>::type;
}    // namespace hpx::traits
