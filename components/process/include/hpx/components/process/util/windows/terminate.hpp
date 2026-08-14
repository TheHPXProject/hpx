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

#include <cstdlib>

namespace hpx::components::process::windows {

    template <typename Process>
    void terminate(Process const& p)
    {
        if (!::TerminateProcess(p.process_handle(), EXIT_FAILURE))
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "process::terminate", "TerminateProcess() failed");
        }
    }

    template <typename Process>
    void terminate(Process const& p, hpx::error_code& ec)
    {
        if (!::TerminateProcess(p.process_handle(), EXIT_FAILURE))
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "process::terminate",
                "TerminateProcess() failed");
        }
        else
        {
            ec = hpx::make_success_code();
        }
    }
}    // namespace hpx::components::process::windows

#endif
