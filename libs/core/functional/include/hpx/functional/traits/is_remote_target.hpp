//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>

namespace hpx::traits {

    // Whether t in hpx::async<&f>(t, ...) is a remote target rather than the
    // first argument of a local call of f. Core knows no remote targets, the
    // distributed runtime specializes this with its target types and traits,
    // so the answer is the same wherever such a type is complete.
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_remote_target : std::false_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_remote_target_v =
        is_remote_target<std::decay_t<T>>::value;
}    // namespace hpx::traits
