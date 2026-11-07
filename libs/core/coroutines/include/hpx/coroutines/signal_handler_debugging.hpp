//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <string>

namespace hpx::threads::coroutines {

    HPX_CORE_EXPORT extern bool attach_debugger_on_sigv;
    HPX_CORE_EXPORT extern bool diagnostics_on_terminate;
    HPX_CORE_EXPORT extern int exception_verbosity;
    HPX_CORE_EXPORT extern std::size_t trace_depth;
#if !defined(HPX_WINDOWS)
    HPX_CORE_EXPORT extern bool register_signal_handler;

    // The build string the signal handlers print, built when they are
    // installed (see hpx::set_error_handlers): building it in a handler may
    // need locks held by the thread that received the signal
    HPX_CORE_EXPORT extern std::string build_string_on_terminate;

    /// Writes \a text to stderr with write(), which, unlike std::cerr, is
    /// safe to use in a signal handler.
    HPX_CORE_EXPORT void write_on_terminate(char const* text) noexcept;

    /// Lets the default action of SIGALRM end the process after a few
    /// seconds, in case printing the diagnostics of a signal handler blocks
    /// on a lock held by the thread that received the signal.
    HPX_CORE_EXPORT void start_signal_handler_watchdog() noexcept;
#endif
}    // namespace hpx::threads::coroutines
