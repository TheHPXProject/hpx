//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2019-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/type_support.hpp>

namespace hpx::parallel::detail {

    template <typename Iter, typename Sent, typename Compare,
        typename Proj = hpx::identity>
    constexpr bool is_sorted_sequential(
        Iter first, Sent last, Compare&& comp, Proj&& proj = Proj())
    {
        bool sorted = true;
        if (first != last)
        {
            for (Iter it1 = first, it2 = ++first; it2 != last &&
                ((sorted = !HPX_INVOKE(
                      comp, HPX_INVOKE(proj, *it2), HPX_INVOKE(proj, *it1))));
                it1 = it2++)
            {
                /**/
            }
        }
        return sorted;
    }

    template <typename Iter, typename Sent, typename Compare,
        typename Proj = hpx::identity>
    constexpr Iter is_sorted_until_sequential(
        Iter first, Sent last, Compare&& comp, Proj&& proj = Proj())
    {
        if (first != last)
        {
            Iter it1 = first;
            Iter it2 = ++first;
            for (; it2 != last &&
                !HPX_INVOKE(
                    comp, HPX_INVOKE(proj, *it2), HPX_INVOKE(proj, *it1));
                it1 = it2++)
            {
                /**/
            }

            return it2;
        }
        return first;
    }
}    // namespace hpx::parallel::detail
