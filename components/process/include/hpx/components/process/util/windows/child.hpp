// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)
#include <cstring>
#include <utility>

namespace hpx::components::process::windows {

    class child
    {
    public:
        PROCESS_INFORMATION proc_info;

        child() noexcept
        {
            std::memset(&proc_info, '\0', sizeof(PROCESS_INFORMATION));
            proc_info.hProcess = INVALID_HANDLE_VALUE;
            proc_info.hThread = INVALID_HANDLE_VALUE;
        }

        explicit child(const PROCESS_INFORMATION& pi) noexcept
          : proc_info(pi)
        {
        }

        ~child()
        {
            if (proc_info.hProcess != INVALID_HANDLE_VALUE)
                ::CloseHandle(proc_info.hProcess);
            if (proc_info.hThread != INVALID_HANDLE_VALUE)
                ::CloseHandle(proc_info.hThread);
        }

        child(child&& c) noexcept
          : proc_info(c.proc_info)
        {
            c.proc_info.hProcess = INVALID_HANDLE_VALUE;
            c.proc_info.hThread = INVALID_HANDLE_VALUE;
        }

        child& operator=(child&& c) noexcept
        {
            if (this == &c)
            {
                if (proc_info.hProcess != INVALID_HANDLE_VALUE)
                    ::CloseHandle(proc_info.hProcess);
                if (proc_info.hThread != INVALID_HANDLE_VALUE)
                    ::CloseHandle(proc_info.hThread);

                proc_info = c.proc_info;
                c.proc_info.hProcess = INVALID_HANDLE_VALUE;
                c.proc_info.hThread = INVALID_HANDLE_VALUE;
            }
            return *this;
        }

        constexpr HANDLE process_handle() const noexcept
        {
            return proc_info.hProcess;
        }
    };
}    // namespace hpx::components::process::windows

#endif
