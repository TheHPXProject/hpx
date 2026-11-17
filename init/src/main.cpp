//  Copyright (c) 2007-2014 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// We support different implementation depending upon the Operating
// System in use.
#if !((defined(__linux) || defined(__linux__) || defined(linux) ||             \
          defined(__APPLE__)) &&                                               \
    !defined(__ANDROID__))

// when statically linking the default definition of main happens on demand only
#if !defined(HPX_HAVE_STATIC_LINKING)
#include <hpx/hpx_main_impl.hpp>
#endif

#endif
