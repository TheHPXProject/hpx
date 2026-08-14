//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)

#include <hpx/executors/datapar/detail/execution_policy_mapping_members.hpp>

namespace hpx::execution::detail {

    template <typename Derived>
    constexpr auto simd_sync_policy_mappings<Derived>::to_task() const
    {
        return map_execution_policy<simd_task_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_task);
    }

    template <typename Derived>
    constexpr auto simd_sync_policy_mappings<Derived>::to_par() const
    {
        return map_execution_policy<par_simd_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_par);
    }

    template <typename Derived>
    constexpr auto simd_sync_policy_mappings<Derived>::to_non_simd() const
    {
        return map_execution_policy<sequenced_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_non_simd);
    }

    template <typename Derived>
    constexpr auto simd_async_policy_mappings<Derived>::to_non_task() const
    {
        return map_execution_policy<simd_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_non_task);
    }

    template <typename Derived>
    constexpr auto simd_async_policy_mappings<Derived>::to_par() const
    {
        return map_execution_policy<par_simd_task_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_par);
    }

    template <typename Derived>
    constexpr auto simd_async_policy_mappings<Derived>::to_non_simd() const
    {
        return map_execution_policy<sequenced_task_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_non_simd);
    }

    template <typename Derived>
    constexpr auto par_simd_sync_policy_mappings<Derived>::to_task() const
    {
        return map_execution_policy<par_simd_task_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_task);
    }

    template <typename Derived>
    constexpr auto par_simd_sync_policy_mappings<Derived>::to_non_par() const
    {
        return map_execution_policy<simd_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_non_par);
    }

    template <typename Derived>
    constexpr auto par_simd_sync_policy_mappings<Derived>::to_non_simd() const
    {
        return map_execution_policy<parallel_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_non_simd);
    }

    template <typename Derived>
    constexpr auto par_simd_async_policy_mappings<Derived>::to_non_task() const
    {
        return map_execution_policy<par_simd_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_non_task);
    }

    template <typename Derived>
    constexpr auto par_simd_async_policy_mappings<Derived>::to_non_par() const
    {
        return map_execution_policy<simd_task_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_non_par);
    }

    template <typename Derived>
    constexpr auto par_simd_async_policy_mappings<Derived>::to_non_simd() const
    {
        return map_execution_policy<parallel_task_policy>(
            static_cast<Derived const&>(*this),
            hpx::execution::experimental::to_non_simd);
    }

}    // namespace hpx::execution::detail

#endif
