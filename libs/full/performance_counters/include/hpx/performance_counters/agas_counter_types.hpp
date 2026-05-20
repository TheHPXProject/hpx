//  Copyright (c) 2011-2021 Hartmut Kaiser
//  Copyright (c) 2016 Parsa Amini
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/agas.hpp>

namespace hpx { namespace performance_counters {

    /// Install performance counter types exposing properties from the local
    /// cache.
    void HPX_EXPORT register_agas_counter_types(
        agas::addressing_service& client);
}}    // namespace hpx::performance_counters
