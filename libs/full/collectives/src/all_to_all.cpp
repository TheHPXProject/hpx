//  Copyright (c) 2024-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/collectives/all_to_all.hpp>

namespace hpx::traits::communication {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    char const* communicator_data<all_to_all_tag>::name() noexcept
    {
        static char const* name = "all_to_all";
        return name;
    }
}    // namespace hpx::traits::communication

#endif
