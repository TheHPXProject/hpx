//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file components_base_fwd.hpp
/// \page hpx::components::component, hpx::components::component_base
/// \headerfile hpx/components.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/traits/managed_component_policies.hpp>

namespace hpx::components {

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT class pinned_ptr;

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component>
    class fixed_component;

    /// The \a component class wraps around a given component type, adding
    /// additional type aliases and constructors. It inherits from the
    /// specified component type.
    HPX_CXX_EXPORT template <typename Component>
    class component;

    HPX_CXX_EXPORT template <typename Component, typename Derived = void>
    class managed_component;

    ///////////////////////////////////////////////////////////////////////
    /// \a component_base serves as a base class for components. It provides
    /// common functionality needed by components, such as address and ID
    /// retrieval. The template parameter \a Component specifies the
    /// derived component type.
    HPX_CXX_EXPORT template <typename Component = void>
    class component_base;

    HPX_CXX_EXPORT template <typename Component = void>
    class fixed_component_base;

    HPX_CXX_EXPORT template <typename Component = void>
    class abstract_component_base;

    HPX_CXX_EXPORT template <typename Component, typename Derived = void>
    class abstract_managed_component_base;

    HPX_CXX_EXPORT template <typename Component, typename Wrapper = void,
        typename CtorPolicy = traits::construct_without_back_ptr,
        typename DtorPolicy = traits::managed_object_controls_lifetime>
    class managed_component_base;
}    // namespace hpx::components
