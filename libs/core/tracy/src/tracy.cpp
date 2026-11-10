//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_TRACY)
#include <hpx/tracy/tracy.hpp>
#include <hpx/tracy/tracy_tls.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_set>

#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

namespace hpx::tracy {

    namespace {

        // Tracy keys a plot by the address of its name and reads the name
        // later, so it must stay valid and be the same for the same name.
        // The set is never freed, as Tracy can still read names during exit.
        char const* plot_name(std::string const& name)
        {
            struct names_type
            {
                std::mutex mtx;
                std::unordered_set<std::string> names;
            };
            static names_type* const names = new names_type;

            std::lock_guard<std::mutex> l(names->mtx);
            return names->names.insert(name).first->c_str();
        }
    }    // namespace

    void set_thread_name(char const* name) noexcept
    {
        ::tracy::SetThreadName(name);
    }

    bool is_profiler_connected() noexcept
    {
        return ___tracy_connected() != 0;
    }

    void emit_appinfo(char const* text, std::size_t size) noexcept
    {
        TracyCAppInfo(text, size);
    }

    namespace detail {

        // Expose Tracy fibers support
        HPX_CORE_EXPORT void enter_fiber(char const* fiber_name,
            char const* zone_name, std::size_t color) noexcept
        {
            // Mark TLS so rename_region / mark_event are no-ops inside
            // the fiber.
            set_in_fiber(true);

            // Switch Tracy's zone stack to this fiber.
            ::TracyFiberEnter(fiber_name);

            // Open a zone on the fiber's zone stack - this makes the fiber
            // track visible in Tracy as a colored bar labeled with zone_name.
            start_fiber_zone(zone_name, static_cast<std::uint32_t>(color));
        }

        HPX_CORE_EXPORT void leave_fiber() noexcept
        {
            // Close the fiber zone BEFORE leaving the fiber context, so that
            // TracyCZoneEnd goes to the fiber's zone stack, not the OS
            // thread's.
            stop_fiber_zone();

            ::TracyFiberLeave;
            set_in_fiber(false);
        }

    }    // namespace detail

    // Create a new plot in Tracy
    void create_counter(char const* name) noexcept
    {
        ::TracyPlotConfig(
            name, ::tracy::PlotFormatType::Number, true, false, 0);
    }

    void create_counter(std::string const& name) noexcept
    {
        create_counter(plot_name(name));
    }

    // Pass a plot value to Tracy
    void sample_value(char const* name, double const value) noexcept
    {
        ::TracyPlot(name, value);
    }

    void sample_value(std::string const& name, double const value) noexcept
    {
        sample_value(plot_name(name), value);
    }

    void message(
        char const* text, std::size_t size, std::uint32_t color) noexcept
    {
        if (color == 0)
        {
            TracyCMessageS(text, size, 10);
        }
        else
        {
            TracyCMessageCS(text, size, color, 10);
        }
    }

    void frame_mark(char const* name) noexcept
    {
        if (name && name[0] != '\0')
        {
            TracyCFrameMarkNamed(name);
        }
        else
        {
            TracyCFrameMark;
        }
    }
}    // namespace hpx::tracy

#endif
