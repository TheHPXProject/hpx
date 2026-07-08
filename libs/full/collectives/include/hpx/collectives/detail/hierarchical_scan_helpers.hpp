//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/hierarchical_scan_helpers.hpp

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/assert.hpp>
#include <hpx/collectives/argument_types.hpp>
#include <hpx/collectives/create_communicator.hpp>
#include <hpx/collectives/detail/hierarchical_helpers.hpp>
#include <hpx/collectives/gather.hpp>
#include <hpx/collectives/scatter.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::collectives::detail {

    struct hierarchical_scan_parameters
    {
        this_site_arg this_site;
        std::size_t num_sites;
        std::size_t arity;
        hierarchical_phase_generation_pair generations;
        char const* error_message;
    };

    inline hierarchical_scan_parameters make_invalid_hierarchical_scan_params(
        char const* error_message) noexcept
    {
        return {this_site_arg(), 0, 0, hierarchical_phase_generation_pair(),
            error_message};
    }

    inline hierarchical_scan_parameters validate_hierarchical_scan_params(
        hierarchical_communicator const& communicators, this_site_arg this_site,
        generation_arg const generation, root_site_arg const root_site)
    {
        if (generation.is_default() || generation == 0)
        {
            return make_invalid_hierarchical_scan_params(
                "hierarchical scan requires an explicit, positive generation "
                "number for the 2k-1/2k internal mapping");
        }

        if (!is_valid_hierarchical_phase_generation(generation))
        {
            return make_invalid_hierarchical_scan_params(
                "the generation number is too large for the internal 2k-1/2k "
                "generation mapping");
        }

        if (this_site.is_default())
        {
            this_site = agas::get_locality_id();
        }

        auto const info = communicators.get_info();
        std::size_t const num_sites = hpx::get<0>(info);
        std::size_t const communicator_site = hpx::get<1>(info);
        std::size_t const arity =
            static_cast<std::size_t>(communicators.get_arity());

        if (root_site != 0)
        {
            return make_invalid_hierarchical_scan_params(
                "hierarchical scan currently supports only root_site == 0 "
                "(the tree designates site 0 as the root)");
        }

        if (this_site >= num_sites)
        {
            return make_invalid_hierarchical_scan_params(
                "this_site must be smaller than the number of participating "
                "sites");
        }

        if (this_site != communicator_site)
        {
            return make_invalid_hierarchical_scan_params(
                "this_site must match the site used to create the "
                "hierarchical communicator");
        }

        return {this_site, num_sites, arity,
            hierarchical_phase_generations(generation), nullptr};
    }

    template <typename Result, typename T, typename BuildResults,
        typename FlatScan>
    hpx::future<Result> hierarchical_scan(char const* operation,
        hierarchical_communicator const& communicators, T&& local_result,
        this_site_arg const this_site, generation_arg const generation,
        root_site_arg const root_site, BuildResults&& build_results,
        FlatScan&& flat_scan)
    {
        hierarchical_scan_parameters const params =
            validate_hierarchical_scan_params(
                communicators, this_site, generation, root_site);

        if (params.error_message != nullptr)
        {
            return hpx::make_exceptional_future<Result>(HPX_GET_EXCEPTION(
                hpx::error::bad_parameter, operation, params.error_message));
        }

        auto const [gather_gen, scatter_gen] = params.generations;

        if (params.arity >= params.num_sites)
        {
            HPX_ASSERT(communicators.size() == 1);
            return HPX_FORWARD(FlatScan, flat_scan)(communicators.get(0),
                HPX_FORWARD(T, local_result), communicators.site(0),
                gather_gen);
        }

        if (params.this_site == root_site)
        {
            std::vector<Result> gathered = detail::gather_here(communicators,
                HPX_FORWARD(T, local_result), params.this_site, gather_gen,
                detail::generation_mode::single_step)
                                               .get();

            std::vector<Result> results =
                HPX_FORWARD(BuildResults, build_results)(HPX_MOVE(gathered));

            return detail::scatter_to(communicators, HPX_MOVE(results),
                params.this_site, scatter_gen,
                detail::generation_mode::single_step);
        }

        detail::gather_there(communicators, HPX_FORWARD(T, local_result),
            params.this_site, gather_gen, detail::generation_mode::single_step)
            .get();

        return detail::scatter_from<Result>(communicators, params.this_site,
            scatter_gen, detail::generation_mode::single_step);
    }
}    // namespace hpx::collectives::detail
#endif
