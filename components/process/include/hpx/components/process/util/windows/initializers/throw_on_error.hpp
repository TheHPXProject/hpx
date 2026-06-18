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
#include <hpx/modules/errors.hpp>

#include <hpx/components/process/util/windows/initializers/initializer_base.hpp>

#include <string>

namespace hpx::components::process::windows::initializers {

    class throw_on_error : public initializer_base
    {
    public:
        template <class WindowsExecutor>
        void on_CreateProcess_error(WindowsExecutor&) const
        {
            HRESULT hr = GetLastError();
            LPVOID buffer = 0;
            if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr, hr,
                    MAKELANGID(LANG_NEUTRAL,
                        SUBLANG_DEFAULT),    // Default language
                    (LPTSTR) &buffer, 0, nullptr))
            {
                HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                    "process::on_CreateProcess_error",
                    "format message failed with {:x} (while retrieving "
                    "message for {:x})",
                    GetLastError(), hr);
            }

            std::string msg("CreateProcess() failed: ");
            msg += static_cast<char*>(buffer);
            LocalFree(buffer);
            HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                "process::on_CreateProcess_error", msg);
        }
    };
}    // namespace hpx::components::process::windows::initializers

#endif
