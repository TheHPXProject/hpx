//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)
#include <hpx/thread_support/win_nanosleep.hpp>

#include <ctime>
#include <windows.h>

namespace hpx::util {

    namespace {

        // Number of performance counter increments per nanosecond, or zero if
        // it could not be determined.
        struct ticks_per_nanosecond
        {
            ticks_per_nanosecond() noexcept
            {
                LARGE_INTEGER freq;
                if (QueryPerformanceFrequency(&freq))
                    ticks = static_cast<double>(freq.QuadPart) / 1e9;
            }

            double ticks = 0.0;
        };
        ticks_per_nanosecond ticks;
    }    // namespace

    // our own (crude) implementation of nanosleep
    int win_nanosleep(std::timespec const& delay)
    {
        if (delay.tv_sec < 0 || delay.tv_nsec < 0 ||
            +delay.tv_nsec >= 1000000000)
        {
            return -1;
        }

        // zero delay simply yields to another thread
        if (delay.tv_sec == 0 && delay.tv_nsec == 0)
        {
            Sleep(0);
            return 0;
        }

        // for small delays we busy-wait
        if (delay.tv_sec == 0 && ticks.ticks != 0.0)
        {
            // compensate for fluctuations introduced by Sleep()
            auto const sleep_for = delay.tv_nsec / 1000000 - 10;

            // overall number of ticks to delay
            auto const ticks_to_delay = static_cast<long>(
                static_cast<double>(delay.tv_nsec) * ticks.ticks);

            LARGE_INTEGER counter;
            if (QueryPerformanceCounter(&counter))
            {
                // wait until the performance counter has reached this value
                auto const wait_until = counter.QuadPart + ticks_to_delay;

                // use Sleep() if appropriate
                if (sleep_for > 0)
                {
                    Sleep(sleep_for);
                }

                // simply busy-wait for the remaining amount of time, don't
                // wait if delay is zero
                while (counter.QuadPart < wait_until &&
                    QueryPerformanceCounter(&counter))
                {
                }
                return 0;
            }
        }

        // Fallback and longer delays
        Sleep(static_cast<long>(delay.tv_sec) * 1000 + delay.tv_nsec / 1000000);

        return 0;
    }
}    // namespace hpx::util

#endif
