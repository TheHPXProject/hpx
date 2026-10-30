//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// 1. Test generated module header inclusion (consumer path)
#include <hpx/modules/version.hpp>

#if defined(HPX_AUTOLINK_LIB_NAME)
#error                                                                         \
    "HPX_AUTOLINK_LIB_NAME should be undefined after including generated module header"
#endif

// 2. Test direct autolink header inclusion with custom lib name
#define HPX_NO_AUTOLINK
#define HPX_AUTOLINK_LIB_NAME "hpx_test"
#include <hpx/config/autolink.hpp>
#undef HPX_NO_AUTOLINK

#if defined(HPX_AUTOLINK_LIB_NAME)
#error                                                                         \
    "HPX_AUTOLINK_LIB_NAME should be undefined after including autolink.hpp directly"
#endif

#include <hpx/modules/testing.hpp>

void test_exports();
void test_no_autolink();
void test_no_lib();

int main()
{
    test_exports();
    test_no_autolink();
    test_no_lib();

    HPX_TEST(true);
    return hpx::util::report_errors();
}
