//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/performance_counters/counter_interface.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/detail/counter_interface_functions.hpp>

namespace hpx::performance_counters {

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<id_type> create_performance_counter_async(
        id_type const& target_id, counter_info const& info)
    {
        return detail::create_performance_counter_async(target_id, info);
    }
}    // namespace hpx::performance_counters
