//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/wrap_main.hpp>

#if defined(HPX_HAVE_DYNAMIC_HPX_MAIN) && defined(HPX_HAVE_STATIC_LINKING) &&  \
    (defined(__linux) || defined(__linux__) || defined(linux)) &&              \
    !defined(HPX_HAVE_WRAP_MAIN_CONFIGURED)
#warning                                                                       \
    "HPX static-link wrap-main check: you included hpx/hpx_main.hpp but the " \
    "--wrap=main linker flag has not been applied. Add "                       \
    "target_link_libraries(<target> PRIVATE HPX::wrap_main) to your "         \
    "CMakeLists.txt, or pass -Wl,--wrap=main to the linker manually. "        \
    "Without this flag the HPX runtime will not be initialised correctly."
#endif

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
