//  Copyright (c)      2025 Aditya Sapra
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// HPX algorithm headers for tag types
#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/thrust/thrust_headers.hpp>

// Centralized Thrust algorithm headers
#include <type_traits>
#include <utility>

namespace hpx::thrust::detail {

    // No definition = compilation error for unmapped algorithms
    HPX_CXX_CORE_EXPORT template <typename Tag>
    struct algorithm_map;

    template <>
    struct algorithm_map<hpx::fill_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::fill(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::fill_n_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::fill_n(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::copy_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::copy(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::transform_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::transform(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::for_each_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::for_each(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::reduce_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::reduce(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::sort_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::sort(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::find_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::find(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::count_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::count(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::unique_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::unique(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::reverse_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::reverse(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::generate_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::generate(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::generate_n_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::generate_n(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::remove_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::remove(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::remove_if_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::remove_if(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::replace_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::replace(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::replace_if_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::replace_if(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::merge_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::merge(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::partition_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::partition(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::transform_reduce_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::transform_reduce(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::copy_n_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::copy_n(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::copy_if_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::copy_if(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::count_if_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::count_if(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::find_if_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::find_if(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::find_if_not_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::find_if_not(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::stable_sort_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::stable_sort(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::stable_partition_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::stable_partition(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::partition_copy_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::partition_copy(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::remove_copy_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::remove_copy(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::remove_copy_if_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::remove_copy_if(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::replace_copy_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::replace_copy(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::replace_copy_if_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::replace_copy_if(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::reverse_copy_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::reverse_copy(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::unique_copy_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::unique_copy(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::adjacent_difference_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::adjacent_difference(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::transform_inclusive_scan_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::transform_inclusive_scan(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::transform_exclusive_scan_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::transform_exclusive_scan(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::equal_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::equal(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::mismatch_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::mismatch(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::min_element_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::min_element(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::max_element_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::max_element(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::minmax_element_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::minmax_element(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::all_of_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::all_of(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::any_of_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::any_of(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::none_of_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::none_of(policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::swap_ranges_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::swap_ranges(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::uninitialized_copy_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::uninitialized_copy(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::uninitialized_copy_n_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::uninitialized_copy_n(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::uninitialized_fill_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::uninitialized_fill(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::uninitialized_fill_n_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::uninitialized_fill_n(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::set_union_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::set_union(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::set_intersection_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::set_intersection(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::set_difference_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::set_difference(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::set_symmetric_difference_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::set_symmetric_difference(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::for_each_n_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::for_each_n(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::is_sorted_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::is_sorted(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::is_partitioned_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::is_partitioned(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::inclusive_scan_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::inclusive_scan(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    template <>
    struct algorithm_map<hpx::exclusive_scan_t>
    {
        template <typename Policy, typename... Args>
        static constexpr decltype(auto) invoke(Policy&& policy, Args&&... args)
        {
            return ::thrust::exclusive_scan(
                policy.get(), HPX_FORWARD(Args, args)...);
        }
    };

    // boolean predicate indicating whether a mapping exists
    //
    // This checks only whether algorithm_map<Tag> is defined. Checking the
    // call to invoke() would require deducing its decltype(auto) return
    // type, which instantiates the Thrust algorithm. For arguments Thrust
    // can't handle (e.g. segmented iterators) that is a hard error instead
    // of an unsatisfied constraint, which would prevent more specialized
    // hpx_invoke overloads from being selected.
    HPX_CXX_CORE_EXPORT template <typename Tag, typename Policy,
        typename... Args>
    concept is_algorithm_mapped = requires { sizeof(algorithm_map<Tag>); };
}    // namespace hpx::thrust::detail
