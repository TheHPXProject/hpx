//  Copyright (c) 2021-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file std_execution_policy.hpp

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_CXX17_STD_EXECUTION_POLICES)
#include <hpx/modules/execution.hpp>

#include <execution>
#include <type_traits>

namespace hpx::detail {

    // Specialize our is_execution_policy traits for the corresponding std
    // versions

    /// \cond NOINTERNAL
    template <>
    struct policy_traits<std::execution::sequenced_policy>
      : policy_traits_default
    {
        static constexpr bool is_policy = true;
        static constexpr bool is_sequenced = true;
    };

    template <>
    struct policy_traits<std::execution::parallel_policy>
      : policy_traits_default
    {
        static constexpr bool is_policy = true;
        static constexpr bool is_parallel = true;
    };

    template <>
    struct policy_traits<std::execution::parallel_unsequenced_policy>
      : policy_traits_default
    {
        static constexpr bool is_policy = true;
        static constexpr bool is_parallel = true;
        static constexpr bool is_unsequenced = true;
    };

#if defined(HPX_HAVE_CXX20_STD_EXECUTION_POLICES)
    template <>
    struct policy_traits<std::execution::unsequenced_policy>
      : policy_traits_default
    {
        static constexpr bool is_policy = true;
        static constexpr bool is_sequenced = true;
        static constexpr bool is_unsequenced = true;
    };
#endif
    /// \endcond
}    // namespace hpx::detail

#endif
