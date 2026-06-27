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
#include <string>

#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

namespace hpx::tracy {

    void set_thread_name(char const* name) noexcept
    {
        ::tracy::SetThreadName(name);
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
    void create_counter(std::string const& name) noexcept
    {
        ::TracyPlotConfig(
            name.c_str(), ::tracy::PlotFormatType::Number, true, false, 0);
    }

    // Pass a plot value to Tracy
    void sample_value(std::string const& name, double const value) noexcept
    {
        ::TracyPlot(name.c_str(), value);
    }
}    // namespace hpx::tracy

#endif
