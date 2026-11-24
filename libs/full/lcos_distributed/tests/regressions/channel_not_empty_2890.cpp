//  Copyright (c) 2017 Element-126
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/modules/lcos_local.hpp>
#include <hpx/modules/testing.hpp>

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
#include <hpx/modules/lcos_distributed.hpp>

HPX_REGISTER_CHANNEL(int)
#endif

int main()
{
    {
        hpx::lcos::local::channel<int> ch;
        ch.set(0);
        ch.close(true);
    }

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
    {
        hpx::lcos::channel<int> ch(hpx::find_here());
        ch.set(0);
        ch.close(true);
    }
#endif

    return hpx::util::report_errors();
}
#endif
