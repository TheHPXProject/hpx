//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/coroutines/signal_handler_debugging.hpp>

#include <cstddef>

#if !defined(HPX_WINDOWS)
#include <cerrno>
#include <cstring>
#include <string>

#include <signal.h>
#include <unistd.h>
#endif

namespace hpx::threads::coroutines {

    bool attach_debugger_on_sigv = false;
    bool diagnostics_on_terminate = true;
    int exception_verbosity = 2;
#if defined(HPX_HAVE_STACKTRACES) && defined(HPX_HAVE_THREAD_BACKTRACE_DEPTH)
    std::size_t trace_depth = HPX_HAVE_THREAD_BACKTRACE_DEPTH;
#else
    std::size_t trace_depth = 0;
#endif
#if !defined(HPX_WINDOWS)
    bool register_signal_handler = true;
    std::string build_string_on_terminate;

    void write_on_terminate(char const* text) noexcept
    {
        std::size_t size = std::strlen(text);
        while (size != 0)
        {
            ssize_t const written = ::write(STDERR_FILENO, text, size);
            if (written < 0 && errno == EINTR)
            {
                continue;
            }
            if (written <= 0)
            {
                return;
            }
            text += written;
            size -= static_cast<std::size_t>(written);
        }
    }

    void start_signal_handler_watchdog() noexcept
    {
        // the default action of SIGALRM ends the process; unblock it on the
        // thread running the handler, so that the alarm is delivered even if
        // the application blocks SIGALRM
        ::signal(SIGALRM, SIG_DFL);
        sigset_t alarm_set;
        sigemptyset(&alarm_set);
        sigaddset(&alarm_set, SIGALRM);
        ::pthread_sigmask(SIG_UNBLOCK, &alarm_set, nullptr);
        ::alarm(10);
    }
#endif
}    // namespace hpx::threads::coroutines
