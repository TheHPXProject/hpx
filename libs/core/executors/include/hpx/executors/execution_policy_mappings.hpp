//  Copyright (c) 2022-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/execution_policy_mappings.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Tag>
    struct is_execution_policy_mapping : std::false_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Tag>
    inline constexpr bool is_execution_policy_mapping_v =
        is_execution_policy_mapping<Tag>::value;

    ///////////////////////////////////////////////////////////////////////////
    // Return the matching non-parallel (sequenced) execution policy
    HPX_CXX_CORE_EXPORT inline constexpr struct to_non_par_t final
      : hpx::functional::detail::tag_fallback<to_non_par_t>
    {
    private:
        // any non-parallel policy just returns itself
        template <execution_policy ExPolicy>
        friend constexpr decltype(auto) tag_fallback_invoke(
            to_non_par_t, ExPolicy&& policy) noexcept
        {
            static_assert(!hpx::is_parallel_execution_policy_v<ExPolicy>,
                "must not be a parallel execution policy");
            return std::forward<ExPolicy>(policy);
        }
    } to_non_par{};

    template <>
    struct is_execution_policy_mapping<to_non_par_t> : std::true_type
    {
    };

    // Return the matching parallel execution policy
    HPX_CXX_CORE_EXPORT inline constexpr struct to_par_t final
      : hpx::functional::detail::tag_fallback<to_par_t>
    {
    private:
        // any parallel policy just returns itself
        template <execution_policy ExPolicy>
        friend constexpr decltype(auto) tag_fallback_invoke(
            to_par_t, ExPolicy&& policy) noexcept
        {
            static_assert(hpx::is_parallel_execution_policy_v<ExPolicy>,
                "must be a parallel execution policy");
            return std::forward<ExPolicy>(policy);
        }
    } to_par{};

    template <>
    struct is_execution_policy_mapping<to_par_t> : std::true_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    // Return the matching non-task (synchronous) execution policy
    HPX_CXX_CORE_EXPORT inline constexpr struct to_non_task_t
      : hpx::functional::detail::tag_fallback<to_non_task_t>
    {
    private:
        // any non-task policy just returns itself
        template <execution_policy ExPolicy>
        friend constexpr decltype(auto) tag_fallback_invoke(
            to_non_task_t, ExPolicy&& policy) noexcept
        {
            static_assert(!hpx::is_async_execution_policy_v<ExPolicy>,
                "must not be an asynchronous (task) execution policy");
            return std::forward<ExPolicy>(policy);
        }
    } to_non_task{};

    template <>
    struct is_execution_policy_mapping<to_non_task_t> : std::true_type
    {
    };

    // Return the matching task (asynchronous) execution policy
    HPX_CXX_CORE_EXPORT inline constexpr struct to_task_t
      : hpx::functional::detail::tag_fallback<to_task_t>
    {
    private:
        // any task policy just returns itself
        template <execution_policy ExPolicy>
        friend constexpr decltype(auto) tag_fallback_invoke(
            to_task_t, ExPolicy&& policy) noexcept
        {
            static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
                "must be an asynchronous (task) execution policy");
            return std::forward<ExPolicy>(policy);
        }
    } to_task{};

    template <>
    struct is_execution_policy_mapping<to_task_t> : std::true_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    // Return the matching non-unsequenced execution policy
    HPX_CXX_CORE_EXPORT inline constexpr struct to_non_unseq_t final
      : hpx::functional::detail::tag_fallback<to_non_unseq_t>
    {
    private:
        // any non-unsequenced policy just returns itself
        template <execution_policy ExPolicy>
        friend constexpr decltype(auto) tag_fallback_invoke(
            to_non_unseq_t, ExPolicy&& policy) noexcept
        {
            static_assert(!hpx::is_unsequenced_execution_policy_v<ExPolicy>,
                "must not be an unsequenced execution policy");
            return std::forward<ExPolicy>(policy);
        }
    } to_non_unseq{};

    template <>
    struct is_execution_policy_mapping<to_non_unseq_t> : std::true_type
    {
    };

    // Return the matching unsequenced execution policy
    HPX_CXX_CORE_EXPORT inline constexpr struct to_unseq_t final
      : hpx::functional::detail::tag_fallback<to_unseq_t>
    {
    private:
        // any unsequenced policy just returns itself
        template <execution_policy ExPolicy>
        friend constexpr decltype(auto) tag_fallback_invoke(
            to_unseq_t, ExPolicy&& policy) noexcept
        {
            static_assert(hpx::is_unsequenced_execution_policy_v<ExPolicy>,
                "must be an unsequenced execution policy");
            return std::forward<ExPolicy>(policy);
        }
    } to_unseq{};

    template <>
    struct is_execution_policy_mapping<to_unseq_t> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental
