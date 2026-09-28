//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/iterator_support.hpp>

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
            using index_iterator = hpx::util::counting_iterator<std::size_t>;
            struct access_position
            {
                std::shared_ptr<storage> state;

                decltype(auto) operator()(index_iterator const& it) const
                {
                    return *state->positions[*it];
                }
            };
            auto state = std::make_shared<storage>(shape);
            auto const& owned_shape = std::as_const(state->shape);
            auto const last = std::ranges::end(owned_shape);
            for (auto it = std::ranges::begin(owned_shape); it != last; ++it)
                state->positions.push_back(it);

            auto const size = state->positions.size();
            // Bulk backends also use legacy iterator traits and standard
            // integral counts. HPX iterators provide both without depending
            // on the library's iota/transform view iterator representation.
            auto const access = access_position{HPX_MOVE(state)};
            return hpx::util::iterator_range(
                hpx::util::transform_iterator(index_iterator(0), access),
                hpx::util::transform_iterator(index_iterator(size), access));
        }
    }

    /// \endcond
}    // namespace hpx::execution::experimental::detail
