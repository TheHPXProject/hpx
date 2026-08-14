//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/pairwise_all_to_all_helpers.hpp
/// Direct site-to-site exchange for the all_to_all collective. The default
/// all_to_all routes every contribution through one communicator site, which
/// concentrates the whole exchange on a single locality. This variant sends
/// each row straight to the site it belongs to, trading two messages per site
/// for num_sites - 1 of them. That trade only pays for large rows, so the
/// caller decides which path to take.

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/collectives/argument_types.hpp>
#include <hpx/collectives/channel_communicator.hpp>
#include <hpx/collectives/detail/hierarchical_helpers.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::collectives::detail {

    ///////////////////////////////////////////////////////////////////////////
    // pairwise_type_bytes
    //
    // The same estimate, but derived from the element type alone.
    //
    // Every site has to choose the same exchange path. If one site went
    // direct while another waited on the routed path, the exchange would
    // simply never complete, so the choice may only rest on something all
    // sites are guaranteed to compute identically. A type is such a thing; the
    // contributed data is not, because nothing in the all_to_all contract
    // forces two sites to contribute rows of equal length.
    //
    // That is why an automatic decision uses this function and never inspects
    // the contributed rows. A caller who knows its rows are uniform can still
    // select the direct path deliberately by passing a threshold of 0 at every
    // site.
    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T>
    constexpr std::size_t pairwise_type_bytes() noexcept
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            return sizeof(T);
        }
        else
        {
            return 0;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // exchange_pairwise
    //
    // Decides between the direct and the routed exchange. A payload whose
    // size cannot be established never selects the direct path on its own,
    // and neither does an exchange with fewer than three sites: below that
    // there is no routing detour left to remove.
    //
    // The caller must pass a bytes_per_pair that every site computes
    // identically, and the same threshold at every site; see
    // pairwise_type_bytes for why.
    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT inline bool exchange_pairwise(std::size_t const num_sites,
        std::size_t const bytes_per_pair,
        pairwise_threshold_arg const threshold) noexcept
    {
        if (num_sites < 3)
        {
            return false;
        }

        if (threshold == 0)
        {
            return true;
        }

        return bytes_per_pair != 0 &&
            bytes_per_pair >= static_cast<std::size_t>(threshold);
    }

    ///////////////////////////////////////////////////////////////////////////
    // pairwise_all_to_all
    //
    // Exchanges one row per participating site over a channel communicator,
    // without a root site relaying the data.
    //
    // local_result must hold exactly num_sites rows, where row d is destined
    // for site d. The returned vector holds one row per source site, so entry
    // s is what site s contributed for this site. That is the same contract
    // the communicator-based all_to_all offers, which lets either path serve
    // the same call.
    //
    // Sends are staggered: site h starts at peer h + 1 rather than at peer 0,
    // so the sites do not all target the same peer at the same time. The
    // receives are staggered in the opposite direction for the same reason.
    //
    // The tag separates concurrent exchanges on one channel communicator, so
    // the caller must pass a distinct tag per generation.
    ///////////////////////////////////////////////////////////////////////////
    // The communicator type has to be spelled out: inside this namespace the
    // unqualified name resolves to the detail implementation class, not to the
    // public handle callers hold.
    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<T>> pairwise_all_to_all(
        hpx::collectives::channel_communicator comm,
        std::vector<T>&& local_result, std::size_t const num_sites,
        std::size_t const this_site, tag_arg const tag)
    {
        if (num_sites == 0)
        {
            return hpx::make_exceptional_future<std::vector<T>>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::detail::pairwise_all_to_all",
                    "the number of participating sites must not be zero"));
        }

        if (this_site >= num_sites)
        {
            return hpx::make_exceptional_future<std::vector<T>>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::detail::pairwise_all_to_all",
                    "the local site must be smaller than the number of "
                    "participating sites"));
        }

        if (local_result.size() != num_sites)
        {
            return hpx::make_exceptional_future<std::vector<T>>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::collectives::detail::pairwise_all_to_all",
                    "each participating site must contribute exactly "
                    "num_sites elements"));
        }

        // A single site exchanges with itself only.
        if (num_sites == 1)
        {
            return hpx::make_ready_future(HPX_MOVE(local_result));
        }

        std::vector<hpx::future<void>> sends;
        sends.reserve(num_sites - 1);

        std::vector<hpx::future<T>> receives;
        receives.reserve(num_sites - 1);

        for (std::size_t k = 1; k != num_sites; ++k)
        {
            std::size_t const destination = (this_site + k) % num_sites;
            sends.push_back(set(comm, that_site_arg(destination),
                handle_bool<T>(HPX_MOVE(local_result[destination])), tag));

            std::size_t const source = (this_site + num_sites - k) % num_sites;
            receives.push_back(get<T>(comm, that_site_arg(source), tag));
        }

        // The diagonal never travels; it is this site's own contribution.
        T diagonal = HPX_MOVE(local_result[this_site]);

        return hpx::when_all(HPX_MOVE(sends), HPX_MOVE(receives))
            .then(hpx::launch::sync,
                // the communicator is captured to keep it alive until every
                // send and receive on it has completed
                [comm = HPX_MOVE(comm), diagonal = HPX_MOVE(diagonal),
                    num_sites, this_site](auto&& f) mutable {
                    auto [sent, received] = HPX_FORWARD(decltype(f), f).get();

                    // Report a failed send rather than a truncated result.
                    for (auto& send : sent)
                    {
                        send.get();
                    }

                    std::vector<T> result;
                    result.reserve(num_sites);
                    auto append_received = [&](std::size_t const source) {
                        // received[k - 1] came from the peer k hops behind
                        // this site, matching the receive order posted above.
                        std::size_t const k =
                            (this_site + num_sites - source) % num_sites;
                        result.push_back(received[k - 1].get());
                    };

                    for (std::size_t source = 0; source != this_site; ++source)
                    {
                        append_received(source);
                    }

                    result.push_back(HPX_MOVE(diagonal));

                    for (std::size_t source = this_site + 1;
                        source != num_sites; ++source)
                    {
                        append_received(source);
                    }

                    return result;
                });
    }
}    // namespace hpx::collectives::detail

#endif    // !HPX_COMPUTE_DEVICE_CODE
