//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpxinspect:linelength
#pragma once

#include <hpx/config/defines.hpp>

#if defined(HPX_HAVE_DYNAMIC_HPX_MAIN)
#if (defined(__linux) || defined(__linux__) || defined(linux) ||               \
    defined(__APPLE__)) &&                                                     \
    defined(HPX_HAVE_STATIC_LINKING) &&                                        \
    !defined(HPX_HAVE_WRAP_MAIN_CONFIGURED)
#warning                                                                       \
    "You are statically linking HPX on Linux/macOS while using hpx_main.hpp. Please ensure you manually configure the linker to use wrap_main, or use the CMake target HPX::wrap_main to avoid linking errors."
#endif
#endif
