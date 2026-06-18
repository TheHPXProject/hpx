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

namespace hpx::components::process::windows {

    template <typename Process>
    int wait_for_exit(Process const& p)
    {
        if (::WaitForSingleObject(p.process_handle(), INFINITE) == WAIT_FAILED)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "process::wait_for_exit", "WaitForSingleObject() failed");
        }

        DWORD exit_code;
        if (!::GetExitCodeProcess(p.process_handle(), &exit_code))
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "process::wait_for_exit", "GetExitCodeProcess() failed");
        }
        return static_cast<int>(exit_code);
    }

    template <typename Process>
    int wait_for_exit(Process const& p, hpx::error_code& ec)
    {
        DWORD exit_code = 1;

        if (::WaitForSingleObject(p.process_handle(), INFINITE) == WAIT_FAILED)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "process::wait_for_exit", "WaitForSingleObject() failed");
        }
        else if (!::GetExitCodeProcess(p.process_handle(), &exit_code))
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "process::wait_for_exit", "GetExitCodeProcess() failed");
        }
        else
        {
            ec = hpx::make_success_code();
        }
        return static_cast<int>(exit_code);
    }
}    // namespace hpx::components::process::windows

#endif
