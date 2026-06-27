//  Copyright (c) 2022-2025 Hartmut Kaiser
//  Copyright (c) 2026 Sai Charan Arvapally
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/execution_policy_parameters.hpp
///
/// Policy forwarding for executor-parameter CPOs now lives on the CPOs
/// themselves (see execution_parameters_fwd.hpp, e.g. processing_units_count_t
/// policy overload and detail::forward_to_policy_executor). Parameter attachment
/// to policies continues to use execution_policy::with() directly.

#pragma once

#include <hpx/config.hpp>
