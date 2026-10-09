//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2014-2016 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpxinspect:nodeprecatedinclude:boost/chrono/chrono.hpp
// hpxinspect:nodeprecatedname:boost::chrono

/// \file steady_clock.hpp
/// \page hpx::chrono::steady_time_point
/// \page hpx::chrono::steady_duration
/// \page hpx::chrono::null_duration
/// \headerfile hpx/chrono.hpp

#pragma once

#include <hpx/config.hpp>

#include <chrono>

namespace hpx::chrono {

    HPX_CXX_CORE_EXPORT using std::chrono::steady_clock;

    HPX_CXX_CORE_EXPORT class steady_time_point
    {
        using value_type = steady_clock::time_point;

    public:
        /*implicit*/ constexpr steady_time_point(
            value_type const& abs_time) noexcept
          : _abs_time(abs_time)
        {
        }

        template <typename Clock, typename Duration>
        /*implicit*/ constexpr steady_time_point(
            std::chrono::time_point<Clock, Duration> const& abs_time) noexcept
          : _abs_time(std::chrono::time_point_cast<value_type::duration>(
                steady_clock::now() + (abs_time - Clock::now())))
        {
        }

        [[nodiscard]] constexpr value_type const& value() const noexcept
        {
            return _abs_time;
        }

    private:
        value_type _abs_time;
    };

    HPX_CXX_CORE_EXPORT class steady_duration
    {
        using value_type = steady_clock::duration;

    public:
        constexpr steady_duration() noexcept
          : _rel_time(0)
        {
        }

        /*implicit*/ constexpr steady_duration(
            value_type const& rel_time) noexcept
          : _rel_time(rel_time)
        {
        }

        template <typename Rep, typename Period>
        /*implicit*/ constexpr steady_duration(
            std::chrono::duration<Rep, Period> const& rel_time) noexcept
          : _rel_time(std::chrono::duration_cast<value_type>(rel_time))
        {
            if (_rel_time < rel_time)
                ++_rel_time;
        }

        [[nodiscard]] constexpr value_type const& value() const noexcept
        {
            return _rel_time;
        }

        [[nodiscard]] steady_clock::time_point from_now() const noexcept
        {
            return steady_clock::now() + _rel_time;
        }

    private:
        value_type _rel_time;
    };

    HPX_CXX_CORE_EXPORT inline constexpr steady_duration null_duration{};

}    // namespace hpx::chrono
