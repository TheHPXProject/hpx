//  Copyright (c) 2014 Bibek Ghimire
//  Copyright (c) 2014-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace hpx::traits {

    HPX_CXX_EXPORT template <typename T, typename Enable = void>
    struct is_distribution_policy : std::false_type
    {
    };

    HPX_CXX_EXPORT template <typename T>
    inline constexpr bool is_distribution_policy_v =
        is_distribution_policy<T>::value;

    // By default, the number of partitions is the same as the number of
    // localities represented by the given distribution policy.
    HPX_CXX_EXPORT template <typename Policy, typename Enable = void>
    struct num_container_partitions
    {
        static std::size_t call(Policy const& policy)
        {
            return policy.get_num_localities();
        }
    };

    // By default, the sizes of the partitions are equal across the localities
    // represented by the given distribution policy.
    HPX_CXX_EXPORT template <typename Policy, typename Enable = void>
    struct container_partition_sizes
    {
        static std::vector<std::size_t> call(
            Policy const& policy, std::size_t const size)
        {
            std::size_t const num_parts = policy.get_num_partitions();
            return std::vector<std::size_t>(
                num_parts, (size + num_parts - 1) / num_parts);
        }
    };
}    // namespace hpx::traits
