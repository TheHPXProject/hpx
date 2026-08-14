//  Copyright (c) 2024-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/collectives/exclusive_scan.hpp>

namespace hpx::traits::communication {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    char const* communicator_data<exclusive_scan_tag>::name() noexcept
    {
        static char const* name = "exclusive_scan";
        return name;
    }

    char const* communicator_data<exclusive_scan_init_tag>::name() noexcept
    {
        static char const* name = "exclusive_scan_init";
        return name;
    }
}    // namespace hpx::traits::communication

#endif
