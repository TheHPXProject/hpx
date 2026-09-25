//  Copyright (c) 2005-2014 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This file can be included multiple times and does not use #pragma once.
// hpxinspect:nopragmaonce

#include <hpx/config/compiler_specific.hpp>
#include <hpx/config/debug.hpp>
#include <hpx/modules/preprocessor.hpp>

// enable auto-linking for supported platforms
#if defined(HPX_MSVC) || defined(__BORLANDC__) ||                              \
    (defined(__MWERKS__) && defined(_WIN32) && (__MWERKS__ >= 0x3000)) ||      \
    (defined(__ICL) && defined(_MSC_EXTENSIONS) && (HPX_MSVC >= 1200))

#if !defined(HPX_AUTOLINK_LIB_NAME)
#error "Macro HPX_AUTOLINK_LIB_NAME not set (internal error)"
#endif

#if !defined(HPX_NO_AUTOLINK) && !defined(HPX_NO_LIB)

#if defined(HPX_DEBUG) && defined(HPX_HAVE_DEBUG_POSTFIX)
#pragma comment(lib,                                                           \
    HPX_AUTOLINK_LIB_NAME HPX_PP_STRINGIZE(HPX_HAVE_DEBUG_POSTFIX) ".lib")
#elif defined(HPX_DEBUG)
#pragma comment(lib,                                                           \
    HPX_AUTOLINK_LIB_NAME "d"                                                  \
                          ".lib")
#else
#pragma comment(lib, HPX_AUTOLINK_LIB_NAME ".lib")
#endif

#endif    // !HPX_NO_AUTOLINK && !HPX_NO_LIB

#endif

#undef HPX_AUTOLINK_LIB_NAME
