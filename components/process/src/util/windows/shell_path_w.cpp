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

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)
#include <hpx/modules/errors.hpp>
#include <hpx/modules/filesystem.hpp>

#include <hpx/components/process/util/windows/shell_path.hpp>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::components::process::windows {

    filesystem::path shell_path()
    {
        TCHAR sysdir[MAX_PATH];
        UINT size = ::GetSystemDirectory(sysdir, sizeof(sysdir));
        if (!size)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "process::shell_path", "GetSystemDirectory() failed");
        }
        filesystem::path p = sysdir;
        return p / "cmd.exe";
    }

    filesystem::path shell_path(hpx::error_code& ec)
    {
        TCHAR sysdir[MAX_PATH];
        UINT size = ::GetSystemDirectory(sysdir, sizeof(sysdir));
        filesystem::path p;
        if (!size)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "process::shell_path",
                "GetSystemDirectory() failed");
        }
        else
        {
            ec = hpx::make_success_code();
            p = sysdir;
            p /= "cmd.exe";
        }
        return p;
    }
}    // namespace hpx::components::process::windows

#include <hpx/config/warnings_suffix.hpp>

#endif
