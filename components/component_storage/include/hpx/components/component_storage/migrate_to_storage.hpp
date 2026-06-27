//  Copyright (c) 2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file migrate_to_storage.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/components/component_storage/component_storage.hpp>
#include <hpx/components/component_storage/server/migrate_to_storage.hpp>

#include <type_traits>

namespace hpx { namespace components {
    /// Migrate the component with the given id to the specified target storage
    ///
    /// The function \a migrate_to_storage<Component> will migrate the component
    /// referenced by \a to_migrate to the storage facility specified with
    /// \a target_storage. It returns a future referring to the migrated
    /// component instance.
    ///
    /// \param to_migrate      [in] The global id of the component to migrate.
    /// \param target_storage  [in] The id of the storage facility to migrate
    ///                        this object to.
    ///
    /// \tparam  The only template argument specifies the component type of the
    ///          component to migrate to the given storage facility.
    ///
    /// \returns A future representing the global id of the migrated
    ///          component instance. This should be the same as \a migrate_to.
    ///
    template <typename Component>
#if defined(DOXYGEN)
    future<hpx::id_type>
#else
    inline typename std::enable_if<traits::is_component<Component>::value,
        future<hpx::id_type>>::type
#endif
    migrate_to_storage(
        hpx::id_type const& to_migrate, hpx::id_type const& target_storage)
    {
        typedef server::trigger_migrate_to_storage_here_action<Component>
            action_type;
        return async<action_type>(naming::get_locality_from_id(to_migrate),
            to_migrate, target_storage);
    }

    /// Migrate the given component to the specified target storage
    ///
    /// The function \a migrate_to_storage will migrate the component
    /// referenced by \a to_migrate to the storage facility specified with
    /// \a target_storage. It returns a future referring to the migrated
    /// component instance.
    ///
    /// \param to_migrate      [in] The client side representation of the
    ///                        component to migrate.
    /// \param target_storage  [in] The id of the storage facility to migrate
    ///                        this object to.
    ///
    /// \returns A client side representation of representing of the migrated
    ///          component instance. This should be the same as \a migrate_to.
    ///
    template <typename Derived, typename Stub, typename Data>
    inline Derived migrate_to_storage(
        client_base<Derived, Stub, Data> const& to_migrate,
        hpx::components::component_storage const& target_storage)
    {
        using component_type =
            typename client_base<Derived, Stub, Data>::server_component_type;
        return Derived(migrate_to_storage<component_type>(
            to_migrate.get_id(), target_storage.get_id()));
    }
}}    // namespace hpx::components
