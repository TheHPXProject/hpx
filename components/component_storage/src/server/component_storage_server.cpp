//  Copyright (c) 2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/runtime_distributed.hpp>

#include <hpx/components/component_storage/server/component_storage.hpp>

#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx { namespace components { namespace server {
    component_storage::component_storage()
      : data_(container_layout(find_all_localities()))
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    naming::gid_type component_storage::migrate_to_here(
        std::vector<char> const& data, hpx::id_type id,
        naming::address const& current_lva)
    {
        naming::gid_type gid(naming::detail::get_stripped_gid(id.get_gid()));
        data_[gid] = data;

        // rebind the object to this storage locality
        naming::address addr(current_lva);
        addr.address_ = nullptr;    // invalidate lva
        if (!agas::bind(launch::sync, gid, addr, this->gid_))
        {
            HPX_THROW_EXCEPTION(hpx::error::duplicate_component_address,
                "component_storage::migrate_to_here",
                "failed to rebind id {} to storage locality: {}", id, gid_);
            return naming::invalid_gid;
        }

        id.make_unmanaged();    // we can now release the object
        return naming::invalid_gid;
    }

    std::vector<char> component_storage::migrate_from_here(
        naming::gid_type const& id)
    {
        // return the stored data and erase it from the map
        return data_.get_value(
            launch::sync, naming::detail::get_stripped_gid(id), true);
    }
}}}    // namespace hpx::components::server

HPX_REGISTER_UNORDERED_MAP(
    hpx::naming::gid_type, hpx_component_storage_data_type)
