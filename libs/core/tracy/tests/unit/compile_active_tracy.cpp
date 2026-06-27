//  Copyright (c) 2026 The STE||AR Group
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This compile-only test verifies that HPX_HAVE_TRACY is defined whenever the
// tracy module is built (i.e., -DHPX_WITH_TRACY=ON). It ensures the full
// CMake -> hpx_add_config_define -> config header chain is intact.

#include <hpx/config.hpp>

#ifndef HPX_HAVE_TRACY
#error "HPX_HAVE_TRACY is not defined. \
Check that HPX_SetupTracing.cmake calls hpx_add_config_define(HPX_HAVE_TRACY) \
when HPX_WITH_TRACY=ON."
#endif

int main()
{
    return 0;
}
