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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
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
      : impl(detail::is_profiler_connected() ?
                create_tracy_region(data, num_thread) :
                hpx::tracy::region(nullptr, 0, 0, false))
    {
    }

    region::~region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // mark_event

    mark_event::mark_event(char const* name) noexcept
      : impl(name, detail::is_profiler_connected())
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
      : impl(detail::is_profiler_connected() ?
                create_tracy_fiber_region(data, num_thread) :
                hpx::tracy::fiber_region(nullptr, nullptr, 0, false))
    {
    }

    fiber_region::~fiber_region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // fiber_suspend_region -- inner impl runs suspend/resume unconditionally,
    // so we only construct it when active. When not active, we pass nullptr
    // (see the inline ctor in tracy_tls.hpp, which gates on active).

    fiber_suspend_region::fiber_suspend_region(char const* desc) noexcept
      : fiber_suspend_region(desc, detail::is_profiler_connected())
    {
    }

    fiber_suspend_region::fiber_suspend_region(
        char const* desc, bool const active) noexcept
      : impl(active ? desc : nullptr, active)
    {
    }

    fiber_suspend_region::~fiber_suspend_region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // background_work_region -- inner impl always emits start/stop_region, so
    // we forward the connected state explicitly.

    background_work_region::background_work_region(
        std::size_t const num_thread) noexcept
      : impl(num_thread, detail::is_profiler_connected())
    {
    }

    background_work_region::~background_work_region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // lock_context -- see rationale in header.

    lock_context::lock_context(
        char const* name, void const* /* addr */) noexcept
      : active_(detail::is_profiler_connected())
      , impl(hpx::tracy::create(name))
    {
    }

    lock_context::lock_context(
        char const* prefix, char const* suffix, void const* /* addr */) noexcept
      : active_(detail::is_profiler_connected())
      , impl(hpx::tracy::create(std::string(prefix) + suffix))
    {
    }

    lock_context::~lock_context()
    {
        hpx::tracy::destroy(impl);
    }

    bool lock_context::before_lock() const noexcept
    {
        if (!active_)
            return false;
        return hpx::tracy::lock_prepare(impl);
    }

    void lock_context::after_lock() const noexcept
    {
        if (!active_)
            return;
        hpx::tracy::lock_acquired(impl);
    }

    void lock_context::after_try_lock(bool acquired) const noexcept
    {
        if (!active_)
            return;
        hpx::tracy::lock_acquired(impl, acquired);
    }

    void lock_context::before_unlock() const noexcept {}

    void lock_context::after_unlock() const noexcept
    {
        if (!active_)
            return;
        hpx::tracy::lock_released(impl);
    }

    ////////////////////////////////////////////////////////////////////////////
    // set_thread_name

    void set_thread_name(char const* name) noexcept
    {
        hpx::tracy::set_thread_name(name);
    }

    ////////////////////////////////////////////////////////////////////////////
    // task lifecycle tracking

    namespace detail {
        ////////////////////////////////////////////////////////////////////////
        // rename_region body (inline gate wrapper lives in the header)

        char const* rename_region_impl(char const* name) noexcept
        {
            return hpx::tracy::detail::rename_region(name);
        }

        ////////////////////////////////////////////////////////////////////////
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

    namespace detail {

        void task_staged_impl(
            char const* description, void const* parent_task_id) noexcept
        {
            char buffer[256];
            if (parent_task_id)
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Task Staged: %s (Parent: %p)", safe_str(description),
                    const_cast<void*>(parent_task_id));
            }
            else
            {
                std::snprintf(buffer, sizeof(buffer), "Task Staged: %s",
                    safe_str(description));
            }
            hpx::tracy::message(buffer, std::strlen(buffer),
                static_cast<std::uint32_t>(color::staged));
        }

        void task_created_impl(char const* description, void const* task_id,
            void const* parent_task_id) noexcept
        {
            char buffer[256];
            if (parent_task_id)
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Task Created: %p - %s (Parent: %p)",
                    const_cast<void*>(task_id), safe_str(description),
                    const_cast<void*>(parent_task_id));
            }
            else
            {
                std::snprintf(buffer, sizeof(buffer), "Task Created: %p - %s",
                    const_cast<void*>(task_id), safe_str(description));
            }
            hpx::tracy::message(buffer, std::strlen(buffer),
                static_cast<std::uint32_t>(color::created));
        }

        void task_executing_impl(void const* task_id, char const* description,
            std::size_t worker_thread) noexcept
        {
            char buffer[256];
            std::snprintf(buffer, sizeof(buffer),
                "Task Executing (W%zu): %p - %s", worker_thread,
                const_cast<void*>(task_id), safe_str(description));
            hpx::tracy::message(buffer, std::strlen(buffer),
                static_cast<std::uint32_t>(color::executing));
        }

        void task_yielded_impl(
            void const* task_id, char const* description) noexcept
        {
            char buffer[256];
            std::snprintf(buffer, sizeof(buffer), "Task Yielded: %p - %s",
                const_cast<void*>(task_id), safe_str(description));
            hpx::tracy::message(buffer, std::strlen(buffer),
                static_cast<std::uint32_t>(color::yielded));
        }

        void task_suspended_impl(void const* task_id, char const* description,
            char const* reason) noexcept
        {
            char buffer[256];
            if (reason)
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Task Suspended: %p - %s (Reason: %s)",
                    const_cast<void*>(task_id), safe_str(description), reason);
            }
            else
            {
                std::snprintf(buffer, sizeof(buffer), "Task Suspended: %p - %s",
                    const_cast<void*>(task_id), safe_str(description));
            }
            hpx::tracy::message(buffer, std::strlen(buffer),
                static_cast<std::uint32_t>(color::suspended));
        }

        void task_resumed_impl(void const* task_id, char const* description,
            char const* wake_reason) noexcept
        {
            char buffer[256];
            if (wake_reason)
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Task Resumed: %p - %s (Wake: %s)",
                    const_cast<void*>(task_id), safe_str(description),
                    wake_reason);
            }
            else
            {
                std::snprintf(buffer, sizeof(buffer), "Task Resumed: %p - %s",
                    const_cast<void*>(task_id), safe_str(description));
            }
            hpx::tracy::message(buffer, std::strlen(buffer),
                static_cast<std::uint32_t>(color::resumed));
        }

        void task_completed_impl(
            void const* task_id, char const* description) noexcept
        {
            char buffer[256];
            std::snprintf(buffer, sizeof(buffer), "Task Completed: %p - %s",
                const_cast<void*>(task_id), safe_str(description));
            hpx::tracy::message(buffer, std::strlen(buffer),
                static_cast<std::uint32_t>(color::completed));
        }

        void task_deleted_impl(void const* task_id) noexcept
        {
            char buffer[256];
            std::snprintf(buffer, sizeof(buffer), "Task Deleted: %p",
                const_cast<void*>(task_id));
            hpx::tracy::message(buffer, std::strlen(buffer),
                static_cast<std::uint32_t>(color::deleted));
        }

        ////////////////////////////////////////////////////////////////////////////
        // Causal tracing: future fulfillment signals

        void future_fulfilled_impl(
            void const* future_id, char const* desc) noexcept
        {
            char buffer[256];
            if (desc)
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Future Fulfilled: %p (%s)", const_cast<void*>(future_id),
                    desc);
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

        void future_exception_set_impl(
            void const* future_id, char const* desc) noexcept
        {
            char buffer[256];
            if (desc)
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Future Exception Set: %p (%s)",
                    const_cast<void*>(future_id), desc);
            }
            else
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Future Exception Set: %p", const_cast<void*>(future_id));
            }
            std::size_t const len = std::strlen(buffer);
            // Red: producer error signal
            hpx::tracy::message(buffer, len, 0xFF0000u);
            // Embed directly into active fiber visual zone text
            hpx::tracy::detail::add_zone_text_to_fiber(buffer, len);
        }

        // Definition of the atomic counter declared extern in tracy.hpp.
        // Updated by the inline continuation_{run,finished} wrappers whether
        // Tracy is connected or not.
        HPX_CORE_EXPORT std::atomic<std::int64_t> g_active_continuations{0};

        void continuation_run_emit(void const* task_id) noexcept
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

            // Sample the counter at emit time so Tracy's plot records the
            // value as of this timestamp, not the snapshot taken at the
            // fetch_add call site. Two concurrent emits can still see
            // slightly different reads, but neither is stale.
            hpx::tracy::sample_value("Active Continuations",
                static_cast<double>(
                    g_active_continuations.load(std::memory_order_relaxed)));
        }

        void continuation_finished_emit() noexcept
        {
            hpx::tracy::sample_value("Active Continuations",
                static_cast<double>(
                    g_active_continuations.load(std::memory_order_relaxed)));
        }

        void handle_on_completed_fired_impl(void const* task_id) noexcept
        {
            char buffer[256];
            if (task_id)
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Handle On Completed Fired: %p",
                    const_cast<void*>(task_id));
            }
            else
            {
                std::snprintf(
                    buffer, sizeof(buffer), "Handle On Completed Fired");
            }
            std::size_t const len = std::strlen(buffer);
            // Yellow: dispatch signal
            hpx::tracy::message(buffer, len, 0xFFFF00u);
            // Embed directly into active fiber visual zone text
            hpx::tracy::detail::add_zone_text_to_fiber(buffer, len);
        }

        void work_stolen_impl(std::size_t thief_id, std::size_t victim_id,
            void const* task_id, char const* desc) noexcept
        {
            char buffer[256];
            if (desc && desc[0] != '\0')
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Work Stolen: Thief W#%zu <- Victim W#%zu | Task: %p (%s)",
                    thief_id, victim_id, task_id, desc);
            }
            else
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Work Stolen: Thief W#%zu <- Victim W#%zu | Task: %p",
                    thief_id, victim_id, task_id);
            }
            // Amber/Gold color: 0xFFC107
            hpx::tracy::message(buffer, std::strlen(buffer), 0xFFC107u);
        }

        void frame_mark_impl(char const* name) noexcept
        {
            hpx::tracy::frame_mark(name);
        }

        void os_thread_sleep_impl(std::size_t num_thread) noexcept
        {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer),
                "OS Worker #%zu entering sleep on CV", num_thread);
            // Blue-grey: cold/inactive state
            hpx::tracy::message(buffer, std::strlen(buffer), 0x546E7Au);
        }

        // Mirror of naming::invalid_locality_id (~std::uint32_t(0)) from
        // libs/full/naming_base/include/hpx/naming_base/naming_base.hpp.
        // Duplicated because core cannot depend on full; if the real
        // constant ever changes, update this or the invalid-locality
        // branches will silently start printing L#4294967295 again.
        static constexpr std::uint64_t invalid_locality = 0xFFFFFFFFULL;

        void send_parcel_impl(std::uint64_t tag_msb, std::uint64_t tag_lsb,
            std::uint64_t size, std::uint64_t target_locality_id) noexcept
        {
            char buffer[160];
            // "Parcel Send: id=<msb>:<lsb> size=<N> to=L#<N>" + NUL
            static_assert(
                sizeof(buffer) >= 16 + 16 + 1 + 16 + 6 + 20 + 6 + 20 + 1);
            if (target_locality_id == invalid_locality)
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Parcel Send: id=%016llx:%016llx size=%llu to=L#unknown",
                    static_cast<unsigned long long>(tag_msb),
                    static_cast<unsigned long long>(tag_lsb),
                    static_cast<unsigned long long>(size));
            }
            else
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Parcel Send: id=%016llx:%016llx size=%llu to=L#%llu",
                    static_cast<unsigned long long>(tag_msb),
                    static_cast<unsigned long long>(tag_lsb),
                    static_cast<unsigned long long>(size),
                    static_cast<unsigned long long>(target_locality_id));
            }
            hpx::tracy::message(buffer, std::strlen(buffer), 0x2196F3u);
        }

        // Size is omitted here: parcel::size_ is not populated on the
        // receive side, so any value would be a fixed zero.
        void recv_parcel_impl(std::uint64_t tag_msb, std::uint64_t tag_lsb,
            std::uint64_t source_locality_id) noexcept
        {
            char buffer[160];
            // "Parcel Recv: id=<msb>:<lsb> from=L#<N>" + NUL
            static_assert(sizeof(buffer) >= 16 + 16 + 1 + 16 + 8 + 20 + 1);
            if (source_locality_id == invalid_locality)
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Parcel Recv: id=%016llx:%016llx from=L#unknown",
                    static_cast<unsigned long long>(tag_msb),
                    static_cast<unsigned long long>(tag_lsb));
            }
            else
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Parcel Recv: id=%016llx:%016llx from=L#%llu",
                    static_cast<unsigned long long>(tag_msb),
                    static_cast<unsigned long long>(tag_lsb),
                    static_cast<unsigned long long>(source_locality_id));
            }
            hpx::tracy::message(buffer, std::strlen(buffer), 0x4CAF50u);
        }

        void parcel_scheduled_impl(std::uint64_t tag_msb, std::uint64_t tag_lsb,
            std::uint64_t source_locality_id,
            std::uint64_t source_thread_id) noexcept
        {
            char buffer[160];
            // "Parcel Scheduled: id=<msb>:<lsb> from=L#<N> thread=0x<hex>" + NUL
            static_assert(
                sizeof(buffer) >= 21 + 16 + 1 + 16 + 8 + 20 + 10 + 16 + 1);
            if (source_locality_id == invalid_locality)
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Parcel Scheduled: id=%016llx:%016llx from=L#unknown "
                    "thread=0x%016llx",
                    static_cast<unsigned long long>(tag_msb),
                    static_cast<unsigned long long>(tag_lsb),
                    static_cast<unsigned long long>(source_thread_id));
            }
            else
            {
                std::snprintf(buffer, sizeof(buffer),
                    "Parcel Scheduled: id=%016llx:%016llx from=L#%llu "
                    "thread=0x%016llx",
                    static_cast<unsigned long long>(tag_msb),
                    static_cast<unsigned long long>(tag_lsb),
                    static_cast<unsigned long long>(source_locality_id),
                    static_cast<unsigned long long>(source_thread_id));
            }
            hpx::tracy::message(buffer, std::strlen(buffer), 0x9C27B0u);
        }

    }    // namespace detail

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
