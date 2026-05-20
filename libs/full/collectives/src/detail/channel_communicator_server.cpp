//  Copyright (c) 2020-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/assert.hpp>
#include <hpx/collectives/channel_communicator.hpp>
#include <hpx/collectives/detail/channel_communicator.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/runtime_components.hpp>

#include <cstddef>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
using channel_communicator_component = hpx::components::component<
    hpx::collectives::detail::channel_communicator_server>;

HPX_REGISTER_COMPONENT(channel_communicator_component)

///////////////////////////////////////////////////////////////////////////////
namespace hpx::collectives::detail {

    ///////////////////////////////////////////////////////////////////////////
    channel_communicator::channel_communicator(char const* basename,
        std::size_t num_sites, std::size_t this_site, client_type here)
      : this_site_(this_site)
      , clients_(find_all_from_basename<client_type>(basename, num_sites))
    {
        // replace reference to our own client (manages base-name registration)
        clients_[this_site] = HPX_MOVE(here);
    }

    channel_communicator::channel_communicator(hpx::launch::sync_policy policy,
        char const* basename, std::size_t num_sites, std::size_t this_site,
        client_type here)
      : this_site_(this_site)
      , clients_(
            find_all_from_basename<client_type>(policy, basename, num_sites))
    {
        // replace reference to our own client (manages base-name registration)
        clients_[this_site] = HPX_MOVE(here);
    }

    channel_communicator::~channel_communicator() = default;

}    // namespace hpx::collectives::detail

#endif    // !HPX_COMPUTE_DEVICE_CODE
