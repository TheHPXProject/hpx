//  Copyright (c)      2017 Shoshana Jakobovits
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file partitioner_mode.hpp
/// \page hpx::resource::partitioner_mode
/// \headerfile hpx/resource_partitioner/partitioner_mode.hpp

#pragma once

#include <hpx/config.hpp>
#include <cstdint>

namespace hpx::resource {

    /// This enumeration describes the modes available when creating a
    /// resource partitioner.
    HPX_CXX_CORE_EXPORT enum class partitioner_mode : std::int8_t {
        /// Default mode.
        default_ = 0,

        /// Allow processing units to be oversubscribed, i.e. multiple
        /// worker threads to share a single processing unit.
        allow_oversubscription = 1,

        /// Allow worker threads to be added and removed from thread pools.
        allow_dynamic_pools = 2
    };

    HPX_CXX_CORE_EXPORT constexpr partitioner_mode operator&(
        partitioner_mode lhs, partitioner_mode rhs) noexcept
    {
        return static_cast<partitioner_mode>(
            static_cast<int>(lhs) & static_cast<int>(rhs));
    }

    HPX_CXX_CORE_EXPORT constexpr partitioner_mode operator|(
        partitioner_mode lhs, partitioner_mode rhs) noexcept
    {
        return static_cast<partitioner_mode>(
            static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    HPX_CXX_CORE_EXPORT constexpr partitioner_mode operator^(
        partitioner_mode lhs, partitioner_mode rhs) noexcept
    {
        return static_cast<partitioner_mode>(
            static_cast<int>(lhs) ^ static_cast<int>(rhs));
    }

    HPX_CXX_CORE_EXPORT constexpr partitioner_mode operator~(
        partitioner_mode mode) noexcept
    {
        return static_cast<partitioner_mode>(~static_cast<int>(mode));
    }

    HPX_CXX_CORE_EXPORT constexpr bool as_bool(partitioner_mode val) noexcept
    {
        return static_cast<int>(val) != 0;
    }

}    // namespace hpx::resource
