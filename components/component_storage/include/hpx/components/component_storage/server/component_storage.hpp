//  Copyright (c) 2007-2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/components/component_storage/export_definitions.hpp>
#include <hpx/components/containers/unordered/unordered_map.hpp>

#include <cstddef>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components::server {

    ///////////////////////////////////////////////////////////////////////////
    class HPX_MIGRATE_TO_STORAGE_EXPORT component_storage
      : public component_base<component_storage>
    {
        typedef hpx::spinlock mutex_type;

    public:
        component_storage();

        naming::gid_type migrate_to_here(
            std::vector<char> const&, hpx::id_type, naming::address const&);
        std::vector<char> migrate_from_here(naming::gid_type const&);
        std::size_t size() const
        {
            return data_.size();
        }

        HPX_DEFINE_COMPONENT_ACTION(component_storage, migrate_to_here)
        HPX_DEFINE_COMPONENT_ACTION(component_storage, migrate_from_here)
        HPX_DEFINE_COMPONENT_ACTION(component_storage, size)

    private:
        hpx::unordered_map<naming::gid_type, std::vector<char>> data_;
    };
}    // namespace hpx::components::server

#include <hpx/config/warnings_suffix.hpp>

HPX_REGISTER_ACTION_DECLARATION(
    hpx::components::server::component_storage::migrate_to_here_action,
    component_storage_migrate_component_to_here_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::components::server::component_storage::migrate_from_here_action,
    component_storage_migrate_component_from_here_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::components::server::component_storage::size_action,
    component_storage_size_action)

typedef std::vector<char> hpx_component_storage_data_type;
HPX_REGISTER_UNORDERED_MAP_DECLARATION(
    hpx::naming::gid_type, hpx_component_storage_data_type)
