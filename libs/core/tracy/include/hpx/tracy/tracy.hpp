//  Copyright (c) 2026 Hartmut Kaiser
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
#include <string>

namespace hpx::tracy {

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_thread_name(
        char const* name) noexcept;

    namespace detail {

        HPX_CORE_EXPORT void enter_fiber(char const* fiber_name,
            char const* zone_name = nullptr, std::size_t color = 0) noexcept;
        HPX_CORE_EXPORT void leave_fiber() noexcept;

    }    // namespace detail

    HPX_CXX_CORE_EXPORT struct fiber_region
    {
        explicit fiber_region(char const* fiber_name,
            char const* zone_name = nullptr, std::size_t const color = 0,
            bool const enabled = true) noexcept
          : active(enabled)
        {
            if (active)
            {
                detail::enter_fiber(fiber_name, zone_name, color);
            }
        }

        ~fiber_region() noexcept
        {
            if (active)
            {
                detail::leave_fiber();
            }
        }

    private:
        bool active;
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void create_counter(
        std::string const& name) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void sample_value(
        std::string const& name, double value) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void message(
        char const* text, std::size_t size, std::uint32_t color) noexcept;

}    // namespace hpx::tracy

#endif
