//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/async_base/query_dispatch.hpp>

#include <utility>

namespace hpx::execution::experimental::detail {

    // CPO tag that prefers target.query(tag, args...) and otherwise invokes
    // Fallback{}(target, args...).
    template <typename Tag, typename Fallback>
    struct query_first_tag_fallback
    {
    protected:
        constexpr query_first_tag_fallback() = default;

    public:
        template <typename Target, typename... Args>
            requires(
                hpx::execution::experimental::has_query_v<Target, Tag, Args...>)
        constexpr auto operator()(Target&& target, Args&&... args) const
        {
            return HPX_FORWARD(Target, target)
                .query(Tag{}, HPX_FORWARD(Args, args)...);
        }

        template <typename Target, typename... Args>
            requires(!hpx::execution::experimental::has_query_v<Target, Tag,
                Args...>)
        constexpr auto operator()(Target&& target, Args&&... args) const
            noexcept(noexcept(Fallback{}(
                HPX_FORWARD(Target, target), HPX_FORWARD(Args, args)...)))
                -> decltype(Fallback{}(
                    HPX_FORWARD(Target, target), HPX_FORWARD(Args, args)...))
        {
            return Fallback{}(
                HPX_FORWARD(Target, target), HPX_FORWARD(Args, args)...);
        }
    };

}    // namespace hpx::execution::experimental::detail
