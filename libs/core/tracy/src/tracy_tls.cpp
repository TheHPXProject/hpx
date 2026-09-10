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

        // True while this OS thread is executing inside a Tracy fiber context.
        // rename_region (called by mark_event) must be a no-op in that case:
        // the zone ctx in TLS belongs to the OS-thread zone opened before
        // FiberEnter, but Tracy's internal zone stack has switched to the
        // fiber's stack - calling TracyCZoneName on the OS zone while inside
        // a fiber causes a "zone name destination doesn't match" crash.
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

        // Store the zone ctx that was opened on the fiber's stack.
        // enter_fiber opens a zone (so the fiber track is visible in Tracy),
        // and leave_fiber closes it.
        // zone_name and color are cached so that suspend_fiber_zone can
        // reopen the running zone with the same label/color after the
        // task resumes from self_.yield().
        struct fiber_zone_data
        {
            zone_ctx_storage ctx_value{};
            char const* zone_name = "fiber";
            std::uint32_t color = 0;
            bool active = false;
            bool just_entered = false;
        };

        fiber_zone_data& current_fiber_zone() noexcept
        {
            thread_local fiber_zone_data fz;
            return fz;
        }

        void open_fiber_zone(fiber_zone_data& fz, char const* zone_name,
            std::uint32_t color) noexcept
        {
            TracyCZoneC(ctx, color, 1);
            TracyCZoneName(ctx, zone_name, std::strlen(zone_name));

            fz.ctx_value = to_storage(ctx);
            fz.active = true;
        }
    }    // namespace

    namespace detail {

        HPX_CORE_EXPORT region_data start_region(char const* new_region,
            std::size_t const thread_num, std::size_t const phase) noexcept
        {
#if defined(HPX_HAVE_STACKTRACES)
            TracyCZoneCS(ctx, static_cast<std::uint32_t>(thread_num),
                HPX_HAVE_THREAD_BACKTRACE_DEPTH, 1);
#else
            TracyCZoneC(ctx, static_cast<std::uint32_t>(thread_num), 1);
#endif
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

            auto& fz = current_fiber_zone();
            open_fiber_zone(fz, safe_zone_name, color);
            fz.zone_name = safe_zone_name;
            fz.color = color;
            fz.just_entered = true;
        }

        HPX_CORE_EXPORT void stop_fiber_zone() noexcept
        {
            auto& fz = current_fiber_zone();
            if (fz.active)
            {
                TracyCZoneEnd(to_tracy(fz.ctx_value));
                fz.active = false;
                fz.ctx_value = {};
            }
        }

        // Close the running fiber zone and open a "suspended" zone (grey) so
        // the fiber track shows a distinct bar during the suspension gap.
        // Called just before self_.yield() inside execution_agent::do_yield().
        // Only touches current_fiber_zone() - never calls stop_region() so the
        // OS-thread zone (current_region()) is completely untouched.
        HPX_CORE_EXPORT void suspend_fiber_zone(
            char const* suspend_reason) noexcept
        {
            auto& fz = current_fiber_zone();
            fz.just_entered = false;
            if (!fz.active)
                return;

            // Close the running zone.
            TracyCZoneEnd(to_tracy(fz.ctx_value));
            fz.active = false;

            // Open a grey "suspended" zone on the fiber stack.
            // 0xAAAAAA = medium grey, distinguishable from any
            // worker-index color.
            constexpr std::uint32_t suspended_color = 0xAAAAAA;
            char const* safe_reason =
                intern_zone_label(suspend_reason, "suspend");

            open_fiber_zone(fz, safe_reason, suspended_color);
            // zone_name and color are preserved from start_fiber_zone so that
            // resume_fiber_zone can restore the original running zone.
        }

        // Close the "suspended" zone and reopen the original running zone.
        // Called immediately after self_.yield() returns inside do_yield(),
        // i.e. when the task has been rescheduled onto a worker thread.
        // zone_name / color default to nullptr/0 which means "use the cached
        // values from start_fiber_zone".
        HPX_CORE_EXPORT void resume_fiber_zone(
            char const* zone_name, std::uint32_t color) noexcept
        {
            auto& fz = current_fiber_zone();
            if (fz.just_entered)
            {
                fz.just_entered = false;
                return;
            }
            if (!fz.active)
                return;

            // Close the suspended (grey) zone.
            TracyCZoneEnd(to_tracy(fz.ctx_value));
            fz.active = false;

            // Reopen the running zone with cached or supplied name/color.
            char const* name =
                (zone_name != nullptr) ? zone_name : fz.zone_name;
            char const* safe_name = intern_zone_label(name, "fiber");
            std::uint32_t col = (color != 0) ? color : fz.color;

            open_fiber_zone(fz, safe_name, col);
        }

        // Embed text into the currently-active fiber zone so it appears in
        // Tracy's "Zone Info" popup when the user clicks the colored bar on
        // the timeline. Uses the same ctx stored by open_fiber_zone.
        HPX_CORE_EXPORT void add_zone_text_to_fiber(
            char const* txt, std::size_t size) noexcept
        {
            auto& fz = current_fiber_zone();
            if (txt == nullptr || size == 0 || !fz.active ||
                ctx_is_empty(fz.ctx_value))
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
            return to_storage(ctx);
        }

        HPX_CORE_EXPORT void pop_zone(
            zone_ctx_storage const& ctx_value) noexcept
        {
            if (!ctx_is_empty(ctx_value))
            {
                TracyCZoneEnd(to_tracy(ctx_value));
            }
        }
    }    // namespace detail
}    // namespace hpx::tracy

#endif
