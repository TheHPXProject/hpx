//  Copyright (c) 2019-2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>

#include <algorithm>
#include <iterator>

namespace hpx::parallel::detail {

    // provide implementation of std::rotate supporting iterators/sentinels
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent>
    constexpr void sequential_rotate_helper(
        Iter first, Iter new_first, Sent last)
    {
        Iter next = new_first;
        while (first != next)
        {
            std::ranges::iter_swap(first++, next++);
            if (next == last)
            {
                next = new_first;
            }
            else if (first == new_first)
            {
                new_first = next;
            }
        }
    }

    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent>
    constexpr util::in_out_result<Iter, Sent> sequential_rotate(
        Iter first, Iter new_first, Sent last)
    {
        if (first != new_first && new_first != last)
            sequential_rotate_helper(first, new_first, last);

        detail::advance(first, detail::distance(new_first, last));
        return {first, last};
    }
}    // namespace hpx::parallel::detail
