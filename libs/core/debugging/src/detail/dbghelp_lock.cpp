//  Copyright (c) 2026 The STE||AR Group
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)

#include <hpx/debugging/detail/dbghelp_lock.hpp>

#include <mutex>

namespace {

    // Function-local static so the mutex is constructed on first use,
    // with no static-init ordering question across DLL boundaries.
    // Recursive because re-entry from a Tracy -S zone or a DbgHelp
    // callback registered later would otherwise deadlock the calling
    // thread against a lock it already holds.
    std::recursive_mutex& dbghelp_mutex() noexcept
    {
        static std::recursive_mutex m;
        return m;
    }

}    // namespace

// PREfast flags each of these functions for an unpaired lock/unlock. That
// is the contract, not a bug. Match the suppression pattern used in
// libs/core/futures/include/hpx/futures/detail/future_data.hpp.
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26110 26111 26115 26117)
#endif

namespace hpx::util::detail {

    void dbghelp_lock()
    {
        dbghelp_mutex().lock();
    }

    void dbghelp_unlock()
    {
        dbghelp_mutex().unlock();
    }
}    // namespace hpx::util::detail

// Tracy interop entry points. TRACY_DBGHELP_LOCK=HpxDbgHelp in
// HPX_SetupTracy.cmake makes Tracy expand its DbgHelp lock macros
// to these three symbols. Init is empty; first-use construction of
// the mutex above covers the same job. noexcept here (unlike the C++
// wrappers) because an exception escaping a C-linkage boundary is UB;
// terminate is the well-defined failure we want on this side.
extern "C" HPX_CORE_EXPORT void HpxDbgHelpInit(void) noexcept {}

extern "C" HPX_CORE_EXPORT void HpxDbgHelpLock(void) noexcept
{
    dbghelp_mutex().lock();
}

extern "C" HPX_CORE_EXPORT void HpxDbgHelpUnlock(void) noexcept
{
    dbghelp_mutex().unlock();
}

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

#endif    // HPX_WINDOWS
