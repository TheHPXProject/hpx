//  Copyright (c) 2021 Srinivas Yadav
//  Copyright (c) 2021 Karame M.shokooh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_adjacent_difference_t
      : hpx::detail::tag_dispatch<sequential_adjacent_difference_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename InIter, typename Sent, typename OutIter, typename Op>
        static constexpr inline OutIter invoke_default(
            InIter first, Sent last, OutIter dest, Op&& op)
        {
            if (first == last)
                return dest;

            using value_t = typename std::iterator_traits<InIter>::value_type;
            value_t acc = *first;
            *dest = acc;
            while (++first != last)
            {
                value_t val = *first;
                *++dest = HPX_INVOKE(op, val, HPX_MOVE(acc));
                acc = HPX_MOVE(val);
            }
            return ++dest;
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_adjacent_difference_t<ExPolicy>
        sequential_adjacent_difference =
            sequential_adjacent_difference_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename Sent, typename OutIter, typename Op>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr OutIter
    sequential_adjacent_difference(
        InIter first, Sent last, OutIter dest, Op&& op)
    {
        return sequential_adjacent_difference_t<ExPolicy>{}(
            first, last, dest, HPX_FORWARD(Op, op));
    }
#endif

}    // namespace hpx::parallel::detail
