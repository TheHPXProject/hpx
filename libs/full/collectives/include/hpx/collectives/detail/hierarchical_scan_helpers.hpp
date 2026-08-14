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
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <utility>
#include <vector>

namespace hpx::collectives::detail {

    template <typename Result, typename T, typename BuildResults,
        typename FlatScan>
    hpx::future<Result> hierarchical_scan(char const* operation,
        hierarchical_communicator const& communicators, T&& local_result,
        this_site_arg const this_site, generation_arg const generation,
        root_site_arg const root_site, BuildResults&& build_results,
        FlatScan&& flat_scan)
    {
        if (generation.is_default() || generation == 0)
        {
            return hpx::make_exceptional_future<Result>(HPX_GET_EXCEPTION(
                hpx::error::bad_parameter, operation,
                "hierarchical scan requires an explicit, positive generation "
                "number for the 2k-1/2k internal mapping"));
        }

        if (!is_valid_hierarchical_phase_generation(generation))
        {
            return hpx::make_exceptional_future<Result>(HPX_GET_EXCEPTION(
                hpx::error::bad_parameter, operation,
                "the generation number is too large for the internal 2k-1/2k "
                "generation mapping"));
        }

        this_site_arg const effective_site = resolve_this_site(this_site);

        if (auto const error =
                validate_hierarchical_root_site(root_site, operation, "scan"))
        {
            return hpx::make_exceptional_future<Result>(error);
        }

        if (auto const error = validate_hierarchical_communicator(
                communicators, effective_site, operation))
        {
            return hpx::make_exceptional_future<Result>(error);
        }

        std::size_t const num_sites = hpx::get<0>(communicators.get_info());
        std::size_t const arity =
            static_cast<std::size_t>(communicators.get_arity());
        auto const [gather_gen, scatter_gen] =
            hierarchical_phase_generations(generation);

        if (arity >= num_sites)
        {
            HPX_ASSERT(communicators.size() == 1);
            return HPX_FORWARD(FlatScan, flat_scan)(communicators.get(0),
                HPX_FORWARD(T, local_result), communicators.site(0),
                gather_gen);
        }

        if (effective_site == root_site)
        {
            std::vector<Result> gathered = detail::gather_here(communicators,
                HPX_FORWARD(T, local_result), effective_site, gather_gen,
                detail::generation_mode::single_step)
                                               .get();

            std::vector<Result> results =
                HPX_FORWARD(BuildResults, build_results)(HPX_MOVE(gathered));

            return detail::scatter_to(communicators, HPX_MOVE(results),
                effective_site, scatter_gen,
                detail::generation_mode::single_step);
        }

        detail::gather_there(communicators, HPX_FORWARD(T, local_result),
            effective_site, gather_gen, detail::generation_mode::single_step)
            .get();

        return detail::scatter_from<Result>(communicators, effective_site,
            scatter_gen, detail::generation_mode::single_step);
    }
}    // namespace hpx::collectives::detail
#endif
