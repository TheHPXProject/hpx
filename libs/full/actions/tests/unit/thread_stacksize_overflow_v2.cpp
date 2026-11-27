// Copyright (C) 2012 Hartmut Kaiser
// Copyright (C) 2017 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>

int main()
{
    // A null pointer dereference in an HPX thread is not a stack overflow and
    // must not be reported as one. The pointer is volatile so that the
    // compiler has to emit the load instead of replacing it with a trap.
    int* volatile i = nullptr;
    return *i;
}
#endif
