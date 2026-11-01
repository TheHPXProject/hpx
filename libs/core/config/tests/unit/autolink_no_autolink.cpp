//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// Test with HPX_NO_AUTOLINK defined
#define HPX_NO_AUTOLINK
#include <hpx/modules/version.hpp>
#undef HPX_NO_AUTOLINK

#if defined(HPX_AUTOLINK_LIB_NAME)
#error "HPX_AUTOLINK_LIB_NAME should be undefined when HPX_NO_AUTOLINK is set"
#endif

void test_no_autolink() {}
