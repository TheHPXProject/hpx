//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstdint>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    /// \enum task_size
    ///
    /// The expected run time of a task, a hint the scheduler may use. A task
    /// with an unknown size is scheduled as it would be without the hint.
    HPX_CXX_CORE_EXPORT enum class task_size : std::uint8_t {
        unknown = 0,
        small_ = 1,    ///< the underscore is to work around 'small' being
                       ///  defined to char on Windows
        medium = 2,
        large = 3
    };
}    // namespace hpx::threads

namespace hpx {

    HPX_CXX_CORE_EXPORT using threads::task_size;
}    // namespace hpx

namespace hpx::traits {

    // By default, we don't know how long the function runs. Size stays
    // unknown for callers; it is an extension point for specializations,
    // which pass it on to the function they wrap.
    HPX_CXX_CORE_EXPORT template <typename F,
        threads::task_size Size = threads::task_size::unknown,
        typename Enable = void>
    struct get_task_size
    {
        static constexpr threads::task_size value = Size;
    };

    HPX_CXX_CORE_EXPORT template <typename F,
        threads::task_size Size = threads::task_size::unknown>
    inline constexpr threads::task_size get_task_size_v =
        get_task_size<F, Size>::value;
}    // namespace hpx::traits
