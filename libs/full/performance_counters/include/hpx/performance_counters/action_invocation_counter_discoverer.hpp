//  Copyright (c) 2015-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>

#include <hpx/performance_counters/counters_fwd.hpp>

namespace hpx::performance_counters {

    HPX_CXX_EXPORT HPX_EXPORT bool action_invocation_counter_discoverer(
        hpx::actions::detail::invocation_count_registry const& registry,
        counter_info const& info, counter_path_elements& p,
        discover_counter_func const& f, discover_counters_mode mode,
        error_code& ec);
}    // namespace hpx::performance_counters
