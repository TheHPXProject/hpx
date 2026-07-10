//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/hierarchical_all_to_all_helpers.hpp
/// Subtree gather/scatter primitives for the hierarchical all_to_all
/// collective. These mirror the existing hierarchical gather_here/
/// gather_there and scatter_to/scatter_from patterns but skip the
/// top-level (inter-group) communicator at level 0.

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/assert.hpp>
#include <hpx/collectives/create_communicator.hpp>
#include <hpx/collectives/detail/flattened_data.hpp>
#include <hpx/collectives/gather.hpp>
#include <hpx/collectives/scatter.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::collectives::detail {

    ///////////////////////////////////////////////////////////////////////////
    // subtree_gather_at_top_rep
    //
    // Called by a top-level representative. Gathers data from all sites in
    // its subtree using communicator levels 1..size()-1, skipping level 0
    // (the inter-group communicator).
    //
    // Returns: flat vector of all subtree sites' data, ordered by subtree
    //          site rank.
    //
    // For the flat fallback case (comms.size() <= 1), returns the site's
    // own data wrapped in a single-element vector.
    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T>
    flattened_data<T> subtree_gather_at_top_rep(
        hierarchical_communicator const& comms, std::vector<T>&& local_result,
        generation_arg const generation)
    {
        HPX_ASSERT(comms.size() != 0);

        flattened_data<T> result = make_flattened_row(HPX_MOVE(local_result));

        // Walk bottom-up from leaf (size()-1) to level 1, skipping level 0
        // (the inter-group communicator). At every subtree level this site
        // is rank 0 (subtree root), so we call gather_here. For a
        // single-level subtree (size() == 1, e.g. the arity >= num_sites
        // case) the loop body simply does not execute and the site's own
        // data is returned.
        for (std::size_t i = comms.size() - 1; i != 0; --i)
        {
            result = gather_flattened_here(comms.get(i), HPX_MOVE(result),
                this_site_arg(0), generation, generation_mode::single_step)
                         .get();
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    // subtree_send_to_top_rep
    //
    // Called by a non-top-level-representative site. Sends this site's data
    // up through the subtree to the top-level representative.
    //
    // For non-rep sites, the entire communicator vector IS the subtree
    // portion (they don't have the inter-group communicator at level 0).
    // This is structurally identical to gather_there(hierarchical_comm):
    // walk bottom-up with gather_here at intermediate levels, then
    // gather_there at the topmost level.
    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T>
    void subtree_send_to_top_rep(hierarchical_communicator const& comms,
        std::vector<T>&& local_result, generation_arg const generation)
    {
        HPX_ASSERT(comms.size() != 0);

        flattened_data<T> data = make_flattened_row(HPX_MOVE(local_result));

        // Walk bottom-up from leaf to level 1, calling gather_here
        // (each intermediate rep gathers from its children).
        for (std::size_t i = comms.size() - 1; i != 0; --i)
        {
            data = gather_flattened_here(comms.get(i), HPX_MOVE(data),
                this_site_arg(0), generation, generation_mode::single_step)
                       .get();
        }

        // At the topmost level (level 0 in this site's vector, which is
        // a subtree-internal communicator, NOT the inter-group one),
        // send to the subtree root via gather_there.
        gather_flattened_there(comms.get(0), HPX_MOVE(data), comms.site(0),
            generation, generation_mode::single_step)
            .get();
    }

    ///////////////////////////////////////////////////////////////////////////
    // subtree_scatter_at_top_rep
    //
    // Called by a top-level representative. Scatters data from a flat vector
    // to all sites in its subtree using communicator levels 1..size()-1,
    // skipping level 0 (the inter-group communicator).
    //
    // Returns: this site's portion (a T) after scattering down the tree.
    //
    // For the flat fallback case (comms.size() <= 1), returns the first
    // (and only) element of the input data.
    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<T>> subtree_scatter_at_top_rep(
        hierarchical_communicator const& comms, flattened_data<T>&& data,
        generation_arg const generation)
    {
        HPX_ASSERT(comms.size() != 0);

        // Flat fallback or single-level subtree: the entire data vector
        // belongs to this site.
        if (comms.size() == 1)
        {
            return hpx::make_ready_future(unwrap_flattened_row(HPX_MOVE(data)));
        }

        // Walk top-down from level 1 through size()-2, partitioning and
        // scattering at each level. At each level, this site is rank 0
        // (subtree root).
        for (std::size_t i = 1; i < comms.size() - 1; ++i)
        {
            data = scatter_flattened_to(comms.get(i), HPX_MOVE(data),
                this_site_arg(0), generation, generation_mode::single_step)
                       .get();
        }

        // At the leaf level (size()-1), scatter asynchronously and return
        // the future directly (no scatter_data; each element maps 1:1 to a
        // leaf site).
        return hpx::make_future<std::vector<T>>(
            scatter_flattened_to(comms.back(), HPX_MOVE(data), this_site_arg(0),
                generation, generation_mode::single_step),
            [](flattened_data<T>&& result) {
                return unwrap_flattened_row(HPX_MOVE(result));
            });
    }

    ///////////////////////////////////////////////////////////////////////////
    // subtree_receive_from_top_rep
    //
    // Called by a non-top-level-representative site. Receives this site's
    // portion of the scattered data from the top-level representative.
    //
    // For non-rep sites, the entire communicator vector IS the subtree
    // portion. This mirrors scatter_from(hierarchical_comm): receive at
    // level 0, then scatter down through intermediate levels to the leaf.
    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T>
    hpx::future<std::vector<T>> subtree_receive_from_top_rep(
        hierarchical_communicator const& comms, generation_arg const generation)
    {
        HPX_ASSERT(comms.size() != 0);

        auto [current_communicator, current_site] = comms[0];

        // A direct leaf of the top representative receives exactly its own
        // portion (a single T) as a future, returned directly.
        if (comms.size() == 1)
        {
            return hpx::make_future<std::vector<T>>(
                scatter_flattened_from<T>(current_communicator, current_site,
                    generation, generation_mode::single_step),
                [](flattened_data<T>&& result) {
                    return unwrap_flattened_row(HPX_MOVE(result));
                });
        }

        // An intermediate representative receives a vector<T> (one element
        // per site in its subtree) to scatter further down.
        flattened_data<T> data = scatter_flattened_from<T>(current_communicator,
            current_site, generation, generation_mode::single_step)
                                     .get();

        // Walk down through intermediate levels.
        for (std::size_t i = 1; i < comms.size() - 1; ++i)
        {
            data = scatter_flattened_to(comms.get(i), HPX_MOVE(data),
                this_site_arg(0), generation, generation_mode::single_step)
                       .get();
        }

        // At the leaf level, scatter asynchronously and return the future
        // directly (no scatter_data; each element maps 1:1 to a leaf site).
        return hpx::make_future<std::vector<T>>(
            scatter_flattened_to(comms.back(), HPX_MOVE(data), this_site_arg(0),
                generation, generation_mode::single_step),
            [](flattened_data<T>&& result) {
                return unwrap_flattened_row(HPX_MOVE(result));
            });
    }

}    // namespace hpx::collectives::detail

#endif    // !HPX_COMPUTE_DEVICE_CODE
