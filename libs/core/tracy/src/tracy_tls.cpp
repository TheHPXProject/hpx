//  Copyright (c) 2025-2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_TRACY)
#include <hpx/tracy/tracy_tls.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <common/TracyVersion.hpp>
#include <tracy/TracyC.h>

// zone_ctx_storage mirrors ___tracy_c_zone_context under TRACY_ON_DEMAND. The
// version check pins the range the layout was written for; the size and
// alignment asserts catch a change within a patch release.
static_assert(tracy::Version::Major == 0 && tracy::Version::Minor == 14,
    "zone_ctx_storage layout was written for Tracy 0.14.x; re-check "
    "___tracy_c_zone_context before widening the range.");
static_assert(sizeof(TracyCZoneCtx) == sizeof(hpx::tracy::zone_ctx_storage));
static_assert(alignof(TracyCZoneCtx) == alignof(hpx::tracy::zone_ctx_storage));

///////////////////////////////////////////////////////////////////////////////
namespace hpx::tracy {

    namespace {

        // Conversions to and from Tracy's C ctx type. Designated initialisers
        // turn a future field reorder or rename into a compile error.
        constexpr zone_ctx_storage to_storage(TracyCZoneCtx const& c) noexcept
        {
            return {
                .id = c.id, .active = c.active, .connectionId = c.connectionId};
        }

        constexpr TracyCZoneCtx to_tracy(zone_ctx_storage const& s) noexcept
        {
            return {
                .id = s.id, .active = s.active, .connectionId = s.connectionId};
        }

        char const* intern_zone_label(
            char const* label, char const* fallback) noexcept
        {
            if (label == nullptr || label[0] == '\0')
            {
                label = fallback;
            }
            return label;
        }

        // Store currently active Tracy zone
        region_data& current_region() noexcept
        {
            thread_local region_data region;
            return region;
        }

        // True while this OS thread is inside a Tracy fiber. rename_region is
        // then a no-op, as its zone is on the OS thread's zone stack, not the
        // fiber's, and push_zone counts the zones it opens on the fiber.
        bool& in_fiber() noexcept
        {
            thread_local bool flag = false;
            return flag;
        }

        // "Unset" sentinel for a ctx stashed in TLS. A live ctx opened while
        // disconnected is also all-zero, but TracyCZoneEnd is a no-op when
        // active is 0, so treating that case as "unset" is safe.
        constexpr bool ctx_is_empty(zone_ctx_storage const& s) noexcept
        {
            return s.id == 0 && s.active == 0 && s.connectionId == 0;
        }

        // Under TRACY_ON_DEMAND Tracy drops the end and the text of a zone
        // opened while an earlier profiler was connected.
        constexpr bool same_connection(
            zone_ctx_storage const& a, zone_ctx_storage const& b) noexcept
        {
            return a.active != 0 && b.active != 0 &&
                a.connectionId == b.connectionId;
        }

        // The innermost zone HPX keeps on this worker's fiber (the task's zone,
        // or the one enter_fiber opens for a resumed task) and how many
        // push_zone zones are open on top of it. Per worker, so a suspended
        // task carries both in its suspended_fiber_zone instead.
        struct fiber_zone_data
        {
            zone_ctx_storage ctx_value{};
            std::uint32_t zones_above = 0;
            bool active = false;
        };

        fiber_zone_data& current_fiber_zone() noexcept
        {
            thread_local fiber_zone_data fz;
            return fz;
        }

        zone_ctx_storage open_zone(
            char const* zone_name, std::uint32_t color) noexcept
        {
            TracyCZoneC(ctx, color, 1);
            TracyCZoneName(ctx, zone_name, std::strlen(zone_name));
            return to_storage(ctx);
        }

        void open_fiber_zone(fiber_zone_data& fz, char const* zone_name,
            std::uint32_t color) noexcept
        {
            fz.ctx_value = open_zone(zone_name, color);
            fz.zones_above = 0;
            fz.active = true;
        }
    }    // namespace

    namespace detail {

        HPX_CORE_EXPORT region_data start_region(char const* new_region,
            std::size_t const thread_num, std::size_t const phase) noexcept
        {
            // No callstack: scheduler-dispatch frames are uniform across
            // tasks and Tracy would resolve them via DbgHelp on every
            // dispatch. Task identity is on Name and Value below.
            TracyCZoneC(ctx, static_cast<std::uint32_t>(thread_num), 1);
            TracyCZoneName(ctx, new_region, std::strlen(new_region));
            TracyCZoneValue(ctx, static_cast<std::uint32_t>(phase));

            region_data& region = current_region();
            region_data const prev_region = region;

            region.name = new_region;
            region.data = to_storage(ctx);
            region.color = static_cast<std::uint32_t>(thread_num);
            region.phase = static_cast<std::uint32_t>(phase);

            return prev_region;
        }

        HPX_CORE_EXPORT region_data start_named_region(
            char const* name, std::uint32_t const color) noexcept
        {
            TracyCZoneC(ctx, color, 1);
            TracyCZoneName(ctx, name, std::strlen(name));

            region_data& region = current_region();
            region_data const prev_region = region;

            region.name = name;
            region.data = to_storage(ctx);
            region.color = color;
            region.phase = 0;

            return prev_region;
        }

        HPX_CORE_EXPORT region_data stop_region(
            region_data const& prev_region) noexcept
        {
            region_data const curr_region = current_region();

            current_region() = prev_region;
            if (curr_region.name != nullptr)
            {
                TracyCZoneEnd(to_tracy(curr_region.data));
            }

            return curr_region;
        }

        HPX_CORE_EXPORT void set_in_fiber(bool value) noexcept
        {
            in_fiber() = value;
        }

        HPX_CORE_EXPORT void start_fiber_zone(
            char const* zone_name, std::uint32_t color) noexcept
        {
            char const* safe_zone_name = intern_zone_label(zone_name, "fiber");

            open_fiber_zone(current_fiber_zone(), safe_zone_name, color);
        }

        HPX_CORE_EXPORT void stop_fiber_zone() noexcept
        {
            auto& fz = current_fiber_zone();
            if (fz.active)
            {
                TracyCZoneEnd(to_tracy(fz.ctx_value));
                fz.active = false;
                fz.ctx_value = {};
                fz.zones_above = 0;
            }
        }

        // Open a zone for the time the task is suspended, on top of the task's
        // zone and of any zone the task opened (HPX_TRACING_MARK_EVENT), as
        // those can stay open across the suspension. Both zones go with the
        // task, so leave_fiber leaves them open.
        HPX_CORE_EXPORT suspended_fiber_zone suspend_fiber_zone(
            char const* suspend_reason) noexcept
        {
            // Not active if the task has no fiber, or if an enclosing region
            // opened a suspension zone and the task has not been switched out
            // since; that zone already covers this one.
            auto& fz = current_fiber_zone();
            if (!fz.active)
                return {};

            constexpr std::uint32_t suspended_color = 0xAAAAAA;
            suspended_fiber_zone const suspended{
                open_zone(intern_zone_label(suspend_reason, "suspend"),
                    suspended_color),
                fz.ctx_value, fz.zones_above, true};
            fz.active = false;
            fz.ctx_value = {};
            fz.zones_above = 0;
            return suspended;
        }

        // If the task was switched out, enter_fiber has opened a zone on top
        // of the suspension zone; Tracy ends a fiber's zones in reverse order,
        // so close that one first. If not (the region ended without a yield),
        // there is none. Either way this worker then holds the task's zone.
        HPX_CORE_EXPORT void resume_fiber_zone(
            suspended_fiber_zone const& suspended) noexcept
        {
            auto& fz = current_fiber_zone();
            if (!suspended.open || !in_fiber())
                return;

            if (fz.active)
            {
                // After a reconnect while the task was switched out, Tracy
                // drops the events of the saved zones and of those the task
                // opened before, so keep the zone enter_fiber opened, with no
                // zones counted above it.
                if (!same_connection(fz.ctx_value, suspended.suspension))
                    return;
                TracyCZoneEnd(to_tracy(fz.ctx_value));
            }
            TracyCZoneEnd(to_tracy(suspended.suspension));
            fz.ctx_value = suspended.task;
            fz.zones_above = suspended.zones_above;
            fz.active = true;
        }

        // The text shows in Tracy's zone information window. Skipped while a
        // push_zone zone is on top, as Tracy accepts zone text only for the
        // innermost zone and stops a verifying capture otherwise; the same
        // text is also sent as a message.
        HPX_CORE_EXPORT void add_zone_text_to_fiber(
            char const* txt, std::size_t size) noexcept
        {
            auto& fz = current_fiber_zone();
            if (txt == nullptr || size == 0 || !fz.active ||
                fz.zones_above != 0 || ctx_is_empty(fz.ctx_value))
                return;

            // TracyCZoneText sends the text via the zone validation protocol.
            TracyCZoneText(to_tracy(fz.ctx_value), txt, size);
        }

        namespace {

            // Pre-formatted "Worker #N" strings for common thread indices.
            // Eliminates snprintf overhead in the hot background-polling path.
            // Capacity covers up to 256 threads for high-core-count HPC systems.
            // Thread counts exceeding capacity safely fall back to per-call snprintf.
            struct worker_label_cache
            {
                static constexpr std::size_t capacity = 256;

                worker_label_cache() noexcept
                {
                    for (std::size_t i = 0; i < capacity; ++i)
                    {
                        int const n = std::snprintf(
                            storage[i], sizeof(storage[i]), "Worker #%zu", i);
                        lengths[i] = n > 0 ? static_cast<std::size_t>(n) :
                                             std::size_t{0};
                    }
                }

                char storage[capacity][24];
                std::size_t lengths[capacity];
            };

            worker_label_cache const& get_worker_labels() noexcept
            {
                static worker_label_cache const cache;
                return cache;
            }

        }    // namespace

        HPX_CORE_EXPORT void add_worker_thread_text(
            region_data const& r, std::size_t const num_thread) noexcept
        {
            if (ctx_is_empty(r.data))
                return;

            auto const& cache = get_worker_labels();
            char const* text;
            std::size_t len;

            char tmp[32];
            if (num_thread < worker_label_cache::capacity)
            {
                text = cache.storage[num_thread];
                len = cache.lengths[num_thread];
            }
            else
            {
                // Fallback for thread counts > 256 (e.g. massive multi-socket systems)
                int const n =
                    std::snprintf(tmp, sizeof(tmp), "Worker #%zu", num_thread);
                text = tmp;
                len = n > 0 ? static_cast<std::size_t>(n) : std::size_t{0};
            }

            if (len > 0)
            {
                TracyCZoneText(to_tracy(r.data), text, len);
            }
        }

        HPX_CORE_EXPORT char const* rename_region(
            char const* new_region) noexcept
        {
            // No-op inside a fiber context: the TLS zone ctx belongs to the
            // OS-thread zone, not the fiber's zone stack. Calling
            // TracyCZoneName here would corrupt Tracy's zone stack and cause
            // an abort.
            if (in_fiber())
                return new_region;

            if (auto& [name, value, _1_, _2_] = current_region();
                name != nullptr)
            {
                char const* previous_name = name;
                name = new_region;

                TracyCZoneName(
                    to_tracy(value), new_region, std::strlen(new_region));
                return previous_name;
            }
            return nullptr;
        }

        HPX_CORE_EXPORT zone_ctx_storage push_zone(char const* name) noexcept
        {
            char const* safe_name = intern_zone_label(name, "event");
            TracyCZoneC(ctx, 0x0078D7, 1);
            TracyCZoneName(ctx, safe_name, std::strlen(safe_name));

            // Inside a fiber the zone goes on top of the task's zone, so
            // add_zone_text_to_fiber must not add text until it is closed.
            if (ctx.active && in_fiber())
            {
                ++current_fiber_zone().zones_above;
            }
            return to_storage(ctx);
        }

        HPX_CORE_EXPORT void pop_zone(
            zone_ctx_storage const& ctx_value) noexcept
        {
            if (!ctx_is_empty(ctx_value))
            {
                TracyCZoneEnd(to_tracy(ctx_value));
            }

            // A zone opened before the task had a fiber, or one that
            // resume_fiber_zone dropped from the count after a reconnect, is
            // not counted; the check keeps the count from wrapping around.
            auto& fz = current_fiber_zone();
            if (ctx_value.active && in_fiber() && fz.zones_above != 0)
            {
                --fz.zones_above;
            }
        }
    }    // namespace detail
}    // namespace hpx::tracy

#endif
