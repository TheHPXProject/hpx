//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    /// \enum task_size
    ///
    /// The expected run time of a task, a hint the scheduler may use. A task
    /// with an unknown size is scheduled as it would be without the hint. The
    /// sizes are ordered by run time, see task_size_duration.
    HPX_CXX_CORE_EXPORT enum class task_size : std::uint8_t {
        unknown = 0,
        nano = 1,      ///< about a nanosecond, mostly for loop bodies
        small_ = 2,    ///< the underscore is to work around 'small' being
                       ///  defined to char on Windows
        medium = 3,
        large = 4
    };

    /// The run time a task of the given \p size stands for. Each size stands
    /// for a hundred times the run time of the one below it: nano about 1 ns,
    /// small about 100 ns, medium about 10 us, large about 1 ms or more.
    /// An unknown size gives a duration of zero. Callers should rely on
    /// std::chrono arithmetic only, not on the unit or the count of the
    /// returned duration, which may change once tasks can carry real run
    /// times.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr std::chrono::nanoseconds
    task_size_duration(task_size const size) noexcept
    {
        using namespace std::chrono_literals;
        switch (size)
        {
        case task_size::nano:
            return 1ns;
        case task_size::small_:
            return 100ns;
        case task_size::medium:
            return 10us;
        case task_size::large:
            return 1ms;
        default:
            return 0ns;
        }
    }

    /// The size of a task that calls a function of the given \p size \p calls
    /// times, for instance a hundred nano calls make a small task, a hundred
    /// small calls a medium one and a hundred medium calls a large one. The
    /// size of a task calling a function of unknown size is unknown.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr task_size bulk_task_size(
        task_size const size, std::size_t const calls) noexcept
    {
        if (size == task_size::large)
            return task_size::large;

        auto const per_call = task_size_duration(size);
        if (per_call == per_call.zero())
            return task_size::unknown;

        // the run time of all calls, limited to avoid overflow
        auto const limit = task_size_duration(task_size::large);
        auto const max_calls = limit / per_call;
        auto const total = calls > static_cast<std::size_t>(max_calls) ?
            limit :
            per_call * static_cast<std::chrono::nanoseconds::rep>(calls);

        for (task_size const s :
            {task_size::large, task_size::medium, task_size::small_})
        {
            if (total >= task_size_duration(s))
                return s;
        }
        return task_size::nano;
    }
}    // namespace hpx::threads

namespace hpx {

    HPX_CXX_CORE_EXPORT using threads::task_size;
    HPX_CXX_CORE_EXPORT using threads::task_size_duration;
    HPX_CXX_CORE_EXPORT using threads::bulk_task_size;
}    // namespace hpx

namespace hpx::traits {

    // By default, we don't know how long the function runs. Size stays
    // unknown for callers; it is an extension point for specializations,
    // which pass it on to the function they wrap.
    //
    // value is the size known from the type of F. call(f) returns the size of
    // the function object f, which can be known only at run time, for
    // instance for a function stored in an hpx::function. Specializations for
    // wrappers forward call to the function they wrap, the same way as the
    // ones of get_function_annotation do.
    HPX_CXX_CORE_EXPORT template <typename F,
        threads::task_size Size = threads::task_size::unknown,
        typename Enable = void>
    struct get_task_size
    {
        static constexpr threads::task_size value = Size;

        [[nodiscard]] static constexpr threads::task_size call(
            F const& /*f*/) noexcept
        {
            return Size;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename F,
        threads::task_size Size = threads::task_size::unknown>
    inline constexpr threads::task_size get_task_size_v =
        get_task_size<F, Size>::value;
}    // namespace hpx::traits
