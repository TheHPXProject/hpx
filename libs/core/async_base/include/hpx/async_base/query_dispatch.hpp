//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    template <typename Target, typename Tag, typename... Args>
    inline constexpr bool has_query_v =
        requires(Target&& target, Tag tag, Args&&... args) {
            HPX_FORWARD(Target, target).query(tag, HPX_FORWARD(Args, args)...);
        };

    template <typename Target, typename Tag, typename... Args>
    using query_result_t = decltype(std::declval<Target>().query(
        std::declval<Tag>(), std::declval<Args>()...));

}    // namespace hpx::execution::experimental
