//  Copyright (c) 2023-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/execution_policy_annotation.hpp

#pragma once

#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <type_traits>

namespace hpx::execution::experimental {

    // Scheduling property implementations for execution policies are provided
    // through the public query() member functions on execution_policy (see
    // hpx/executors/execution_policy.hpp). The scheduling property CPOs detect
    // those members directly (via property_base), so no tag_invoke bridge is
    // needed here.
}    // namespace hpx::execution::experimental
