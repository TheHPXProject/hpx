//  Copyright (c) 2012-2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config/compiler_specific.hpp>

#if defined(DOXYGEN)
/// Marks a function to be forced inline.
#define HPX_FORCEINLINE
#else

// clang-format off
#if !defined(HPX_FORCEINLINE)
#   if defined(HPX_DEBUG)
#       define HPX_FORCEINLINE inline
#   elif defined(__NVCC__) || defined(__CUDACC__)
#       define HPX_FORCEINLINE inline
#   elif defined(HPX_MSVC)
#       define HPX_FORCEINLINE __forceinline
#   elif defined(__GNUC__)
#       define HPX_FORCEINLINE inline __attribute__ ((__always_inline__))
#   else
#       define HPX_FORCEINLINE inline
#   endif
#endif
// clang-format on
#endif
