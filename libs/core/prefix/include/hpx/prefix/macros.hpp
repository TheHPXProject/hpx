//  Copyright (c) 2012 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The HPX runtime needs to know where to look for the HPX ini files if no ini
// path is specified by the user (default in $HPX_LOCATION/share/hpx-1.0.0/ini).
// Also, the default component path is set within the same prefix.

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/preprocessor.hpp>

// clang-format off
#define HPX_BASE_DIR_NAME                                                      \
    "hpx-"                                                                     \
    HPX_PP_STRINGIZE(HPX_VERSION_MAJOR) "."                                    \
    HPX_PP_STRINGIZE(HPX_VERSION_MINOR) "."                                    \
    HPX_PP_STRINGIZE(HPX_VERSION_SUBMINOR)                                     \
/**/
// clang-format on

#if !defined(HPX_DEFAULT_INI_PATH)
#define HPX_DEFAULT_INI_PATH                                                   \
    ::hpx::util::find_prefixes("/share/" HPX_BASE_DIR_NAME "/ini") /**/
#endif
#if !defined(HPX_DEFAULT_INI_FILE)
#define HPX_DEFAULT_INI_FILE                                                   \
    ::hpx::util::find_prefixes("/share/" HPX_BASE_DIR_NAME "/hpx.ini") /**/
#endif
#if !defined(HPX_DEFAULT_COMPONENT_PATH)
#define HPX_DEFAULT_COMPONENT_PATH ::hpx::util::find_prefixes("/hpx") /**/
#endif
