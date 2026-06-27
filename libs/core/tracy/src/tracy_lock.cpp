//  Copyright (c) 2025-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_TRACY)

#include <hpx/tracy/tracy_lock.hpp>

#include <cstring>
#include <string>

#include <tracy/TracyC.h>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::tracy {

    lock_data create(char const* name) noexcept
    {
        // clang-format off
        lock_data data;
        TracyCLockAnnounce(data.context)
        if (name != nullptr)
        {
            TracyCLockCustomName(
                static_cast<__tracy_lockable_context_data*>(data.context),
                name, std::strlen(name))
        }
        return data;
        // clang-format on
    }

    lock_data create(std::string const& name) noexcept
    {
        // clang-format off
        lock_data data;
        TracyCLockAnnounce(data.context)
        TracyCLockCustomName(
            static_cast<__tracy_lockable_context_data*>(data.context),
            name.c_str(), name.size())
        return data;
        // clang-format on
    }

    void destroy(lock_data const& data) noexcept
    {
        TracyCLockTerminate(
            static_cast<__tracy_lockable_context_data*>(data.context))
    }

    bool lock_prepare(lock_data const& data) noexcept
    {
        // clang-format off
        auto const result = TracyCLockBeforeLock(
            static_cast<__tracy_lockable_context_data*>(data.context))
        return result ? true : false;
        // clang-format on
    }

    void lock_acquired(lock_data const& data) noexcept
    {
        TracyCLockAfterLock(
            static_cast<__tracy_lockable_context_data*>(data.context))
    }

    void lock_released(lock_data const& data) noexcept
    {
        TracyCLockAfterUnlock(
            static_cast<__tracy_lockable_context_data*>(data.context))
    }

    void lock_acquired(lock_data const& data, bool const acquired) noexcept
    {
        TracyCLockAfterTryLock(
            static_cast<__tracy_lockable_context_data*>(data.context), acquired)
    }
}    // namespace hpx::tracy

#endif
