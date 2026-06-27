//  Copyright (c) 2025-2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_TRACY)

#include <cstddef>
#include <cstdint>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::tracy {

    HPX_CXX_CORE_EXPORT struct region_data
    {
        char const* name = nullptr;
        std::uint64_t data = 0;
        std::uint32_t color = 0;
        std::uint32_t phase = 0;
    };

    namespace detail {

        HPX_CORE_EXPORT region_data start_region(
            char const*, std::size_t = 0, std::size_t = 0) noexcept;
        HPX_CORE_EXPORT char const* rename_region(char const*) noexcept;
        HPX_CORE_EXPORT region_data stop_region(
            region_data const& prev_region) noexcept;
        // Set/clear the per-OS-thread "inside fiber" flag used to guard
        // rename_region from operating on a stale zone ctx.
        HPX_CORE_EXPORT void set_in_fiber(bool) noexcept;

        // Open/close a zone on the fiber's zone stack so that the fiber track
        // is visible in Tracy.  Must be called after/before
        // TracyFiberEnter/Leave.
        HPX_CORE_EXPORT void start_fiber_zone(
            char const* zone_name, std::uint32_t color = 0) noexcept;
        HPX_CORE_EXPORT void stop_fiber_zone() noexcept;

        // Suspend/resume the zone that is currently open on the fiber's zone
        // stack. Call suspend_fiber_zone() just before self_.yield() and
        // resume_fiber_zone() immediately after self_.yield() returns.
        // Both operate solely on current_fiber_zone() - they never touch the
        // OS-thread zone (current_region()), so there is no zone-stack
        // conflict.
        HPX_CORE_EXPORT void suspend_fiber_zone(
            char const* suspend_reason = nullptr) noexcept;
        HPX_CORE_EXPORT void resume_fiber_zone(
            char const* zone_name = nullptr, std::uint32_t color = 0) noexcept;

    }    // namespace detail

    HPX_CXX_CORE_EXPORT struct region
    {
        explicit region(char const* name, std::size_t const thread_num,
            std::size_t const phase, bool const enabled = true) noexcept
          : active(enabled)
        {
            if (active)
            {
                surrounding_region =
                    detail::start_region(name, thread_num, phase);
            }
        }

        ~region() noexcept
        {
            if (active)
            {
                detail::stop_region(surrounding_region);
            }
        }

    private:
        bool active;
        region_data surrounding_region;
    };

    // NOTE: suspend_region (OS-thread zone version) is intentionally NOT
    // used for the fiber path.  Calling stop_region() while inside a Tracy
    // fiber context corrupts the zone stack and causes an abort because the
    // ctx stored in current_region() belongs to the OS-thread zone opened
    // before TracyFiberEnter, but Tracy's internal stack has already switched
    // to the fiber's stack.
    //
    // Use fiber_suspend_region instead - it operates only on the fiber zone.
    HPX_CXX_CORE_EXPORT struct suspend_region
    {
        suspend_region() noexcept
          : suspended_region(detail::stop_region({}))
        {
        }

        ~suspend_region() noexcept
        {
            if (suspended_region.name != nullptr)
            {
                detail::start_region(suspended_region.name,
                    suspended_region.color, suspended_region.phase);
            }
        }

        region_data suspended_region;
    };

    HPX_CXX_CORE_EXPORT struct mark_event
    {
        explicit mark_event(char const* name) noexcept
          : previous_name(detail::rename_region(name))
        {
        }

        ~mark_event()
        {
            detail::rename_region(previous_name);
        }

    private:
        char const* previous_name;
    };

    // RAII guard that closes the running fiber zone before self_.yield() and
    // reopens it after self_.yield() returns. Fully inline so it is visible
    // to all translation units (execution_agent.cpp, thread_helpers.cpp, etc.)
    // without requiring a separate shared-library export.
    // Only touches current_fiber_zone() - never calls stop_region() /
    // start_region() - so there is no zone-stack conflict.
    struct fiber_suspend_region
    {
        explicit fiber_suspend_region(
            char const* suspend_reason = nullptr) noexcept
        {
            detail::suspend_fiber_zone(suspend_reason);
        }

        ~fiber_suspend_region() noexcept
        {
            detail::resume_fiber_zone();
        }
    };
}    // namespace hpx::tracy

#endif
