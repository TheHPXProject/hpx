//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components/basename_registration_fwd.hpp>
#include <hpx/components/make_client.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/futures.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    /// Return all registered clients from all localities from the given base
    /// name.
    ///
    /// This function locates all ids which were registered with the given
    /// base name. It returns a list of futures representing those ids.
    ///
    /// \tparam Client     The client type to return
    ///
    /// \param base_name    [in] The base name for which to retrieve the
    ///                     registered ids.
    /// \param num_ids      [in] The number of registered ids to expect.
    ///
    /// \returns A list of futures representing the ids which were registered
    ///          using the given base name.
    ///
    /// \note   The futures embedded in the returned client objects will become
    ///         ready even if the event (for instance,
    ///         binding the name to an id) has already happened in the past.
    ///         This is important in order to reliably retrieve ids from a
    ///         name, even if the name was already registered.
    ///
    HPX_CXX_EXPORT template <typename Client>
    std::vector<Client> find_all_from_basename(
        std::string base_name, std::size_t num_ids)
    {
        return components::make_clients<Client>(
            find_all_from_basename(HPX_MOVE(base_name), num_ids));
    }

    HPX_CXX_EXPORT template <typename Client>
    std::vector<Client> find_all_from_basename(hpx::launch::sync_policy policy,
        std::string base_name, std::size_t num_ids)
    {
        return components::make_clients<Client>(
            find_all_from_basename(policy, HPX_MOVE(base_name), num_ids));
    }

    /// Return registered clients from the given base name and sequence numbers.
    ///
    /// This function locates the ids which were registered with the given
    /// base name and the given sequence numbers. It returns a list of futures
    /// representing those ids.
    ///
    /// \tparam Client     The client type to return
    ///
    /// \param base_name    [in] The base name for which to retrieve the
    ///                     registered ids.
    /// \param ids          [in] The sequence numbers of the registered ids.
    ///
    /// \returns A list of futures representing the ids which were registered
    ///          using the given base name and sequence numbers.
    ///
    /// \note   The futures embedded in the returned client objects will become
    ///         ready even if the event (for instance,
    ///         binding the name to an id) has already happened in the past.
    ///         This is important in order to reliably retrieve ids from a
    ///         name, even if the name was already registered.
    ///
    HPX_CXX_EXPORT template <typename Client>
    std::vector<Client> find_from_basename(
        std::string base_name, std::vector<std::size_t> const& ids)
    {
        return components::make_clients<Client>(
            find_from_basename(HPX_MOVE(base_name), ids));
    }

    HPX_CXX_EXPORT template <typename Client>
    std::vector<Client> find_from_basename(hpx::launch::sync_policy policy,
        std::string base_name, std::vector<std::size_t> const& ids)
    {
        return components::make_clients<Client>(
            find_from_basename(policy, HPX_MOVE(base_name), ids));
    }

    /// \brief Return registered id from the given base name and sequence number.
    ///
    /// This function locates the id which was registered with the given
    /// base name and the given sequence number. It returns a future
    /// representing those id.
    ///
    /// \tparam Client     The client type to return
    ///
    /// \param base_name    [in] The base name for which to retrieve the
    ///                     registered ids.
    /// \param sequence_nr  [in] The sequence number of the registered id.
    ///
    /// \returns A representing the id which was registered using the given
    ///          base name and sequence numbers.
    ///
    /// \note   The future embedded in the returned client object will become
    ///         ready even if the event (for instance,
    ///         binding the name to an id) has already happened in the past.
    ///         This is important in order to reliably retrieve ids from a
    ///         name, even if the name was already registered.
    ///
    HPX_CXX_EXPORT template <typename Client>
    Client find_from_basename(std::string base_name, std::size_t sequence_nr)
    {
        return components::make_client<Client>(
            find_from_basename(HPX_MOVE(base_name), sequence_nr));
    }

    HPX_CXX_EXPORT template <typename Client>
    Client find_from_basename(hpx::launch::sync_policy policy,
        std::string base_name, std::size_t sequence_nr)
    {
        return components::make_client<Client>(
            find_from_basename(policy, HPX_MOVE(base_name), sequence_nr));
    }

    /// Register the id wrapped in the given client using the given base name.
    ///
    /// The function registers the object the given client refers to using the
    /// provided base name.
    ///
    /// \tparam Client      The client type to register
    ///
    /// \param base_name    [in] The base name for which to retrieve the
    ///                     registered ids.
    /// \param client       [in] The client which should be registered using
    ///                     the given base name.
    /// \param sequence_nr  [in, optional] The sequential number to use for the
    ///                     registration of the id. This number has to be
    ///                     unique system-wide  for each registration using the
    ///                     same base name. The default is the current locality
    ///                     identifier. Also, the sequence numbers have to be
    ///                     consecutive starting from zero.
    ///
    /// \returns A future representing the result of the registration operation
    ///          itself.
    ///
    /// \note    The operation will fail if the given sequence number is not
    ///          unique.
    ///
    HPX_CXX_EXPORT template <typename Client, typename Stub, typename Data>
    hpx::future<bool> register_with_basename(std::string base_name,
        components::client_base<Client, Stub, Data>& client,
        std::size_t sequence_nr)
    {
        // clang-format off
        return client.then(
            [sequence_nr, base_name = HPX_MOVE(base_name)](
                components::client_base<Client, Stub, Data>&& c) mutable
                -> hpx::future<bool> {
                return register_with_basename(
                    HPX_MOVE(base_name), c.get_id(), sequence_nr);
            });
        // clang-format on
    }

    /// Unregister the given base name.
    ///
    /// The function unregisters the given ids using the provided base name.
    ///
    /// \tparam Client      The client type to return
    ///
    /// \param base_name    [in] The base name for which to retrieve the
    ///                     registered ids.
    /// \param sequence_nr  [in, optional] The sequential number to use for the
    ///                     un-registration. This number has to be the same
    ///                     as has been used with \a register_with_basename
    ///                     before.
    ///
    /// \returns A future representing the result of the un-registration
    ///          operation itself.
    ///
    HPX_CXX_EXPORT template <typename Client>
    Client unregister_with_basename(
        std::string base_name, std::size_t sequence_nr)
    {
        return components::make_client<Client>(
            unregister_with_basename(HPX_MOVE(base_name), sequence_nr));
    }
}    // namespace hpx
