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
#include <hpx/timing/durations.hpp>

#include <chrono>
#include <cstdint>

namespace hpx::chrono {

    HPX_CXX_CORE_EXPORT using std::chrono::steady_clock;

    /// Represents a time point measured by the HPX steady clock.
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
          , _below_ps(0)
        {
        }

        /*implicit*/ constexpr steady_duration(
            value_type const& rel_time) noexcept
          : _rel_time(rel_time)
          , _below_ps(0)
        {
        }

        // A duration that isn't a whole number of clock ticks is rounded up
        // (value), and the part of the last tick it doesn't fill is kept, in
        // picoseconds, so that as_picoseconds gives the duration itself. This
        // is done for durations below an hour, the longest a run time of a
        // loop iteration needs; longer ones are only rounded up.
        template <typename Rep, typename Period>
        /*implicit*/ constexpr steady_duration(
            std::chrono::duration<Rep, Period> const& rel_time) noexcept
          : _rel_time(std::chrono::duration_cast<value_type>(rel_time))
          , _below_ps(0)
        {
            if (_rel_time < rel_time)
            {
                ++_rel_time;
                if (_rel_time < std::chrono::hours(1))
                {
                    auto const below =
                        std::chrono::duration_cast<picoseconds>(_rel_time) -
                        std::chrono::duration_cast<picoseconds>(rel_time);
                    if (below.count() > 0 && below < value_type(1))
                    {
                        _below_ps = static_cast<std::uint16_t>(below.count());
                    }
                }
            }
        }

        [[nodiscard]] constexpr value_type const& value() const noexcept
        {
            return _rel_time;
        }

        /// The duration in picoseconds, also below a clock tick: a duration
        /// given in picoseconds (or as a fraction of a tick) is returned as
        /// it was given. Saturates at the largest number of picoseconds, about
        /// 106 days.
        [[nodiscard]] constexpr picoseconds as_picoseconds() const noexcept
        {
            constexpr auto limit =
                std::chrono::duration_cast<value_type>((picoseconds::max) ());
            if (_rel_time >= limit)
            {
                return (picoseconds::max) ();
            }
            if (_rel_time <= -limit)
            {
                return (picoseconds::min) ();
            }
            return std::chrono::duration_cast<picoseconds>(_rel_time) -
                picoseconds(_below_ps);
        }

        [[nodiscard]] steady_clock::time_point from_now() const noexcept
        {
            return steady_clock::now() + _rel_time;
        }

        // A duration is true if it is not zero, in particular, null_duration
        // is false.
        [[nodiscard]] explicit constexpr operator bool() const noexcept
        {
            return _rel_time != value_type::zero();
        }

    private:
        value_type _rel_time;

        // the part of the last tick of _rel_time the duration doesn't fill,
        // in picoseconds (see as_picoseconds)
        std::uint16_t _below_ps;
    };

    HPX_CXX_CORE_EXPORT inline constexpr steady_duration null_duration{};

}    // namespace hpx::chrono
