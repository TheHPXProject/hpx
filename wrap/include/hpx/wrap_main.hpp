//  Copyright (c) 2007-2022 Hartmut Kaiser
//  Copyright (c) 2018-2020 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100)
#endif

#include <hpx/hpx_init.hpp>

// We support different implementation depending upon the Operating
// System in use.
#if defined(HPX_HAVE_DYNAMIC_HPX_MAIN) &&                                      \
    (defined(__linux) || defined(__linux__) || defined(linux) ||               \
        defined(__APPLE__))

#include <string>

namespace hpx_start {
    // include_libhpx_wrap here is an override for the one present in
    // src/hpx_wrap.cpp. The value of this variable defines if we need
    // to change the program's entry point or not.
    HPX_SYMBOL_EXPORT extern bool include_libhpx_wrap;
    HPX_SYMBOL_EXPORT bool include_libhpx_wrap = true;
    HPX_SYMBOL_EXPORT extern const char* app_name_libhpx_wrap;
    HPX_SYMBOL_EXPORT const char* app_name_libhpx_wrap = HPX_APPLICATION_STRING;

    // This variable declaration is added solely for the purpose to trigger
    // a different runtime error when a user forgets to add HPX::wrap_main in
    // their CMakeLists.txt while using hpx_main.hpp functionality.
    // It will enable the user to know that he is required to link to
    // HPX::wrap_main instead of an orthogonal exception about thread not
    // registered with the HPX runtime system.
    HPX_SYMBOL_EXPORT bool is_linked __attribute__((weak)) = false;
}    // namespace hpx_start

#else

// HPX::auto_wrap_main on MSVC force-includes this into every translation unit,
// in which case main() is provided by hpx_wrap instead.
#if defined(HPX_HAVE_STATIC_LINKING) &&                                        \
    !defined(HPX_AUTO_WRAP_MAIN_FORCE_INCLUDE)
#include <hpx/hpx_main_impl.hpp>
#endif

// We support redefining the plain C-main provided by the user to be executed
// as the first HPX-thread (equivalent to hpx_main()). This is implemented by
// a macro redefining main, so we disable it by default.
#define main hpx_startup::user_main
#endif

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif
