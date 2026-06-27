//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <utility>

namespace hpx::traits {

    HPX_CXX_CORE_EXPORT template <typename Result, typename RemoteResult,
        typename Enable = void>
    struct get_remote_result
    {
        static Result call(RemoteResult const& rhs)
        {
            return Result(rhs);
        }

        static Result call(RemoteResult&& rhs) noexcept    //-V659
        {
            return Result(HPX_MOVE(rhs));
        }
    };

    template <typename Result>
    struct get_remote_result<Result, Result>
    {
        static constexpr Result const& call(Result const& rhs) noexcept
        {
            return rhs;
        }

        static constexpr Result&& call(Result&& rhs) noexcept
        {
            return HPX_MOVE(rhs);
        }
    };
}    // namespace hpx::traits
