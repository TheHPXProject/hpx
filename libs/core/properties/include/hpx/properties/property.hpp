//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2023-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <concepts>
#include <utility>

namespace hpx::experimental {

    // prefer(tag, obj, prop) calls tag(obj, prop) if invocable, else
    // returns obj unchanged.
    HPX_CXX_CORE_EXPORT inline constexpr struct prefer_t
    {
        // call tag(tn...) when invocable
        // clang-format off
        template <typename Tag, typename... Tn>
            requires std::invocable<Tag, Tn&&...>
        constexpr HPX_FORCEINLINE auto operator()(Tag tag, Tn&&... tn) const
            noexcept(noexcept(tag(HPX_FORWARD(Tn, tn)...)))
            -> decltype(tag(HPX_FORWARD(Tn, tn)...))
        // clang-format on
        {
            return tag(HPX_FORWARD(Tn, tn)...);
        }

        // return t0 unchanged when tag is not invocable
        // clang-format off
        template <typename Tag, typename T0, typename... Tn>
            requires (!std::invocable<Tag, T0, Tn...>)
        constexpr HPX_FORCEINLINE auto operator()(Tag, T0&& t0, Tn&&...) const
            noexcept(noexcept(HPX_FORWARD(T0, t0)))
            -> decltype(HPX_FORWARD(T0, t0))
        // clang-format on
        {
            return HPX_FORWARD(T0, t0);
        }
    } prefer{};
}    // namespace hpx::experimental
