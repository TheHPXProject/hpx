////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming.hpp>

#include <hpx/performance_counters/counters_fwd.hpp>

namespace hpx::performance_counters {

    /// Install a new performance counter in a way, which will uninstall it
    /// automatically during shutdown.
    HPX_CXX_EXPORT HPX_EXPORT void install_counter(hpx::id_type const& id,
        counter_info const& info, error_code& ec = throws);
}    // namespace hpx::performance_counters
