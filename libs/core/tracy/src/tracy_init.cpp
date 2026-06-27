//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_TRACY)
#include <tracy/Tracy.hpp>

namespace hpx::tracy {

    struct tracy_init
    {
        tracy_init()
        {
            ::TracyNoop;
        }
    };

    tracy_init tracy_init_instance;
}    // namespace hpx::tracy

#endif
