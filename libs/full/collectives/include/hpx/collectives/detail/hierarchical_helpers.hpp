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

#include <cstddef>
#include <exception>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::collectives {
    struct hierarchical_communicator;
}

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

    [[nodiscard]] HPX_EXPORT std::exception_ptr
    validate_hierarchical_communicator(
        hierarchical_communicator const& communicators, this_site_arg this_site,
        char const* operation);

    // The tree hardcodes site 0 as the root at every level, so the root-side
    // entry points (broadcast_to, gather_here, reduce_here, scatter_to) may
    // only be called from site 0, and the non-root entry points only from the
    // other sites. `root_operation` names the root-side operation for the
    // resulting error message.
    [[nodiscard]] HPX_EXPORT std::exception_ptr
    validate_hierarchical_root_caller(this_site_arg this_site,
        char const* operation, char const* root_operation);

    [[nodiscard]] HPX_EXPORT std::exception_ptr
    validate_hierarchical_non_root_caller(this_site_arg this_site,
        char const* operation, char const* root_operation);

    // A non-zero root site is not supported by any of the hierarchical
    // implementations (the tree designates site 0 as the root).
    // `operation_name` is the plain operation name (e.g. "barrier") used in
    // the resulting error message.
    [[nodiscard]] HPX_EXPORT std::exception_ptr validate_hierarchical_root_site(
        root_site_arg root_site, char const* operation,
        char const* operation_name);

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

    // The topology helpers below are deliberately the only entities in this
    // file that carry HPX_CXX_EXPORT: the unit tests (hierarchical_helpers,
    // subtree_gather_scatter) call them directly, and with C++20 modules
    // enabled those tests consume this header through `import HPX.Full;`
    // (via the generated hpx/modules/collectives.hpp), so anything named
    // directly by an importer must be module-exported. Everything else here
    // is referenced only from within the module purview (reachability is
    // sufficient for the templates and validation helpers) and intentionally
    // stays unexported.
    HPX_CXX_EXPORT [[nodiscard]] HPX_EXPORT std::size_t
    get_top_level_group_count(std::size_t num_sites, std::size_t arity);

    HPX_CXX_EXPORT [[nodiscard]] HPX_EXPORT std::size_t
    get_top_level_group_left(
        std::size_t group, std::size_t num_sites, std::size_t arity);

    HPX_CXX_EXPORT [[nodiscard]] HPX_EXPORT std::size_t
    get_top_level_group_size(
        std::size_t group, std::size_t num_sites, std::size_t arity);

    HPX_CXX_EXPORT [[nodiscard]] HPX_EXPORT std::ptrdiff_t classify_site(
        std::size_t this_site, std::size_t num_sites, std::size_t arity);

    HPX_CXX_EXPORT [[nodiscard]] HPX_EXPORT bool is_top_level_rep(
        std::size_t this_site, std::size_t num_sites, std::size_t arity);

    // Overload for callers that have already classified this_site; avoids
    // repeating the classify_site scan. Only referenced from within the
    // module purview, hence no HPX_CXX_EXPORT.
    [[nodiscard]] HPX_EXPORT bool is_top_level_rep(std::ptrdiff_t group,
        std::size_t this_site, std::size_t num_sites, std::size_t arity);

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
