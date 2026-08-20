//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c) 2017      Denis Blank
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/debugging/attach_debugger.hpp>

#include <iostream>

#if defined(HPX_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HPX_WINDOWS)
#include <Windows.h>
#endif

#if defined(_POSIX_VERSION)
#include <asio/ip/host_name.hpp>
#endif

namespace hpx::util {

    void attach_debugger()
    {
#if defined(HPX_WINDOWS)
        DebugBreak();
#elif defined(_POSIX_VERSION) && defined(HPX_HAVE_UNISTD_H)
        volatile int i = 0;
        std::cerr << "PID: " << getpid() << " on " << ::asio::ip::host_name()
                  << " ready for attaching debugger. Once attached set i = 1 "
                     "and continue"
                  << std::endl;
        while (i == 0)
        {
            sleep(1);
        }
#endif
    }
}    // namespace hpx::util
