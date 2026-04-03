//  Copyright (c) 2025 Bhoomish Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/execution/traits/vector_pack_find.hpp>
#include <hpx/execution/traits/vector_pack_get_set.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/remove.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/datapar/loop.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_remove_if
    {
        template <typename Iter, typename Sent, typename Pred, typename Proj>
        static inline Iter call(
            ExPolicy&&, Iter first, Sent last, Pred pred, Proj proj)
        {
            using value_type = typename std::iterator_traits<Iter>::value_type;
            using V = hpx::parallel::traits::vector_pack_type_t<value_type>;
            constexpr std::size_t size =
                hpx::parallel::traits::vector_pack_size_v<V>;

            Iter dest = first;

            while (first != last && !util::detail::is_data_aligned(first))
            {
                if (!HPX_INVOKE(pred, HPX_INVOKE(proj, *first)))
                {
                    if (dest != first)
                        *dest = HPX_MOVE(*first);
                    ++dest;
                }
                ++first;
            }

            while (
                last - first >= static_cast<std::ptrdiff_t>(size))    //Safety
            {
                V tmp(hpx::parallel::traits::vector_pack_load<V,
                    value_type>::aligned(first));

                auto msk = HPX_INVOKE(pred, HPX_INVOKE(proj, tmp));

                if (hpx::parallel::traits::none_of(msk))
                {
                    //no elements match
                    if (dest != first)
                    {
                        if (util::detail::is_data_aligned(dest))
                        {
                            hpx::parallel::traits::vector_pack_store<V,
                                value_type>::aligned(tmp, dest);
                        }
                        else
                        {
                            hpx::parallel::traits::vector_pack_store<V,
                                value_type>::unaligned(tmp, dest);
                        }
                    }
                    std::advance(dest, size);
                }
                else if (!hpx::parallel::traits::all_of(msk))
                {
                    //mixed
                    int first_match = hpx::parallel::traits::find_first_of(msk);

                    for (int i = 0; i < first_match; ++i)
                    {
                        *dest++ =
                            value_type(hpx::parallel::traits::get(tmp, i));
                    }

                    for (std::size_t i = static_cast<std::size_t>(first_match);
                        i < size; ++i)
                    {
                        bool match = false;
                        if constexpr (std::is_class_v<
                                          std::decay_t<decltype(msk)>>)
                        {
#if defined(HPX_HAVE_DATAPAR_EVE)
                            match = msk.get(i);
#else
                            match = msk[i];
#endif
                        }
                        else
                            match = msk;

                        if (!match)
                        {
                            *dest++ =
                                value_type(hpx::parallel::traits::get(tmp, i));
                        }
                    }
                }
                //all elements match
                std::advance(first, size);
            }

            while (first != last)
            {
                if (!HPX_INVOKE(pred, HPX_INVOKE(proj, *first)))
                {
                    if (dest != first)
                        *dest = HPX_MOVE(*first);
                    ++dest;
                }
                ++first;
            }

            return dest;
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
            ExPolicy&& policy, Iter first, Sent last, T const& value, Proj proj)
        {
            return datapar_remove_if<ExPolicy>::call(
                HPX_FORWARD(ExPolicy, policy), first, last,
                [&value](auto const& a) { return a == value; }, proj);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename T, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE Iter tag_invoke(
        sequential_remove_t<ExPolicy>, ExPolicy&& policy, Iter first, Sent last,
        T const& value, Proj proj)
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
