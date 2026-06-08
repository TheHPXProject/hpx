//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/topology/topology.hpp>

namespace hpx::execution::experimental::detail {

    struct machine_affinity_mask_fallback
    {
        template <typename Target, typename... Args>
        HPX_FORCEINLINE decltype(auto) operator()(
            Target&&, Args&&...) const noexcept
        {
            return hpx::threads::create_topology().get_machine_affinity_mask();
        }
    };

}    // namespace hpx::execution::experimental::detail
