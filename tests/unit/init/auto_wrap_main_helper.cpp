//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Nothing HPX specific here. With HPX::auto_wrap_main on MSVC this file gets
// hpx/hpx_main.hpp force-included as well, which must not add another main().
int auto_wrap_main_helper()
{
    return 42;
}
