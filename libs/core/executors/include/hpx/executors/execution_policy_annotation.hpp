//  Copyright (c) 2021-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/execution_policy_annotation.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/executors/annotating_executor.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/properties.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <concepts>
#include <string>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    // with_annotation/get_annotation property implementations for execution
    // policies are provided through the public query() member functions on
    // execution_policy (see hpx/executors/execution_policy.hpp). The property
    // CPOs detect those members directly (via property_base), so no tag_invoke
    // bridge is needed here.
}    // namespace hpx::execution::experimental
