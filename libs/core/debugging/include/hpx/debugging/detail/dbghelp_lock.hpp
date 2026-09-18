//  Copyright (c) 2026 The STE||AR Group
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

// Windows-only. DbgHelp is single-threaded per MSDN and both HPX and
// Tracy call it. This header declares the shared lock; Tracy reaches
// it via TRACY_DBGHELP_LOCK=HpxDbgHelp in HPX_SetupTracy.cmake.
#if defined(HPX_WINDOWS)

namespace hpx::util::detail {

    HPX_CORE_EXPORT void dbghelp_lock();
    HPX_CORE_EXPORT void dbghelp_unlock();

    struct dbghelp_scoped_lock
    {
        dbghelp_scoped_lock()
        {
            dbghelp_lock();
        }
        ~dbghelp_scoped_lock()
        {
            dbghelp_unlock();
        }
        dbghelp_scoped_lock(dbghelp_scoped_lock const&) = delete;
        dbghelp_scoped_lock& operator=(dbghelp_scoped_lock const&) = delete;
    };
}    // namespace hpx::util::detail

#endif    // HPX_WINDOWS
