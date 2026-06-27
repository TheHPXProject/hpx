//  Copyright (c) 2025-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_TRACY)

#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::tracy {

    HPX_CXX_CORE_EXPORT struct lock_data
    {
        void* context = nullptr;
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT lock_data create(
        char const* name = nullptr) noexcept;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT lock_data create(
        std::string const& name) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void destroy(lock_data const&) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool lock_prepare(
        lock_data const&) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void lock_acquired(
        lock_data const&) noexcept;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void lock_acquired(
        lock_data const&, bool acquired) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void lock_released(
        lock_data const&) noexcept;
}    // namespace hpx::tracy

#endif
