//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/hierarchical_helpers.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/collectives/argument_types.hpp>
#include <hpx/modules/functional.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::collectives::detail {

    // The per-call generation parameters a hierarchical sub-communicator uses:
    // the run generation handed to the flat operation, and how many generations
    // the gate advances by in a single step.
    struct hierarchical_run
    {
        generation_arg generation;
        generation_mode num_generations;
    };

    struct hierarchical_phase_generation_pair
    {
        generation_arg first;
        generation_arg second;
    };

    template <typename ValueType, typename Data>
    constexpr decltype(auto) handle_bool(Data&& data) noexcept
    {
        if constexpr (std::is_same_v<ValueType, bool>)
        {
            return static_cast<bool>(data);
        }
        else
        {
            return HPX_FORWARD(Data, data);
        }
    }

    [[nodiscard]] constexpr bool is_valid_hierarchical_phase_generation(
        generation_arg const generation) noexcept
    {
        return !generation.is_default() && generation != 0 &&
            static_cast<std::size_t>(generation) <=
            (std::numeric_limits<std::size_t>::max)() / 2;
    }

    [[nodiscard]] constexpr bool is_valid_hierarchical_run_generation(
        generation_arg const generation,
        generation_mode const num_generations) noexcept
    {
        if (generation.is_default() || generation == 0)
        {
            return true;
        }

        std::size_t const step = static_cast<std::size_t>(num_generations);
        HPX_ASSERT(step != 0);
        return static_cast<std::size_t>(generation) <=
            (std::numeric_limits<std::size_t>::max)() / step;
    }

    constexpr hierarchical_phase_generation_pair hierarchical_phase_generations(
        generation_arg const generation) noexcept
    {
        HPX_ASSERT(is_valid_hierarchical_phase_generation(generation));

        std::size_t const k = generation;
        return {generation_arg(2 * k - 1), generation_arg(2 * k)};
    }

    // Map a user generation k to those parameters when the communicator
    // advances num_generations per call: the run generation becomes
    // num_generations * (k - 1) + 1 (so 2k-1 for the common double_step, and
    // the identity for single_step). A default (auto) generation is passed
    // through unchanged and always advances by one, preserving the single-step
    // behavior for instances that are not shared across collectives (sharing
    // requires explicit, consecutive generations). Generation 0 is also passed
    // through so the downstream flat operation rejects it with bad_parameter
    // rather than the mapping silently wrapping it onto the default sentinel.
    constexpr hierarchical_run hierarchical_run_params(
        generation_arg const generation, generation_mode const num_generations)
    {
        HPX_ASSERT(
            is_valid_hierarchical_run_generation(generation, num_generations));

        if (generation.is_default() || generation == 0)
        {
            return {generation, generation_mode::single_step};
        }

        // Each hierarchical call consumes `step` consecutive gate generations
        // on every sub-communicator it touches, so user generation k (>= 1)
        // owns the gap-free block of internal generations
        // [step*(k-1) + 1, step*k]: k=1 -> [1, step], k=2 -> [step+1, 2*step],
        // and so on. The flat operation is launched at the first generation of
        // that block, step*(k-1) + 1, and `step` (== num_generations) is
        // reported back so handle_data advances the gate past the remainder of
        // the block in a single move. So double_step maps k -> 2k-1 (e.g. the
        // 2k-1 gather/exchange phase, with 2k consumed by the paired
        // broadcast/scatter) and single_step is the identity k -> k (used when
        // the caller already passes the mapped generation).
        std::size_t const step = static_cast<std::size_t>(num_generations);
        return {generation_arg(
                    step * (static_cast<std::size_t>(generation) - 1) + 1),
            num_generations};
    }

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

        std::size_t const division_steps = num_sites / arity;
        std::size_t const remainder = num_sites % arity;

        std::vector<top_level_group> groups;
        groups.reserve(arity);

        std::size_t offset = 0;
        for (std::size_t i = 0; i != arity; ++i)
        {
            std::size_t const group_size =
                division_steps + (i < remainder ? 1 : 0);

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

    template <typename T, typename F>
    std::vector<T> make_inclusive_scan_results(std::vector<T>&& values, F&& op)
    {
        std::vector<T> results;
        results.reserve(values.size());

        auto it = values.begin();
        if (it == values.end())
        {
            return results;
        }

        T prefix = handle_bool<T>(HPX_MOVE(*it));
        results.emplace_back(prefix);

        for (++it; it != values.end(); ++it)
        {
            prefix = HPX_INVOKE(
                op, handle_bool<T>(HPX_MOVE(prefix)), handle_bool<T>(*it));
            results.emplace_back(prefix);
        }

        return results;
    }

    template <typename T, typename F>
    std::vector<T> make_exclusive_scan_results(std::vector<T>&& values, F&& op)
    {
        std::vector<T> results;
        results.reserve(values.size());

        auto it = values.begin();
        if (it == values.end())
        {
            return results;
        }

        results.emplace_back(T{});

        T prefix = handle_bool<T>(HPX_MOVE(*it));
        for (++it; it != values.end(); ++it)
        {
            results.emplace_back(prefix);
            prefix = HPX_INVOKE(
                op, handle_bool<T>(HPX_MOVE(prefix)), handle_bool<T>(*it));
        }

        return results;
    }

    template <typename T, typename U, typename F>
    std::vector<T> make_exclusive_scan_results(
        std::vector<T>&& values, U&& init, F&& op)
    {
        std::vector<T> results;
        results.reserve(values.size());

        T prefix = handle_bool<T>(static_cast<T>(HPX_FORWARD(U, init)));
        for (auto&& value : values)
        {
            results.emplace_back(prefix);
            prefix = HPX_INVOKE(
                op, handle_bool<T>(HPX_MOVE(prefix)), handle_bool<T>(value));
        }

        return results;
    }

}    // namespace hpx::collectives::detail
