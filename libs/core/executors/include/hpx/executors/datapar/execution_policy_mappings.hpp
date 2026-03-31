//  Copyright (c) 2022-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/execution_policy_mappings.hpp

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/executors/execution_policy_mappings.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // Return the matching non-simd (vectorpack) execution policy
    HPX_CXX_CORE_EXPORT inline constexpr struct to_non_simd_t final
      : hpx::functional::detail::tag_fallback<to_non_simd_t>
    {
    private:
        // any non-simd policy just returns itself
        template <execution_policy ExPolicy>
        friend constexpr decltype(auto) tag_fallback_invoke(
            to_non_simd_t, ExPolicy&& policy) noexcept
        {
            static_assert(!hpx::is_vectorpack_execution_policy_v<ExPolicy>,
                "must not be a simd (vectorpack) execution policy");
            return std::forward<ExPolicy>(policy);
        }
    } to_non_simd{};

    template <>
    struct is_execution_policy_mapping<to_non_simd_t> : std::true_type
    {
    };

    // Return the matching simd (vectorpack) execution policy
    HPX_CXX_CORE_EXPORT inline constexpr struct to_simd_t final
      : hpx::functional::detail::tag_fallback<to_simd_t>
    {
    private:
        // any simd policy just returns itself

        template <execution_policy ExPolicy>
        friend constexpr decltype(auto) tag_fallback_invoke(
            to_simd_t, ExPolicy&& policy) noexcept
        {
            static_assert(hpx::is_vectorpack_execution_policy_v<ExPolicy>,
                "must be a simd (vectorpack) execution policy");
            return std::forward<ExPolicy>(policy);
        }
    } to_simd{};

    template <>
    struct is_execution_policy_mapping<to_simd_t> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

#endif
