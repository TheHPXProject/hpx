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

    // Mirror of TracyCZoneCtx under TRACY_ON_DEMAND (which HPX forces on).
    // Keeping our own struct lets tracy_tls.hpp stay free of Tracy's headers.
    // tracy_tls.cpp asserts the Tracy version and that the layouts still match.
    struct zone_ctx_storage
    {
        std::uint32_t id = 0;
        std::int32_t active = 0;
        std::uint64_t connectionId = 0;
    };

    HPX_CXX_CORE_EXPORT struct region_data
    {
        char const* name = nullptr;
        zone_ctx_storage data{};
        std::uint32_t color = 0;
        std::uint32_t phase = 0;
    };

    namespace detail {

        HPX_CORE_EXPORT region_data start_region(
            char const*, std::size_t = 0, std::size_t = 0) noexcept;
        HPX_CORE_EXPORT region_data start_named_region(
            char const* name, std::uint32_t color) noexcept;
        HPX_CORE_EXPORT void add_worker_thread_text(
            region_data const& r, std::size_t num_thread) noexcept;
        HPX_CORE_EXPORT char const* rename_region(char const*) noexcept;
        HPX_CORE_EXPORT zone_ctx_storage push_zone(char const*) noexcept;
        HPX_CORE_EXPORT void pop_zone(zone_ctx_storage const&) noexcept;
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

        /// \brief Embed arbitrary text into the currently-active fiber zone.
        ///
        /// The text appears in the Zone Info popup when the user clicks the
        /// colored bar on the Tracy timeline. This is how producer/consumer
        /// addresses are attached directly to the visual zone rectangle.
        ///
        /// \param txt  Pointer to character string.
        /// \param size Length of the string.
        HPX_CORE_EXPORT void add_zone_text_to_fiber(
            char const* txt, std::size_t size) noexcept;

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

    HPX_CXX_CORE_EXPORT struct background_work_region
    {
        /// \brief Opens a background-work zone on the given worker thread.
        ///
        /// \param active  If false, the ctor and dtor emit no Tracy events.
        // Two-arg ctor: caller supplies an explicit `active` flag captured
        // at the outer wrapper's construction (see backends/tracy.hpp
        // background_work_region). Disconnected -> we skip start_named_region
        // AND the paired stop_region in the dtor, preserving the invariant
        // that an instance whose ctor didn't emit must not emit on destroy.
        explicit background_work_region(
            std::size_t num_thread, bool active) noexcept
          : active_(active)
        {
            if (active_)
            {
                surrounding_region = detail::start_named_region(
                    "hpx::background_work", 0x008080u);
                detail::add_worker_thread_text(surrounding_region, num_thread);
            }
        }

        // Legacy single-arg ctor kept for source-compat with code that
        // instantiates this type directly (nothing in-tree does today).
        // Defaults to active -- the classic pre-gate behaviour.
        explicit background_work_region(std::size_t num_thread) noexcept
          : background_work_region(num_thread, true)
        {
        }

        ~background_work_region() noexcept
        {
            if (active_)
            {
                detail::stop_region(surrounding_region);
            }
        }

        background_work_region(background_work_region const&) = delete;
        background_work_region& operator=(
            background_work_region const&) = delete;

    private:
        bool active_;
        region_data surrounding_region;
    };

    HPX_CXX_CORE_EXPORT struct mark_event
    {
        /// \brief Opens a Tracy zone for the given event name.
        ///
        /// \param active  If false, the ctor and dtor emit no Tracy events.
        // Two-arg ctor: `active` records the profiler-connected decision
        // taken by the outer wrapper. If not active, we skip push_zone and
        // the paired pop_zone in the dtor.
        explicit mark_event(char const* name, bool active) noexcept
          : active_(active)
          , ctx_value(active_ ? detail::push_zone(name) : zone_ctx_storage{})
        {
        }

        // Legacy single-arg ctor for direct instantiation; defaults active.
        explicit mark_event(char const* name) noexcept
          : mark_event(name, true)
        {
        }

        mark_event(mark_event const&) = delete;
        mark_event(mark_event&&) = delete;
        mark_event& operator=(mark_event const&) = delete;
        mark_event& operator=(mark_event&&) = delete;

        ~mark_event()
        {
            if (active_)
            {
                detail::pop_zone(ctx_value);
            }
        }

    private:
        bool active_;
        zone_ctx_storage ctx_value;
    };

    // RAII guard that closes the running fiber zone before self_.yield() and
    // reopens it after self_.yield() returns. Fully inline so it is visible
    // to all translation units (execution_agent.cpp, thread_helpers.cpp, etc.)
    // without requiring a separate shared-library export.
    // Only touches current_fiber_zone() - never calls stop_region() /
    // start_region() - so there is no zone-stack conflict.
    struct fiber_suspend_region
    {
        /// \brief Suspends the currently active fiber zone with the given
        ///        reason.
        ///
        /// \param active  If false, the ctor and dtor emit no Tracy events.
        // Two-arg ctor honours the profiler-connected decision captured by
        // the outer wrapper. Skipping suspend_fiber_zone means the paired
        // resume_fiber_zone in the dtor is also skipped -- critical for
        // zone-stack balance.
        explicit fiber_suspend_region(
            char const* suspend_reason, bool active) noexcept
          : active_(active)
        {
            if (active_)
            {
                detail::suspend_fiber_zone(suspend_reason);
            }
        }

        // Legacy single-arg ctor for direct instantiation; defaults active.
        explicit fiber_suspend_region(
            char const* suspend_reason = nullptr) noexcept
          : fiber_suspend_region(suspend_reason, true)
        {
        }

        ~fiber_suspend_region() noexcept
        {
            if (active_)
            {
                detail::resume_fiber_zone();
            }
        }

    private:
        bool active_;
    };
}    // namespace hpx::tracy

#endif
