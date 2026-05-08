//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpxinspect:linelength
#pragma once

#include <hpx/config/defines.hpp>

// Emit a compile-time diagnostic when the user includes hpx/hpx_main.hpp
// while building a statically-linked HPX application on Linux or macOS.
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
// This check is intentionally limited to Linux/macOS static builds because:
//   * On Windows the wrap mechanism is not used (MSVC uses a different ABI).
//   * Dynamic (shared-library) builds already embed the wrap stub inside
//     libhpx.so/dylib, so no extra linker flag is needed.

#if defined(HPX_HAVE_DYNAMIC_HPX_MAIN)
#if (defined(__linux) || defined(__linux__) || defined(linux) ||               \
    defined(__APPLE__)) &&                                                     \
    defined(HPX_HAVE_STATIC_LINKING) &&                                        \
    !defined(HPX_HAVE_WRAP_MAIN_CONFIGURED)
#warning                                                                       \
    "HPX static-link wrap-main check: you included hpx/hpx_main.hpp but the " \
    "--wrap=main linker flag has not been applied. Add "                       \
    "target_link_libraries(<target> PRIVATE HPX::wrap_main) to your "         \
    "CMakeLists.txt, or pass -Wl,--wrap=main to the linker manually. "        \
    "Without this flag the HPX runtime will not be initialised correctly."
#endif
#endif
