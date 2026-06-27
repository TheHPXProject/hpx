//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2007-2009 Chirag Dekate, Anshul Tandon
//  Copyright (c)      2011 Bryce Lelbach, Katelyn Kufahl
//  Copyright (c)      2017 Shoshana Jakobovits
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/threadmanager.hpp>

namespace hpx::performance_counters {

    HPX_CXX_EXPORT HPX_EXPORT void register_threadmanager_counter_types(
        threads::threadmanager& tm);
}    // namespace hpx::performance_counters
