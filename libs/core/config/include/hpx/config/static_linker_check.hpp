//  Copyright (c) 2020-2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/config/static_linker_check.hpp
/// \brief Compile-time safety check for the -Wl,-wrap=main linker flag.
///
/// On Linux, when HPX is statically linked with dynamic main wrapping
/// enabled, the linker requires -Wl,-wrap=main to intercept main() and
/// bootstrap the HPX runtime. Without it, programs crash at startup.
///
/// This header emits a #warning when those conditions are detected and
/// the HPX_HAVE_WRAP_MAIN_CONFIGURED define is absent. CMake consumers
/// get the define injected automatically via the hpx_wrap target's
/// INTERFACE compile definitions, so they never see this warning.

#pragma once

#include <hpx/config/defines.hpp>

#if defined(__linux__) && defined(HPX_HAVE_DYNAMIC_HPX_MAIN) &&                \
    defined(HPX_HAVE_STATIC_LINKING) &&                                        \
    !defined(HPX_HAVE_WRAP_MAIN_CONFIGURED)
// clang-format off
#warning "HPX: Static Linux build detected. The linker requires '-Wl,-wrap=main' to properly intercept main(). If you are linking manually, add this flag to your linker command. Define 'HPX_HAVE_WRAP_MAIN_CONFIGURED' to suppress this warning."
// clang-format on
#endif
