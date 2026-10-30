//  Copyright (c) 2026 Anshuman Agrawal
//  Copyright (c) 2026 Alexander Strack
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
        hierarchical_communicator const& communicators,
        this_site_arg const this_site, char const* operation,
        char const* root_operation)
    {
        if (this_site != communicators.get_root_site())
        {
            return HPX_GET_EXCEPTION(hpx::error::bad_parameter, operation,
                hpx::util::format("only the root site may call {} on a "
                                  "hierarchical communicator",
                    root_operation));
        }

        return std::exception_ptr();
    }

    std::exception_ptr validate_hierarchical_non_root_caller(
        hierarchical_communicator const& communicators,
        this_site_arg const this_site, char const* operation,
        char const* root_operation)
    {
        if (this_site == communicators.get_root_site())
        {
            return HPX_GET_EXCEPTION(hpx::error::bad_parameter, operation,
                hpx::util::format(
                    "the root site must call {} on a hierarchical communicator",
                    root_operation));
        }

        return std::exception_ptr();
    }

    std::exception_ptr validate_hierarchical_root_site(
        hierarchical_communicator const& communicators,
        root_site_arg const root_site, char const* operation,
        char const* operation_name)
    {
        // 0 is accepted so callers that never named a root keep working; it
        // can't be told from an explicit root_site_arg(0), which is therefore
        // accepted on a communicator rooted elsewhere as well. Anything else
        // must be the root the tree was built with.
        if (!root_site.is_default() &&
            root_site != communicators.get_root_site())
        {
            return HPX_GET_EXCEPTION(hpx::error::bad_parameter, operation,
                hpx::util::format(
                    "hierarchical {} must use the root site the communicator "
                    "was created with ({})",
                    operation_name,
                    static_cast<std::size_t>(communicators.get_root_site())));
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

    bool is_top_level_rep(std::ptrdiff_t const group,
        std::size_t const this_site, std::size_t const num_sites,
        std::size_t const arity, std::size_t const root)
    {
        if (group == -1)
        {
            return false;
        }

        std::size_t const left = get_top_level_group_left(
            static_cast<std::size_t>(group), num_sites, arity);
        std::size_t const size = get_top_level_group_size(
            static_cast<std::size_t>(group), num_sites, arity);

        return this_site == range_leader(left, left + size - 1, root);
    }

    bool is_top_level_rep(std::size_t const this_site,
        std::size_t const num_sites, std::size_t const arity,
        std::size_t const root)
    {
        return is_top_level_rep(classify_site(this_site, num_sites, arity),
            this_site, num_sites, arity, root);
    }
}    // namespace hpx::collectives::detail

#endif
