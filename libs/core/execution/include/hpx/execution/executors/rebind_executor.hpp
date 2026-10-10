//  Copyright (c) 2016-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/executors/rebind_executor.hpp
///
/// \brief rebind_executor, rebind_executor_t and create_rebound_policy.
///
/// rebind_executor and rebind_executor_t are defined in rebind_policy.hpp,
/// next to the per-axis customization points built on top of them, and
/// create_rebound_policy is defined in create_rebound_policy.hpp. This
/// header includes both, so code that includes it directly keeps getting
/// everything it provided before.

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/executors/create_rebound_policy.hpp>
#include <hpx/execution/executors/rebind_policy.hpp>
