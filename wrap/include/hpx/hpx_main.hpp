//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config/static_linker_check.hpp>
#include <hpx/wrap_main.hpp>

#if defined(HPX_HAVE_RUN_MAIN_EVERYWHERE)

namespace hpx_startup {

    void install_user_main_config();

    struct register_user_main_config
    {
        register_user_main_config()
        {
            install_user_main_config();
        }
    };

    inline register_user_main_config cfg;
}    // namespace hpx_startup

#endif
