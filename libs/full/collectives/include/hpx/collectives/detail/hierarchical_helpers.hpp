//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/hierarchical_helpers.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/assert.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <vector>

namespace hpx::collectives::detail {

    HPX_CXX_EXPORT struct top_level_group
    {
        std::size_t left;
        std::size_t right;
        std::size_t size;
    };

    // Compute the top-level partition of [0, num_sites) into `arity` groups.
    // Uses the same division logic as recursively_fill_communicators:
    // first (num_sites % arity) groups get ceil(num_sites/arity) sites,
    // the rest get floor(num_sites/arity).
    HPX_CXX_EXPORT inline std::vector<top_level_group> get_top_level_groups(
        std::size_t num_sites, std::size_t arity)
    {
        HPX_ASSERT(arity != 0);

        if (arity > num_sites)
        {
            arity = num_sites;
        }

        auto const dv = std::lldiv(
            static_cast<long long>(num_sites), static_cast<long long>(arity));

        std::vector<top_level_group> groups;
        groups.reserve(arity);

        std::size_t offset = 0;
        for (std::size_t i = 0; i != arity; ++i)
        {
            std::size_t const group_size = static_cast<std::size_t>(dv.quot) +
                (i < static_cast<std::size_t>(dv.rem) ? 1 : 0);

            std::size_t const left = offset;
            std::size_t const right = left + group_size - 1;
            groups.push_back(top_level_group{left, right, group_size});

            offset += group_size;
        }

        return groups;
    }

    // Return the index of the top-level group that contains `this_site`,
    // or -1 if there is none. Groups are sorted by left boundary, so we use
    // std::lower_bound. The return type is signed so the -1 sentinel and the
    // std::distance result need no casts.
    HPX_CXX_EXPORT inline std::ptrdiff_t classify_site(
        std::size_t this_site, std::vector<top_level_group> const& groups)
    {
        auto const it = std::lower_bound(groups.begin(), groups.end(),
            this_site, [](top_level_group const& g, std::size_t site) {
                return g.right < site;
            });

        if (it != groups.end() && this_site >= it->left)
        {
            return std::distance(groups.begin(), it);
        }

        return -1;
    }

    // Return true if `this_site` is the leftmost site (representative)
    // of its top-level group.
    HPX_CXX_EXPORT inline bool is_top_level_rep(
        std::size_t this_site, std::size_t num_sites, std::size_t arity)
    {
        auto const groups = get_top_level_groups(num_sites, arity);
        auto const g = classify_site(this_site, groups);
        return g != -1 && this_site == groups[g].left;
    }

}    // namespace hpx::collectives::detail
