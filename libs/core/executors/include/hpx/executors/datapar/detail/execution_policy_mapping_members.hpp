//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)

#include <hpx/modules/properties.hpp>

namespace hpx::execution::detail {

    template <typename ResultPolicy, typename Derived, typename MappingTag>
    constexpr auto map_execution_policy(Derived const& self, MappingTag tag)
    {
        return ResultPolicy()
            .on(hpx::experimental::prefer(tag, self.executor()))
            .with(self.parameters());
    }

    template <typename Derived>
    struct simd_sync_policy_mappings
    {
        constexpr auto to_task() const;
        constexpr auto to_par() const;
        constexpr auto to_non_simd() const;

    private:
        friend Derived;
        constexpr simd_sync_policy_mappings() = default;
    };

    template <typename Derived>
    struct simd_async_policy_mappings
    {
        constexpr auto to_non_task() const;
        constexpr auto to_par() const;
        constexpr auto to_non_simd() const;

    private:
        friend Derived;
        constexpr simd_async_policy_mappings() = default;
    };

    template <typename Derived>
    struct par_simd_sync_policy_mappings
    {
        constexpr auto to_task() const;
        constexpr auto to_non_par() const;
        constexpr auto to_non_simd() const;

    private:
        friend Derived;
        constexpr par_simd_sync_policy_mappings() = default;
    };

    template <typename Derived>
    struct par_simd_async_policy_mappings
    {
        constexpr auto to_non_task() const;
        constexpr auto to_non_par() const;
        constexpr auto to_non_simd() const;

    private:
        friend Derived;
        constexpr par_simd_async_policy_mappings() = default;
    };

}    // namespace hpx::execution::detail

#endif
