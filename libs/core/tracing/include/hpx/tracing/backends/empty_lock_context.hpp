//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

namespace hpx::tracing {

    ////////////////////////////////////////////////////////////////////////////
    // Used without tracing and by the Tracy and ITT backends when
    // HPX_WITH_TRACING_LOCK_EVENTS is OFF. The locks hold it as
    // HPX_NO_UNIQUE_ADDRESS, so it takes no space and every call folds away.
    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT struct [[maybe_unused]] lock_context
    {
        constexpr explicit lock_context(
            char const* = nullptr, void const* = nullptr) noexcept
        {
        }
        constexpr explicit lock_context(
            char const*, char const*, void const* = nullptr) noexcept
        {
        }

        constexpr bool before_lock() const noexcept
        {
            return false;
        }

        constexpr void after_lock() const noexcept {}

        constexpr void after_try_lock(bool) const noexcept {}

        constexpr void before_unlock() const noexcept {}

        constexpr void after_unlock() const noexcept {}
    };
    /// \endcond
}    // namespace hpx::tracing
