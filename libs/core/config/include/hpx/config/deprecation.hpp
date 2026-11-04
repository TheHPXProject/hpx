//  Copyright (c) 2020-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config/attributes.hpp>
#include <hpx/config/defines.hpp>
#include <hpx/config/version.hpp>
#include <hpx/modules/preprocessor.hpp>

///////////////////////////////////////////////////////////////////////////////
// Deprecate a given functionality starting HPX V1.9
#if !defined(HPX_HAVE_DEPRECATION_WARNINGS_V1_9)
#define HPX_HAVE_DEPRECATION_WARNINGS_V1_9 1
#endif

#if (HPX_VERSION_FULL >= 0x010900) && (HPX_HAVE_DEPRECATION_WARNINGS_V1_9 != 0)
#define HPX_DEPRECATED_MSG_V1_9                                                \
    "This functionality is deprecated starting HPX V1.9 and will be removed "  \
    "in the future. You can define HPX_HAVE_DEPRECATION_WARNINGS_V1_9=0 to "   \
    "acknowledge that you have received this warning."
#define HPX_DEPRECATED_V1_9(x)                                                 \
    [[deprecated(x " (" HPX_PP_EXPAND(HPX_DEPRECATED_MSG_V1_9) ")")]]
#endif

#if !defined(HPX_DEPRECATED_V1_9)
#define HPX_DEPRECATED_V1_9(x)
#endif

///////////////////////////////////////////////////////////////////////////////
// Deprecate a given functionality starting HPX V1.10
#if !defined(HPX_HAVE_DEPRECATION_WARNINGS_V1_10)
#define HPX_HAVE_DEPRECATION_WARNINGS_V1_10 1
#endif

#if (HPX_VERSION_FULL >= 0x011000) && (HPX_HAVE_DEPRECATION_WARNINGS_V1_10 != 0)
#define HPX_DEPRECATED_MSG_V1_10                                               \
    "This functionality is deprecated starting HPX V1.10 and will be removed " \
    "in the future. You can define HPX_HAVE_DEPRECATION_WARNINGS_V1_10=0 to "  \
    "acknowledge that you have received this warning."
#define HPX_DEPRECATED_V1_10(x)                                                \
    [[deprecated(x " (" HPX_PP_EXPAND(HPX_DEPRECATED_MSG_V1_10) ")")]]
#endif

#if !defined(HPX_DEPRECATED_V1_10)
#define HPX_DEPRECATED_V1_10(x)
#endif

///////////////////////////////////////////////////////////////////////////////
// Deprecate a given functionality starting HPX V1.11
#if !defined(HPX_HAVE_DEPRECATION_WARNINGS_V1_11)
#define HPX_HAVE_DEPRECATION_WARNINGS_V1_11 1
#endif

#if (HPX_VERSION_FULL >= 0x011100) && (HPX_HAVE_DEPRECATION_WARNINGS_V1_11 != 0)
#define HPX_DEPRECATED_MSG_V1_11                                               \
    "This functionality is deprecated starting HPX V1.11 and will be removed " \
    "in the future. You can define HPX_HAVE_DEPRECATION_WARNINGS_V1_11=0 to "  \
    "acknowledge that you have received this warning."
#define HPX_DEPRECATED_V1_11(x)                                                \
    [[deprecated(x " (" HPX_PP_EXPAND(HPX_DEPRECATED_MSG_V1_11) ")")]]
#endif

#if !defined(HPX_DEPRECATED_V1_11)
#define HPX_DEPRECATED_V1_11(x)
#endif

///////////////////////////////////////////////////////////////////////////////
// Deprecate a given functionality starting HPX V2.0
#if !defined(HPX_HAVE_DEPRECATION_WARNINGS_V2_0)
#define HPX_HAVE_DEPRECATION_WARNINGS_V2_0 1
#endif

#if (HPX_VERSION_FULL >= 0x020000) && (HPX_HAVE_DEPRECATION_WARNINGS_V2_0 != 0)
#define HPX_DEPRECATED_MSG_V2_0                                                \
    "This functionality is deprecated starting HPX V2.0 and will be removed "  \
    "in the future. You can define HPX_HAVE_DEPRECATION_WARNINGS_V2_0=0 to "   \
    "acknowledge that you have received this warning."
#define HPX_DEPRECATED_V2_0(x)                                                 \
    [[deprecated(x " (" HPX_PP_EXPAND(HPX_DEPRECATED_MSG_V2_0) ")")]]
#endif

#if !defined(HPX_DEPRECATED_V2_0)
#define HPX_DEPRECATED_V2_0(x)
#endif

///////////////////////////////////////////////////////////////////////////////
// Deprecate a given functionality starting HPX V2.1
#if !defined(HPX_HAVE_DEPRECATION_WARNINGS_V2_1)
#define HPX_HAVE_DEPRECATION_WARNINGS_V2_1 1
#endif

#if (HPX_VERSION_FULL >= 0x020100) && (HPX_HAVE_DEPRECATION_WARNINGS_V2_1 != 0)
#define HPX_DEPRECATED_MSG_V2_1                                                \
    "This functionality is deprecated starting HPX V2.1 and will be removed "  \
    "in the future. You can define HPX_HAVE_DEPRECATION_WARNINGS_V2_1=0 to "   \
    "acknowledge that you have received this warning."
#define HPX_DEPRECATED_V2_1(x)                                                 \
    [[deprecated(x " (" HPX_PP_EXPAND(HPX_DEPRECATED_MSG_V2_1) ")")]]
#endif

#if !defined(HPX_DEPRECATED_V2_1)
#define HPX_DEPRECATED_V2_1(x)
#endif

///////////////////////////////////////////////////////////////////////////////
// Deprecate a given functionality starting at the given version of HPX
#define HPX_DEPRECATED_V(major, minor, x)                                      \
    HPX_PP_CAT(HPX_PP_CAT(HPX_PP_CAT(HPX_DEPRECATED_V, major), _), minor)(x)
