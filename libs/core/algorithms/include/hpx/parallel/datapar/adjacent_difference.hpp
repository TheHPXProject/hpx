//  Copyright (c) 2021 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/adjacent_difference.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/datapar/loop.hpp>
#include <hpx/parallel/datapar/zip_iterator.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_adjacent_difference
    {
        template <typename InIter, typename OutIter, typename Op>
        static constexpr OutIter call(
            InIter first, InIter last, OutIter dest, Op&& op)
        {
            if (first == last)
            {
                return dest;
            }

            auto count = hpx::parallel::detail::distance(first, last) - 1;

            InIter prev = first;
            *dest++ = *first++;

            if (count == 0)
            {
                return dest;
            }

            using hpx::get;

            util::loop_n<std::decay_t<ExPolicy>>(
                hpx::util::zip_iterator(first, prev, dest), count,
                [op](auto&& it) mutable {
                    get<2>(*it) = HPX_INVOKE(op, get<0>(*it), get<1>(*it));
                });

            std::advance(dest, count);
            return dest;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename OutIter, typename Op>
    // clang-format off
        requires (
            hpx::is_vectorpack_execution_policy_v<ExPolicy> &&
            hpx::parallel::util::detail::iterator_datapar_compatible_v<InIter>
        )
    // clang-format on
    constexpr OutIter tag_invoke(sequential_adjacent_difference_t<ExPolicy>,
        InIter first, InIter last, OutIter dest, Op&& op)
    {
        return datapar_adjacent_difference<ExPolicy>::call(
            first, last, dest, HPX_FORWARD(Op, op));
    }
}    // namespace hpx::parallel::detail

#endif
