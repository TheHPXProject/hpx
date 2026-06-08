//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/execution_policy_parameters.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <concepts>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    // with_processing_units_count property implementations for execution
    // policies that support the embedded executor are provided through the
    // public query() member functions on execution_policy (see
    // hpx/executors/execution_policy.hpp).

    // general fallback for parameters types that are not directly supported by
    // the underlying executor
    HPX_CXX_CORE_EXPORT template <typename ParametersProperty,
        execution_policy ExPolicy, executor_parameters Params>
        requires(!is_scheduling_property_v<ParametersProperty>)
    constexpr decltype(auto) tag_fallback_invoke(
        ParametersProperty, ExPolicy&& policy, Params&& params)
    {
        return policy.with(HPX_FORWARD(Params, params));
    }

    HPX_CXX_CORE_EXPORT template <typename ParametersProperty,
        typename ExPolicy, typename... Ts,
        HPX_CONCEPT_REQUIRES_(hpx::is_execution_policy_v<ExPolicy>)>
    constexpr auto tag_fallback_invoke(
        ParametersProperty prop, ExPolicy&& policy, Ts&&... ts)
        -> decltype(std::declval<ParametersProperty>()(
            std::declval<typename std::decay_t<ExPolicy>::executor_type>(),
            std::declval<Ts>()...))
    {
        return prop(policy.executor(), HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::execution::experimental
