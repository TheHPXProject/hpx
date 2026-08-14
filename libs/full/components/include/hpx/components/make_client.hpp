//  Copyright (c) 2017-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Create client objects from id_type, future<id_type>, etc.
namespace hpx::components {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Client>
        requires(traits::is_client_v<Client>)
    Client make_client(hpx::id_type const& id)
    {
        return Client(id);
    }

    HPX_CXX_EXPORT template <typename Client>
        requires(traits::is_client_v<Client>)
    Client make_client(hpx::id_type&& id)
    {
        return Client(HPX_MOVE(id));
    }

    HPX_CXX_EXPORT template <typename Client>
        requires(traits::is_client_v<Client>)
    Client make_client(hpx::future<hpx::id_type>&& id) noexcept
    {
        return Client(HPX_MOVE(id));
    }

    HPX_CXX_EXPORT template <typename Client>
        requires(traits::is_client_v<Client>)
    Client make_client(hpx::shared_future<hpx::id_type> const& id) noexcept
    {
        return Client(id);
    }

    HPX_CXX_EXPORT template <typename Client>
        requires(traits::is_client_v<Client>)
    Client make_client(hpx::shared_future<hpx::id_type>&& id) noexcept
    {
        return Client(HPX_MOVE(id));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Client>
        requires(traits::is_client_v<Client>)
    std::vector<Client> make_clients(std::vector<hpx::id_type> const& ids)
    {
        std::vector<Client> result;
        result.reserve(ids.size());
        for (hpx::id_type const& id : ids)
        {
            result.push_back(Client(id));
        }
        return result;
    }

    // this is broken at least up until CUDA V11.5
#if !defined(HPX_CUDA_VERSION)
    HPX_CXX_EXPORT template <typename Client>
        requires(traits::is_client_v<Client>)
    std::vector<Client> make_clients(std::vector<hpx::id_type>&& ids)
    {
        std::vector<Client> result;
        result.reserve(ids.size());
        for (hpx::id_type& id : HPX_MOVE(ids))
        {
            result.push_back(Client(HPX_MOVE(id)));
        }
        return result;
    }
#endif

    HPX_CXX_EXPORT template <typename Client>
        requires(traits::is_client_v<Client>)
    std::vector<Client> make_clients(
        std::vector<hpx::future<hpx::id_type>> const& ids)
    {
        std::vector<Client> result;
        result.reserve(ids.size());
        for (hpx::future<hpx::id_type> const& id : ids)
        {
            result.push_back(Client(id));
        }
        return result;
    }

    // this is broken at least up until CUDA V11.5
#if !defined(HPX_CUDA_VERSION)
    HPX_CXX_EXPORT template <typename Client>
        requires(traits::is_client_v<Client>)
    std::vector<Client> make_clients(
        std::vector<hpx::future<hpx::id_type>>&& ids)
    {
        std::vector<Client> result;
        result.reserve(ids.size());
        for (hpx::future<hpx::id_type>& id : HPX_MOVE(ids))
        {
            result.push_back(Client(HPX_MOVE(id)));
        }
        return result;
    }
#endif

    HPX_CXX_EXPORT template <typename Client>
        requires(traits::is_client_v<Client>)
    std::vector<Client> make_clients(
        std::vector<hpx::shared_future<hpx::id_type>> const& ids)
    {
        std::vector<Client> result;
        result.reserve(ids.size());
        for (hpx::shared_future<hpx::id_type> const& id : ids)
        {
            result.push_back(Client(id));
        }
        return result;
    }

    // this is broken at least up until CUDA V11.5
#if !defined(HPX_CUDA_VERSION)
    HPX_CXX_EXPORT template <typename Client>
        requires(traits::is_client_v<Client>)
    std::vector<Client> make_clients(
        std::vector<hpx::shared_future<hpx::id_type>>&& ids)
    {
        std::vector<Client> result;
        result.reserve(ids.size());
        for (hpx::shared_future<hpx::id_type>& id : HPX_MOVE(ids))
        {
            result.push_back(Client(HPX_MOVE(id)));
        }
        return result;
    }
#endif
}    // namespace hpx::components
