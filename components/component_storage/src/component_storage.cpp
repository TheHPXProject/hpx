//  Copyright (c) 2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/components/component_storage/component_storage.hpp>

#include <cstddef>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx { namespace components {
    component_storage::component_storage(hpx::id_type target_locality)
      : base_type(hpx::new_<server::component_storage>(target_locality))
    {
    }

    component_storage::component_storage(hpx::future<hpx::id_type>&& f)
      : base_type(HPX_MOVE(f))
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<hpx::id_type> component_storage::migrate_to_here(
        std::vector<char> const& data, hpx::id_type const& id,
        naming::address const& addr)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        typedef server::component_storage::migrate_to_here_action action_type;
        return hpx::async<action_type>(this->get_id(), data, id, addr);
#else
        HPX_ASSERT(false);
        HPX_UNUSED(data);
        HPX_UNUSED(id);
        HPX_UNUSED(addr);
        return hpx::make_ready_future(hpx::id_type{});
#endif
    }

    hpx::id_type component_storage::migrate_to_here(launch::sync_policy,
        std::vector<char> const& data, hpx::id_type const& id,
        naming::address const& addr)
    {
        return migrate_to_here(data, id, addr).get();
    }

    hpx::future<std::vector<char>> component_storage::migrate_from_here(
        naming::gid_type const& id)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        typedef server::component_storage::migrate_from_here_action action_type;
        return hpx::async<action_type>(this->get_id(), id);
#else
        HPX_ASSERT(false);
        HPX_UNUSED(id);
        return hpx::make_ready_future(std::vector<char>{});
#endif
    }

    std::vector<char> component_storage::migrate_from_here(
        launch::sync_policy, naming::gid_type const& id)
    {
        return migrate_from_here(id).get();
    }

    hpx::future<std::size_t> component_storage::size() const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        typedef server::component_storage::size_action action_type;
        return hpx::async<action_type>(this->get_id());
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future(std::size_t{});
#endif
    }

    std::size_t component_storage::size(launch::sync_policy) const
    {
        return size().get();
    }
}}    // namespace hpx::components
