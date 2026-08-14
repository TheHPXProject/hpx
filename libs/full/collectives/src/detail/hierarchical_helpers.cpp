//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/assert.hpp>
#include <hpx/collectives/create_communicator.hpp>
#include <hpx/collectives/detail/hierarchical_helpers.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>

#include <cstddef>
#include <exception>
#include <utility>

namespace hpx::collectives::detail {

    namespace {

        // The top level distributes num_sites as evenly as possible over
        // count groups (matching the division logic used by
        // recursively_fill_communicators): the first (num_sites % count)
        // groups get one extra site.
        std::pair<std::size_t, std::size_t> divide_sites(
            std::size_t const num_sites, std::size_t const count) noexcept
        {
            return {num_sites / count, num_sites % count};
        }
    }    // namespace

    std::exception_ptr validate_hierarchical_communicator(
        hierarchical_communicator const& communicators,
        this_site_arg const this_site, char const* operation)
    {
        if (!communicators.valid())
        {
            return HPX_GET_EXCEPTION(hpx::error::invalid_status, operation,
                "the hierarchical communicator is not valid");
        }

        auto const [num_sites_val, communicator_site] =
            communicators.get_info();

        // An out-of-range site would classify into no top-level group and
        // silently take a non-representative branch, hanging the collective.
        if (this_site >= num_sites_val)
        {
            return HPX_GET_EXCEPTION(hpx::error::bad_parameter, operation,
                "this_site must be smaller than the number of participating "
                "sites");
        }

        if (this_site != communicator_site)
        {
            return HPX_GET_EXCEPTION(hpx::error::bad_parameter, operation,
                "this_site must match the site used to create the "
                "hierarchical communicator");
        }

        return std::exception_ptr();
    }

    std::exception_ptr validate_hierarchical_root_caller(
        this_site_arg const this_site, char const* operation,
        char const* root_operation)
    {
        if (this_site != 0)
        {
            return HPX_GET_EXCEPTION(hpx::error::bad_parameter, operation,
                hpx::util::format(
                    "only site 0 may call {} on a hierarchical communicator",
                    root_operation));
        }

        return std::exception_ptr();
    }

    std::exception_ptr validate_hierarchical_non_root_caller(
        this_site_arg const this_site, char const* operation,
        char const* root_operation)
    {
        if (this_site == 0)
        {
            return HPX_GET_EXCEPTION(hpx::error::bad_parameter, operation,
                hpx::util::format(
                    "site 0 must call {} on a hierarchical communicator",
                    root_operation));
        }

        return std::exception_ptr();
    }

    std::exception_ptr validate_hierarchical_root_site(
        root_site_arg const root_site, char const* operation,
        char const* operation_name)
    {
        // The hierarchical helpers hardcode site 0 as the local root at
        // every tree level, so a non-zero root is not supported.
        if (root_site != 0)
        {
            return HPX_GET_EXCEPTION(hpx::error::bad_parameter, operation,
                hpx::util::format(
                    "hierarchical {} currently supports only root_site == 0 "
                    "(the tree designates site 0 as the root)",
                    operation_name));
        }

        return std::exception_ptr();
    }

    std::size_t get_top_level_group_count(
        std::size_t const num_sites, std::size_t const arity)
    {
        HPX_ASSERT(num_sites != 0);
        HPX_ASSERT(arity != 0);

        return arity > num_sites ? num_sites : arity;
    }

    std::size_t get_top_level_group_size(std::size_t const group,
        std::size_t const num_sites, std::size_t const arity)
    {
        std::size_t const count = get_top_level_group_count(num_sites, arity);
        HPX_ASSERT(group < count);

        auto const [division_steps, remainder] = divide_sites(num_sites, count);
        return division_steps + (group < remainder ? 1 : 0);
    }

    std::size_t get_top_level_group_left(std::size_t const group,
        std::size_t const num_sites, std::size_t const arity)
    {
        std::size_t const count = get_top_level_group_count(num_sites, arity);
        HPX_ASSERT(group < count);

        auto const [division_steps, remainder] = divide_sites(num_sites, count);
        return group * division_steps + (group < remainder ? group : remainder);
    }

    // Return the index of the top-level group this_site belongs to, or -1 if
    // there is none. The return type is signed so the -1 sentinel needs no
    // casts at the call sites.
    std::ptrdiff_t classify_site(std::size_t const this_site,
        std::size_t const num_sites, std::size_t const arity)
    {
        if (this_site >= num_sites)
        {
            return -1;
        }

        std::size_t const count = get_top_level_group_count(num_sites, arity);
        for (std::size_t group = 0; group != count; ++group)
        {
            std::size_t const left =
                get_top_level_group_left(group, num_sites, arity);
            std::size_t const size =
                get_top_level_group_size(group, num_sites, arity);

            if (this_site >= left && this_site < left + size)
            {
                return static_cast<std::ptrdiff_t>(group);
            }
        }

        return -1;
    }

    // Return true if this_site is the leftmost site (the representative) of
    // the given top-level group.
    bool is_top_level_rep(std::ptrdiff_t const group,
        std::size_t const this_site, std::size_t const num_sites,
        std::size_t const arity)
    {
        if (group == -1)
        {
            return false;
        }

        return this_site ==
            get_top_level_group_left(
                static_cast<std::size_t>(group), num_sites, arity);
    }

    bool is_top_level_rep(std::size_t const this_site,
        std::size_t const num_sites, std::size_t const arity)
    {
        return is_top_level_rep(classify_site(this_site, num_sites, arity),
            this_site, num_sites, arity);
    }
}    // namespace hpx::collectives::detail

#endif
