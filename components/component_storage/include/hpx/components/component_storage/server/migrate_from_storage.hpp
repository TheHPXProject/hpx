//  Copyright (c) 2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_distributed.hpp>

#include <hpx/components/component_storage/export_definitions.hpp>
#include <hpx/components/component_storage/server/component_storage.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace hpx::components::server {

    ///////////////////////////////////////////////////////////////////////////
    // Migrate given component from the specified storage component
    //
    // Object migration is performed from the storage in several steps:
    //
    // 1) The migration is triggered by invoking the
    //    trigger_migrate_from_storage_here_action on the locality which is
    //    responsible for managing the address resolution for the object which
    //    has to be migrated.
    // 2) The trigger_migrate_from_storage_here_action performs 3 steps:
    //    a) Invoke agas::begin_migration, which marks the global id in AGAS,
    //       deferring all address resolution requests until end_migration is
    //       called.
    //    b) Invoke the actual migration operation (see step 3)
    //    c) Invoke end_migration, which un-marks the global id and releases
    //       all pending address resolution requests. Those requests now return
    //       the new object location.
    // 3) The actual migration (component_storage::migrate_from_here_action)
    //    is executed on the storage facility where the object is currently
    //    stored. This involves several steps as well:
    //    a) Retrieve the byte stream representing the object from the storage
    //    b) Deserialize the byte stream to re-create the object. The newly
    //       recreated object is pinned immediately. The object is unpinned by
    //       the deleter associated with the shared pointer.
    //    c) Invoke the action runtime_support::migrate_component on the
    //       locality where the object has to be moved to. This passes
    //       along the shared pointer to the object and recreates the object
    //       on the target locality and updates the association of the object's
    //       global id with the new local virtual address in AGAS.
    //    d) Mark the old object (through the original shared pointer) as
    //       migrated which will delete it once the shared pointer goes out of
    //       scope.
    //
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename Component>
        future<hpx::id_type> migrate_from_storage_here_id(
            hpx::id_type const& target_locality,
            std::shared_ptr<Component> const& ptr,
            hpx::id_type const& to_resurrect)
        {
            // and resurrect it on the specified locality
            using hpx::components::runtime_support;
            return runtime_support::migrate_component_async<Component>(
                target_locality, ptr, to_resurrect)
                .then(launch::sync, [ptr, to_resurrect](future<id_type>&& f) {
                    ptr->mark_as_migrated();
                    return f.get();
                });
        }

        template <typename Component>
        future<hpx::id_type> migrate_from_storage_here_address(
            naming::address const& addr, std::shared_ptr<Component> const& ptr,
            hpx::id_type const& to_resurrect)
        {
            hpx::id_type id(
                addr.locality_, id_type::management_type::unmanaged);
            return migrate_from_storage_here_id(id, ptr, to_resurrect);
        }

        // convert the extracted data into a living component instance
        template <typename Component>
        future<hpx::id_type> migrate_from_storage_here(
            future<std::vector<char>>&& f, hpx::id_type const& to_resurrect,
            naming::address const& addr, hpx::id_type const& target_locality)
        {
            // recreate the object
            std::shared_ptr<Component> ptr;

            {
                std::vector<char> data = f.get();
                serialization::input_archive archive(
                    data, data.size(), nullptr);
                archive >> ptr;
            }

            // if target locality is not specified, use the address of the last
            // locality where the object was living before
            if (target_locality == hpx::invalid_id)
            {
                return migrate_from_storage_here_address<Component>(
                    addr, ptr, to_resurrect);
            }

            // otherwise directly refer to the locality where the object should
            // be resurrected
            return migrate_from_storage_here_id(
                target_locality, ptr, to_resurrect);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // This is executed on the locality responsible for managing the address
    // resolution for the given object.
    template <typename Component>
    future<hpx::id_type> trigger_migrate_from_storage_here(
        hpx::id_type const& to_resurrect, hpx::id_type const& target_locality)
    {
        if constexpr (!traits::component_supports_migration<Component>::call())
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::components::server::trigger_migrate_from_storage_here",
                "attempting to migrate an instance of a component which "
                "does not support migration");
        }

        if (naming::get_locality_id_from_id(to_resurrect) != get_locality_id())
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::components::server::trigger_migrate_from_storage_here",
                "this function has to be executed on the locality responsible "
                "for managing the address of the given object");
        }

        auto const r = agas::begin_migration(to_resurrect).get();

        // retrieve the data from the given storage
        using action_type =
            typename server::component_storage::migrate_from_here_action;

        return async<action_type>(r.first, to_resurrect.get_gid())
            .then(hpx::bind_back(&detail::migrate_from_storage_here<Component>,
                to_resurrect, r.second, target_locality))
            .then([to_resurrect](future<hpx::id_type>&& f) -> hpx::id_type {
                agas::end_migration(to_resurrect);
                return f.get();
            });
    }

    template <typename Component>
    struct trigger_migrate_from_storage_here_action
      : ::hpx::actions::action<future<hpx::id_type> (*)(
                                   hpx::id_type const&, hpx::id_type const&),
            &trigger_migrate_from_storage_here<Component>,
            trigger_migrate_from_storage_here_action<Component>>
    {
    };
}    // namespace hpx::components::server
