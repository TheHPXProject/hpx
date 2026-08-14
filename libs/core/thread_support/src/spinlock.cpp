////////////////////////////////////////////////////////////////////////////////
//  Copyright 2008, 2020 Peter Dimov
//  Copyright (c) 2020 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#include <hpx/config.hpp>
#include <hpx/thread_support/spinlock.hpp>
#include <hpx/thread_support/win_nanosleep.hpp>

#include <chrono>
#include <thread>

#if !defined(HPX_WINDOWS)
#ifndef _AIX
#include <sched.h>
#else
// AIX's sched.h defines ::var which sometimes conflicts with Lambda's var
extern "C" int sched_yield(void);
#endif
#include <time.h>
#endif

namespace hpx::util::detail {

    bool yield_k_backoff(unsigned const k) noexcept
    {
        if (k < 4)    //-V112
        {
            return false;
        }
        if (k < 16)
        {
            HPX_SMT_PAUSE;
            return false;
        }

        if (k < 32 || k & 1)    //-V112
        {
#if defined(HPX_WINDOWS)
            hpx::util::win_nanosleep(hpx::util::wait_0ns);
#else
            sched_yield();
#endif
        }
        else
        {
#if defined(HPX_WINDOWS)
            hpx::util::win_nanosleep(hpx::util::wait_1000ns);
#else
            // g++ -Wextra warns on {} or {0}
            struct timespec rqtp = {0, 0};

            // POSIX says that timespec has tv_sec and tv_nsec
            // But it doesn't guarantee order or placement

            rqtp.tv_sec = 0;
            rqtp.tv_nsec = 1000;

            nanosleep(&rqtp, nullptr);
#endif
        }
        return true;
    }
}    // namespace hpx::util::detail
