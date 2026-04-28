//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <ranges>
#include <type_traits>

namespace hpx::parallel::traits {

    ///////////////////////////////////////////////////////////////////////////
    template <typename F, typename Rng, typename Enable = void>
    struct projected_range_result_of
    {
    };

    template <typename Proj, typename Rng>
    struct projected_range_result_of<Proj, Rng,
        std::enable_if_t<std::ranges::range<Rng>>>
      : detail::projected_result_of<std::decay_t<Proj>,
            std::ranges::iterator_t<Rng>>
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Proj, typename Rng, typename Enable = void>
    struct is_projected_range : std::false_type
    {
    };

    template <typename Proj, typename Rng>
    struct is_projected_range<Proj, Rng,
        std::enable_if_t<std::ranges::range<Rng>>>
      : detail::is_projected<std::decay_t<Proj>, std::ranges::iterator_t<Rng>>
    {
    };

    template <typename Proj, typename Rng>
    inline constexpr bool is_projected_range_v =
        is_projected_range<Proj, Rng>::value;

    ///////////////////////////////////////////////////////////////////////////
    template <typename Proj, typename Rng, typename Enable = void>
    struct projected_range
    {
    };

    template <typename Proj, typename Rng>
    struct projected_range<Proj, Rng, std::enable_if_t<std::ranges::range<Rng>>>
    {
        using projector_type = std::decay_t<Proj>;
        using iterator_type = std::ranges::iterator_t<Rng>;
    };
}    // namespace hpx::parallel::traits
