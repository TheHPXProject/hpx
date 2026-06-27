//  Copyright (c) 2015 John Biddiscombe
//  Copyright (c) 2015-2023 Hartmut Kaiser
//  Copyright (c) 2015-2019 Francisco Jose Tapia
//  Copyright (c) 2018 Taeguk Kwon
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>

#include <algorithm>
#include <cstddef>

namespace hpx::parallel::detail {

    /// Return the iterator to the mid value of the three values
    /// passed as parameters
    ///
    /// \param iter_1 : iterator to the first value
    /// \param iter_2 : iterator to the second value
    /// \param iter_3 : iterator to the third value
    /// \param comp : object for comparing two values
    /// \return iterator to mid value
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Comp>
    constexpr Iter mid3(Iter iter_1, Iter iter_2, Iter iter_3, Comp&& comp)
    {
        return HPX_INVOKE(comp, *iter_1, *iter_2) ?
            (HPX_INVOKE(comp, *iter_2, *iter_3)        ? iter_2 :
                    HPX_INVOKE(comp, *iter_1, *iter_3) ? iter_3 :
                                                         iter_1) :
            HPX_INVOKE(comp, *iter_3, *iter_2) ? iter_2 :
            HPX_INVOKE(comp, *iter_3, *iter_1) ? iter_3 :
                                                 iter_1;
    }

    /// Return the iterator to the mid value of the nine values
    /// passed as parameters
    //
    /// \param iter_1   iterator to the first value
    /// \param iter_2   iterator to the second value
    /// \param iter_3   iterator to the third value
    /// \param iter_4   iterator to the fourth value
    /// \param iter_5   iterator to the fifth value
    /// \param iter_6   iterator to the sixth value
    /// \param iter_7   iterator to the seventh value
    /// \param iter_8   iterator to the eighth value
    /// \param iter_9   iterator to the ninth value
    /// \param comp : object for comparing two values
    /// \return iterator to the mid value
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Comp>
    inline constexpr Iter mid9(Iter iter_1, Iter iter_2, Iter iter_3,
        Iter iter_4, Iter iter_5, Iter iter_6, Iter iter_7, Iter iter_8,
        Iter iter_9, Comp&& comp)
    {
        return mid3(mid3(iter_1, iter_2, iter_3, comp),
            mid3(iter_4, iter_5, iter_6, comp),
            mid3(iter_7, iter_8, iter_9, comp), comp);
    }

    /// Receive a range between first and last, obtain 9 values
    /// between the elements  including the first and the previous
    /// to the last. Obtain the iterator to the mid value and swap
    /// with the first position
    //
    /// \param first    iterator to the first element
    /// \param last     iterator to the last element
    /// \param comp     object for to Comp two elements
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Comp>
    constexpr void pivot9(Iter first, Iter last, Comp&& comp)
    {
        std::size_t chunk = (last - first) >> 3;
        Iter itaux = mid9(first + 1, first + chunk, first + 2 * chunk,
            first + 3 * chunk, first + 4 * chunk, first + 5 * chunk,
            first + 6 * chunk, first + 7 * chunk, last - 1, comp);

        std::ranges::iter_swap(first, itaux);
    }
}    // namespace hpx::parallel::detail
