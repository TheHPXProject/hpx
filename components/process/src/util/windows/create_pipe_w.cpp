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

#include <hpx/components/process/util/windows/create_pipe.hpp>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::components::process::windows {

    pipe create_pipe()
    {
        HANDLE handles[2];
        if (!::CreatePipe(&handles[0], &handles[1], nullptr, 0))
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "posix::create_pipe", "CreatePipe() failed");
        }
        return make_pipe(handles[0], handles[1]);
    }

    pipe create_pipe(hpx::error_code& ec)
    {
        HANDLE handles[2] = {nullptr, nullptr};
        if (!::CreatePipe(&handles[0], &handles[1], nullptr, 0))
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "posix::create_pipe",
                "CreatePipe() failed");
        }
        else
        {
            ec = hpx::make_success_code();
        }
        return make_pipe(handles[0], handles[1]);
    }
}    // namespace hpx::components::process::windows

#include <hpx/config/warnings_suffix.hpp>

#endif
