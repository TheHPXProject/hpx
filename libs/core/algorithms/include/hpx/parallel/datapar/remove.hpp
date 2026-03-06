//  Copyright (c) 2026 Bhoomish Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/find.hpp>
#include <hpx/parallel/algorithms/detail/remove.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/datapar/loop.hpp>

#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_remove_if
    {
        template <typename Iter, typename Sent, typename Pred, typename Proj>
        static inline Iter call(
            ExPolicy&& policy, Iter first, Sent last, Pred pred, Proj proj)
        {
            first = hpx::parallel::detail::sequential_find_if<ExPolicy>(
                first, last, pred, proj);

            if (first != last)
            {
                for (Iter i = first; ++i != last;)
                    if (!HPX_INVOKE(pred, HPX_INVOKE(proj, *i)))
                    {
                        *first++ = HPX_MOVE(*i);
                    }
            }
            return first;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename Pred, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE Iter tag_invoke(
        sequential_remove_if_t<ExPolicy>, ExPolicy&& policy, Iter first,
        Sent last, Pred pred, Proj proj)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          Iter>::value)
        {
            return datapar_remove_if<ExPolicy>::call(
                HPX_FORWARD(ExPolicy, policy), first, last, pred, proj);
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_remove_if<base_policy_type>(
                hpx::execution::experimental::to_non_simd(policy), first, last,
                pred, proj);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_remove
    {
        template <typename Iter, typename Sent, typename T, typename Proj>
        static inline Iter call(
            ExPolicy&& policy, Iter first, Sent last, T const& value,
            Proj proj)
        {
            return datapar_remove_if<ExPolicy>::call(
                HPX_FORWARD(ExPolicy, policy), first, last,
                [&value](auto const& a) { return value == a; },
                proj);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename T, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE Iter tag_invoke(
        sequential_remove_t<ExPolicy>, ExPolicy&& policy, Iter first,
        Sent last, T const& value, Proj proj)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          Iter>::value)
        {
            return datapar_remove<ExPolicy>::call(
                HPX_FORWARD(ExPolicy, policy), first, last, value, proj);
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_remove<base_policy_type>(
                hpx::execution::experimental::to_non_simd(policy), first, last,
                value, proj);
        }
    }

}    // namespace hpx::parallel::detail

#endif
