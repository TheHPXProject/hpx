//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file copy_component.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/runtime_distributed/server/copy_component.hpp>

#include <type_traits>

namespace hpx::components {

    /// \brief Copy given component to the specified target locality
    ///
    /// The function \a copy<Component> will create a copy of the component
    /// referenced by \a to_copy on the locality specified with
    /// \a target_locality. It returns a future referring to the newly created
    /// component instance.
    ///
    /// \param to_copy         [in] The global id of the component to copy
    ///
    /// \tparam  The only template argument specifies the component type to
    ///          create.
    ///
    /// \returns A future representing the global id of the newly (copied)
    ///          component instance.
    ///
    /// \note The new component instance is created on the locality of the
    ///       component instance which is to be copied.
    ///
    template <typename Component>
#if defined(DOXYGEN)
    future<hpx::id_type>
#else
    std::enable_if_t<traits::is_component_v<Component>, future<hpx::id_type>>
#endif
    copy(hpx::id_type const& to_copy)
    {
        using action_type = server::copy_component_action_here<Component>;
        return hpx::detail::async_colocated<action_type>(to_copy, to_copy);
    }

    /// \brief Copy given component to the specified target locality
    ///
    /// The function \a copy<Component> will create a copy of the component
    /// referenced by \a to_copy on the locality specified with
    /// \a target_locality. It returns a future referring to the newly created
    /// component instance.
    ///
    /// \param to_copy         [in] The global id of the component to copy
    /// \param target_locality [in ] The locality where the copy
    ///                        should be created.
    ///
    /// \tparam  The only template argument specifies the component type to
    ///          create.
    ///
    /// \returns A future representing the global id of the newly (copied)
    ///          component instance.
    ///
    template <typename Component>
#if defined(DOXYGEN)
    future<hpx::id_type>
#else
    std::enable_if_t<traits::is_component_v<Component>, future<hpx::id_type>>
#endif
    copy(hpx::id_type const& to_copy, hpx::id_type const& target_locality)
    {
        using action_type = server::copy_component_action<Component>;
        return hpx::detail::async_colocated<action_type>(
            to_copy, to_copy, target_locality);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Copy given component to the specified target locality
    ///
    /// The function \a copy will create a copy of the component
    /// referenced by the client side object \a to_copy on the locality
    /// specified with \a target_locality. It returns a new client side object
    /// future referring to the newly created component instance.
    ///
    /// \param to_copy         [in] The client side object representing the
    ///                        component to copy
    /// \param target_locality [in, optional] The locality where the copy
    ///                        should be created (default is same locality
    ///                        as source).
    ///
    /// \tparam  The only template argument specifies the component type to
    ///          create.
    ///
    /// \returns A future representing the global id of the newly (copied)
    ///          component instance.
    ///
    /// \note If the second argument is omitted (or is invalid_id) the
    ///       new component instance is created on the locality of the
    ///       component instance which is to be copied.
    ///
    template <typename Derived, typename Stub, typename Data>
    Derived copy(client_base<Derived, Stub, Data> const& to_copy,
        hpx::id_type const& target_locality = hpx::invalid_id)
    {
        using component_type =
            typename client_base<Derived, Stub, Data>::server_component_type;
        using action_type = server::copy_component_action<component_type>;

        return Derived(hpx::detail::async_colocated<action_type>(
            to_copy, to_copy, target_locality));
    }
}    // namespace hpx::components
