//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_base/detail/query_first_fallback.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/topology/detail/mask_fallback.hpp>

#include <type_traits>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct get_processing_units_mask_t
        final
      : detail::query_first_tag_fallback<get_processing_units_mask_t,
            detail::machine_affinity_mask_fallback>
    {
        constexpr get_processing_units_mask_t() = default;
    } get_processing_units_mask{};

    template <>
    struct is_scheduling_property<get_processing_units_mask_t> : std::true_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct get_cores_mask_t final
      : detail::query_first_tag_fallback<get_cores_mask_t,
            detail::machine_affinity_mask_fallback>
    {
        constexpr get_cores_mask_t() = default;
    } get_cores_mask{};

    template <>
    struct is_scheduling_property<get_cores_mask_t> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental
