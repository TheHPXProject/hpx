//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// This header may be force-included into all sources of a target (see
// HPX::auto_wrap_main on MSVC), so make it a no-op for non-C++ sources.
#if defined(__cplusplus)

#include <hpx/config.hpp>
#include <hpx/wrap_main.hpp>

// Emit a compile-time diagnostic when the user includes hpx/hpx_main.hpp
// on Linux without linking against HPX::wrap_main.
//
// Root cause: the '--wrap=main' linker flag, which redirects user 'main' to
// the HPX runtime entry-point, is only applied automatically when the user
// links against 'HPX::wrap_main'. Without it, the resulting binary will call
// the raw 'main' symbol, bypassing HPX initialisation and producing a
// hard-to-diagnose runtime crash or silent hang.
//
// Actionable remedies (pick one):
//   CMake  -- add target_link_libraries(<target> PRIVATE HPX::wrap_main)
//   Manual -- pass -Wl,--wrap=main to the linker explicitly
//
// This check is intentionally limited to Linux because the --wrap linker
// mechanism is only used on Linux.
#if defined(HPX_HAVE_DYNAMIC_HPX_MAIN) &&                                      \
    (defined(__linux) || defined(__linux__) || defined(linux)) &&              \
    !defined(HPX_HAVE_WRAP_MAIN_CONFIGURED)
#warning "HPX wrap-main check: you included hpx/hpx_main.hpp but the "             \
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

#endif
