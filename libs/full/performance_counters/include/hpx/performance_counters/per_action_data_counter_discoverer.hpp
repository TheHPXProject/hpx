//  Copyright (c) 2015-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS) &&                            \
    defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/actions_base.hpp>

#include <hpx/performance_counters/counters_fwd.hpp>

namespace hpx::performance_counters {

    HPX_CXX_EXPORT HPX_EXPORT bool per_action_counter_counter_discoverer(
        hpx::actions::detail::per_action_data_counter_registry const& registry,
        performance_counters::counter_info const& info,
        performance_counters::counter_path_elements& p,
        performance_counters::discover_counter_func const& f,
        performance_counters::discover_counters_mode mode, error_code& ec);
}    // namespace hpx::performance_counters

#endif
