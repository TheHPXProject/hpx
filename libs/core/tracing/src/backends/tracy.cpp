//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_TRACY)

#include <hpx/modules/tracy.hpp>
#include <hpx/tracing/tracing.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

namespace hpx::tracing {

    ////////////////////////////////////////////////////////////////////////////
    // region

    hpx::tracy::region region::create_tracy_region(
        region_init_data const& data, std::size_t const num_thread) noexcept
    {
        bool const enabled = data.name != nullptr && !data.is_stackless;
        return hpx::tracy::region(
            data.name, num_thread, data.thread_phase, enabled);
    }

    region::region(loop_context&, region_init_data const& data,
        std::size_t const num_thread) noexcept
      : impl(create_tracy_region(data, num_thread))
    {
    }

    region::~region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // mark_event

    mark_event::mark_event(char const* name) noexcept
      : impl(name)
    {
    }

    mark_event::~mark_event() = default;

    ////////////////////////////////////////////////////////////////////////////
    // fiber_region

    hpx::tracy::fiber_region fiber_region::create_tracy_fiber_region(
        fiber_region_init_data const& data,
        std::size_t const num_thread) noexcept
    {
        bool const enabled = data.name != nullptr && !data.is_stackless;
        char const* fiber_name = enabled ? data.fiber_name : nullptr;

        // Use num_thread as color seed so each worker thread gets a distinct
        // color on the fiber track in Tracy.
        auto const color =
            static_cast<std::size_t>(num_thread + 1) * 0x9e3779b9;
        return hpx::tracy::fiber_region(fiber_name, data.name, color, enabled);
    }

    fiber_region::fiber_region(fiber_region_init_data const& data,
        std::size_t const num_thread) noexcept
      : impl(create_tracy_fiber_region(data, num_thread))
    {
    }

    fiber_region::~fiber_region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // fiber_suspend_region

    fiber_suspend_region::fiber_suspend_region(char const* desc) noexcept
      : impl(desc)
    {
    }

    fiber_suspend_region::~fiber_suspend_region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // lock_context

    lock_context::lock_context(
        char const* name, void const* /* addr */) noexcept
      : impl(hpx::tracy::create(name))
    {
    }

    lock_context::lock_context(
        char const* prefix, char const* suffix, void const* /* addr */) noexcept
      : impl(hpx::tracy::create(std::string(prefix) + suffix))
    {
    }

    lock_context::~lock_context()
    {
        hpx::tracy::destroy(impl);
    }

    bool lock_context::before_lock() const noexcept
    {
        return hpx::tracy::lock_prepare(impl);
    }

    void lock_context::after_lock() const noexcept
    {
        hpx::tracy::lock_acquired(impl);
    }

    void lock_context::after_try_lock(bool acquired) const noexcept
    {
        hpx::tracy::lock_acquired(impl, acquired);
    }

    void lock_context::before_unlock() const noexcept {}

    void lock_context::after_unlock() const noexcept
    {
        hpx::tracy::lock_released(impl);
    }

    ////////////////////////////////////////////////////////////////////////////
    // set_thread_name

    void set_thread_name(char const* name) noexcept
    {
        hpx::tracy::set_thread_name(name);
    }

    ////////////////////////////////////////////////////////////////////////////
    // rename_region

    char const* rename_region(char const* name) noexcept
    {
        return hpx::tracy::detail::rename_region(name);
    }

    ////////////////////////////////////////////////////////////////////////////
    // task lifecycle tracking

    namespace detail {
        enum class color : std::uint32_t
        {
            staged = 0x808080,
            created = 0x00FF00,
            executing = 0xFF00FF,
            yielded = 0xFFA500,
            suspended = 0xFF0000,
            resumed = 0x00FFFF,
            completed = 0x008000,
            deleted = 0x800080
        };

        constexpr char const* safe_str(char const* str) noexcept
        {
            return str ? str : "<unknown>";
        }
    }    // namespace detail

    void task_staged(
        char const* description, void const* parent_task_id) noexcept
    {
        char buffer[256];
        if (parent_task_id)
        {
            std::snprintf(buffer, sizeof(buffer),
                "Task Staged: %s (Parent: %p)", detail::safe_str(description),
                const_cast<void*>(parent_task_id));
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer), "Task Staged: %s",
                detail::safe_str(description));
        }
        hpx::tracy::message(buffer, std::strlen(buffer),
            static_cast<std::uint32_t>(detail::color::staged));
    }

    void task_created(char const* description, void const* task_id,
        void const* parent_task_id) noexcept
    {
        char buffer[256];
        if (parent_task_id)
        {
            std::snprintf(buffer, sizeof(buffer),
                "Task Created: %p - %s (Parent: %p)",
                const_cast<void*>(task_id), detail::safe_str(description),
                const_cast<void*>(parent_task_id));
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer), "Task Created: %p - %s",
                const_cast<void*>(task_id), detail::safe_str(description));
        }
        hpx::tracy::message(buffer, std::strlen(buffer),
            static_cast<std::uint32_t>(detail::color::created));
    }

    void task_executing(void const* task_id, char const* description,
        std::size_t worker_thread) noexcept
    {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "Task Executing (W%zu): %p - %s",
            worker_thread, const_cast<void*>(task_id),
            detail::safe_str(description));
        hpx::tracy::message(buffer, std::strlen(buffer),
            static_cast<std::uint32_t>(detail::color::executing));
    }

    void task_yielded(void const* task_id, char const* description) noexcept
    {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "Task Yielded: %p - %s",
            const_cast<void*>(task_id), detail::safe_str(description));
        hpx::tracy::message(buffer, std::strlen(buffer),
            static_cast<std::uint32_t>(detail::color::yielded));
    }

    void task_suspended(void const* task_id, char const* description,
        char const* reason) noexcept
    {
        char buffer[256];
        if (reason)
        {
            std::snprintf(buffer, sizeof(buffer),
                "Task Suspended: %p - %s (Reason: %s)",
                const_cast<void*>(task_id), detail::safe_str(description),
                reason);
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer), "Task Suspended: %p - %s",
                const_cast<void*>(task_id), detail::safe_str(description));
        }
        hpx::tracy::message(buffer, std::strlen(buffer),
            static_cast<std::uint32_t>(detail::color::suspended));
    }

    void task_resumed(void const* task_id, char const* description,
        char const* wake_reason) noexcept
    {
        char buffer[256];
        if (wake_reason)
        {
            std::snprintf(buffer, sizeof(buffer),
                "Task Resumed: %p - %s (Wake: %s)", const_cast<void*>(task_id),
                detail::safe_str(description), wake_reason);
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer), "Task Resumed: %p - %s",
                const_cast<void*>(task_id), detail::safe_str(description));
        }
        hpx::tracy::message(buffer, std::strlen(buffer),
            static_cast<std::uint32_t>(detail::color::resumed));
    }

    void task_completed(void const* task_id, char const* description) noexcept
    {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "Task Completed: %p - %s",
            const_cast<void*>(task_id), detail::safe_str(description));
        hpx::tracy::message(buffer, std::strlen(buffer),
            static_cast<std::uint32_t>(detail::color::completed));
    }

    void task_deleted(void const* task_id) noexcept
    {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "Task Deleted: %p",
            const_cast<void*>(task_id));
        hpx::tracy::message(buffer, std::strlen(buffer),
            static_cast<std::uint32_t>(detail::color::deleted));
    }

    ////////////////////////////////////////////////////////////////////////////
    // Causal tracing: future fulfillment signals

    void future_fulfilled(void const* future_id, char const* desc) noexcept
    {
        char buffer[256];
        if (desc)
        {
            std::snprintf(buffer, sizeof(buffer), "Future Fulfilled: %p (%s)",
                const_cast<void*>(future_id), desc);
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer), "Future Fulfilled: %p",
                const_cast<void*>(future_id));
        }
        std::size_t const len = std::strlen(buffer);
        // Green: producer signal
        hpx::tracy::message(buffer, len, 0x00FF00u);
        // Embed directly into active fiber visual zone text
        hpx::tracy::detail::add_zone_text_to_fiber(buffer, len);
    }

    void future_exception_set(void const* future_id, char const* desc) noexcept
    {
        char buffer[256];
        if (desc)
        {
            std::snprintf(buffer, sizeof(buffer),
                "Future Exception Set: %p (%s)", const_cast<void*>(future_id),
                desc);
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer), "Future Exception Set: %p",
                const_cast<void*>(future_id));
        }
        std::size_t const len = std::strlen(buffer);
        // Red: producer error signal
        hpx::tracy::message(buffer, len, 0xFF0000u);
        // Embed directly into active fiber visual zone text
        hpx::tracy::detail::add_zone_text_to_fiber(buffer, len);
    }

    namespace {
        std::int64_t g_active_continuations{0};
        std::mutex g_active_continuations_mtx;
    }    // namespace

    void continuation_run(void const* task_id) noexcept
    {
        char buffer[256];
        if (task_id)
        {
            std::snprintf(buffer, sizeof(buffer), "Continuation Run: %p",
                const_cast<void*>(task_id));
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer), "Continuation Run");
        }
        std::size_t const len = std::strlen(buffer);
        // White: consumer signal
        hpx::tracy::message(buffer, len, 0xFFFFFFu);
        // Embed directly into active fiber visual zone text
        hpx::tracy::detail::add_zone_text_to_fiber(buffer, len);

        // Update live time-series plot graph for Active Continuations in Tracy
        std::int64_t current_active = 0;
        {
            std::lock_guard<std::mutex> lock(g_active_continuations_mtx);
            current_active = ++g_active_continuations;
            hpx::tracy::sample_value(
                "Active Continuations", static_cast<double>(current_active));
        }
    }

    void continuation_finished(void const* /* task_id */) noexcept
    {
        std::int64_t current_active = 0;
        {
            std::lock_guard<std::mutex> lock(g_active_continuations_mtx);
            current_active = --g_active_continuations;
            hpx::tracy::sample_value(
                "Active Continuations", static_cast<double>(current_active));
        }
    }

    void handle_on_completed_fired(void const* task_id) noexcept
    {
        char buffer[256];
        if (task_id)
        {
            std::snprintf(buffer, sizeof(buffer),
                "Handle On Completed Fired: %p", const_cast<void*>(task_id));
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer), "Handle On Completed Fired");
        }
        std::size_t const len = std::strlen(buffer);
        // Yellow: dispatch signal
        hpx::tracy::message(buffer, len, 0xFFFF00u);
        // Embed directly into active fiber visual zone text
        hpx::tracy::detail::add_zone_text_to_fiber(buffer, len);
    }

    void frame_mark(char const* name) noexcept
    {
        hpx::tracy::frame_mark(name);
    }

    ////////////////////////////////////////////////////////////////////////////
    // counters

    void create_counter(
        std::string const&, std::string const& short_name) noexcept
    {
        hpx::tracy::create_counter(short_name);
    }

    void sample_counter(std::string const&, std::string const& short_name,
        double value) noexcept
    {
        hpx::tracy::sample_value(short_name, value);
    }

}    // namespace hpx::tracing

#endif
