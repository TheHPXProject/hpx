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
#include <asio/windows/stream_handle.hpp>
#else
#include <asio/posix/stream_descriptor.hpp>
#endif

namespace hpx::components::process::util {

#if defined(HPX_WINDOWS)
    using pipe_end = ::asio::windows::stream_handle;
#else
    using pipe_end = ::asio::posix::stream_descriptor;
#endif

    inline const char* zero_device()
    {
#if defined(HPX_WINDOWS)
        return "NUL";
#else
        return "/dev/zero";
#endif
    }

    inline char const* null_device()
    {
#if defined(HPX_WINDOWS)
        return "NUL";
#else
        return "/dev/null";
#endif
    }
}    // namespace hpx::components::process::util
