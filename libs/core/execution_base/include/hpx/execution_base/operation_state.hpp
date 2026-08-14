//  Copyright (c) 2020 Thomas Heller
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/execution_base/stdexec_forward.hpp>
#include <hpx/modules/execution_base.hpp>

#include <type_traits>

namespace hpx::execution::experimental {

    HPX_CXX_CORE_EXPORT template <typename OperationState>
    inline constexpr bool is_operation_state_v =
        operation_state<OperationState>;

    HPX_CXX_CORE_EXPORT template <typename OperationState>
    struct is_operation_state
      : std::bool_constant<operation_state<OperationState>>
    {
    };
}    // namespace hpx::execution::experimental
