//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

namespace hpx::execution::experimental::detail {
    /// \cond NOINTERNAL

    // Keep the existing constant-time path for random access shapes. Other
    // multipass shapes are indexed once, preserving references to elements
    // and keeping the copied shape alive for asynchronous work.
    template <typename S>
        requires std::ranges::forward_range<S const>
    decltype(auto) make_indexed_shape(S& shape)
    {
        if constexpr (std::ranges::random_access_range<S const> &&
            std::ranges::sized_range<S const>)
        {
            return (shape);
        }
        else
        {
            struct storage
            {
                S shape;
                std::vector<std::ranges::iterator_t<S const>> positions;
            };
            auto state = std::make_shared<storage>(shape);
            auto const& owned_shape = std::as_const(state->shape);
            auto const last = std::ranges::end(owned_shape);
            for (auto it = std::ranges::begin(owned_shape); it != last; ++it)
                state->positions.push_back(it);

            auto const size = state->positions.size();
            return std::views::iota(std::size_t(0), size) |
                std::views::transform(
                    [state = HPX_MOVE(state)](std::size_t i) -> decltype(auto) {
                        return *state->positions[i];
                    });
        }
    }

    /// \endcond
}    // namespace hpx::execution::experimental::detail
