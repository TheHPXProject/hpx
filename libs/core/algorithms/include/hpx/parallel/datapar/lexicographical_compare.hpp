//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/lexicographical_compare.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/datapar/loop.hpp>
#include <hpx/parallel/datapar/zip_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename Pred, typename Proj1, typename Proj2,
        typename Vector1, typename Vector2 = Vector1>
    inline constexpr bool is_lexicographical_compare_datapar_compatible_v =
        requires(std::decay_t<Pred>& pred, std::decay_t<Proj1>& proj1,
            std::decay_t<Proj2>& proj2, Vector1& v1, Vector2& v2) {
            requires !std::is_convertible_v<
                std::decay_t<decltype(HPX_INVOKE(
                    pred, HPX_INVOKE(proj1, v1), HPX_INVOKE(proj2, v2)))>,
                bool>;
            requires !std::is_convertible_v<
                std::decay_t<decltype(HPX_INVOKE(
                    pred, HPX_INVOKE(proj2, v2), HPX_INVOKE(proj1, v1)))>,
                bool>;
            hpx::parallel::traits::find_first_of(
                HPX_INVOKE(pred, HPX_INVOKE(proj1, v1), HPX_INVOKE(proj2, v2)));
            hpx::parallel::traits::find_first_of(
                HPX_INVOKE(pred, HPX_INVOKE(proj2, v2), HPX_INVOKE(proj1, v1)));
        };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_vectorizable_value_type_v =
        std::is_arithmetic_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_lexicographical_compare
    {
        template <typename InIter1, typename Sent1, typename InIter2,
            typename Sent2, typename Pred, typename Proj1, typename Proj2>
        static bool call(InIter1 first1, Sent1 last1, InIter2 first2,
            Sent2 last2, Pred&& pred, Proj1&& proj1, Proj2&& proj2)
        {
            using policy_t = std::decay_t<ExPolicy>;
            constexpr std::size_t lanes =
                hpx::execution::policy_traits<policy_t>::num_lanes;
            using value_type1 =
                typename std::iterator_traits<InIter1>::value_type;
            using value_type2 =
                typename std::iterator_traits<InIter2>::value_type;

            constexpr bool can_vectorize_values =
                is_vectorizable_value_type_v<value_type1> &&
                is_vectorizable_value_type_v<value_type2>;

            if constexpr (!can_vectorize_values)
            {
                using base_policy_type =
                    decltype(hpx::execution::experimental::to_non_simd(
                        std::declval<ExPolicy>()));
                return sequential_lexicographical_compare<base_policy_type>(
                    first1, last1, first2, last2, HPX_FORWARD(Pred, pred),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
            }
            else
            {
                using vector_type1 =
                    traits::vector_pack_type_t<value_type1, lanes>;
                using vector_type2 =
                    traits::vector_pack_type_t<value_type2, lanes>;

                constexpr std::size_t num_lanes1 =
                    traits::vector_pack_size_v<vector_type1>;
                constexpr std::size_t num_lanes2 =
                    traits::vector_pack_size_v<vector_type2>;

                constexpr bool can_vectorize = (num_lanes1 > 1) &&
                    (num_lanes1 == num_lanes2) &&
                    is_lexicographical_compare_datapar_compatible_v<Pred, Proj1,
                        Proj2, vector_type1, vector_type2>;

                if constexpr (!can_vectorize)
                {
                    using base_policy_type =
                        decltype(hpx::execution::experimental::to_non_simd(
                            std::declval<ExPolicy>()));
                    return sequential_lexicographical_compare<base_policy_type>(
                        first1, last1, first2, last2, HPX_FORWARD(Pred, pred),
                        HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
                }
                else
                {
                    auto count1 = detail::distance(first1, last1);
                    auto count2 = detail::distance(first2, last2);
                    auto count = (std::min) (count1, count2);
                    constexpr std::size_t num_lanes = num_lanes1;

                    while (count >= static_cast<decltype(count)>(num_lanes))
                    {
                        auto v1 = traits::vector_pack_load<vector_type1,
                            value_type1>::unaligned(first1);
                        auto v2 = traits::vector_pack_load<vector_type2,
                            value_type2>::unaligned(first2);

                        auto msk1 = HPX_INVOKE(
                            pred, HPX_INVOKE(proj1, v1), HPX_INVOKE(proj2, v2));
                        auto msk2 = HPX_INVOKE(
                            pred, HPX_INVOKE(proj2, v2), HPX_INVOKE(proj1, v1));

                        int offset1 = traits::find_first_of(msk1);
                        int offset2 = traits::find_first_of(msk2);

                        if (offset1 != -1 &&
                            (offset2 == -1 || offset1 < offset2))
                        {
                            return true;
                        }
                        if (offset2 != -1 &&
                            (offset1 == -1 || offset2 < offset1))
                        {
                            return false;
                        }

                        std::advance(first1, num_lanes);
                        std::advance(first2, num_lanes);
                        count -= num_lanes;
                    }

                    while (first1 != last1 && first2 != last2)
                    {
                        if (HPX_INVOKE(pred, HPX_INVOKE(proj1, *first1),
                                HPX_INVOKE(proj2, *first2)))
                        {
                            return true;
                        }
                        if (HPX_INVOKE(pred, HPX_INVOKE(proj2, *first2),
                                HPX_INVOKE(proj1, *first1)))
                        {
                            return false;
                        }
                        ++first1;
                        ++first2;
                    }
                    return first1 == last1 && first2 != last2;
                }
            }
        }

        template <typename ZipIterator, typename Token, typename Pred,
            typename Proj1, typename Proj2>
        static void call(std::size_t base_idx, ZipIterator it,
            std::size_t part_count, Token& tok, Pred&& pred, Proj1&& proj1,
            Proj2&& proj2)
        {
            using policy_t = std::decay_t<ExPolicy>;
            constexpr std::size_t lanes =
                hpx::execution::policy_traits<policy_t>::num_lanes;
            using Iter1 = typename hpx::tuple_element<0,
                typename ZipIterator::iterator_tuple_type>::type;
            using Iter2 = typename hpx::tuple_element<1,
                typename ZipIterator::iterator_tuple_type>::type;
            using value_type1 =
                typename std::iterator_traits<Iter1>::value_type;
            using value_type2 =
                typename std::iterator_traits<Iter2>::value_type;

            constexpr bool can_vectorize_values =
                is_vectorizable_value_type_v<value_type1> &&
                is_vectorizable_value_type_v<value_type2>;

            if constexpr (!can_vectorize_values)
            {
                using base_policy_type =
                    decltype(hpx::execution::experimental::to_non_simd(
                        std::declval<ExPolicy>()));
                return sequential_lexicographical_compare<base_policy_type>(
                    base_idx, it, part_count, tok, HPX_FORWARD(Pred, pred),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
            }
            else
            {
                using vector_type1 =
                    traits::vector_pack_type_t<value_type1, lanes>;
                using vector_type2 =
                    traits::vector_pack_type_t<value_type2, lanes>;

                constexpr std::size_t num_lanes1 =
                    traits::vector_pack_size_v<vector_type1>;
                constexpr std::size_t num_lanes2 =
                    traits::vector_pack_size_v<vector_type2>;

                constexpr bool can_vectorize = (num_lanes1 > 1) &&
                    (num_lanes1 == num_lanes2) &&
                    is_lexicographical_compare_datapar_compatible_v<Pred, Proj1,
                        Proj2, vector_type1, vector_type2>;

                if constexpr (!can_vectorize)
                {
                    using base_policy_type =
                        decltype(hpx::execution::experimental::to_non_simd(
                            std::declval<ExPolicy>()));
                    return sequential_lexicographical_compare<base_policy_type>(
                        base_idx, it, part_count, tok, HPX_FORWARD(Pred, pred),
                        HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
                }
                else
                {
                    if (tok.was_cancelled(base_idx))
                    {
                        return;
                    }

                    auto first1 = hpx::get<0>(it.get_iterator_tuple());
                    auto first2 = hpx::get<1>(it.get_iterator_tuple());
                    constexpr std::size_t num_lanes = num_lanes1;

                    while (part_count >= num_lanes)
                    {
                        if (tok.was_cancelled(base_idx))
                        {
                            return;
                        }

                        auto v1 = traits::vector_pack_load<vector_type1,
                            value_type1>::unaligned(first1);
                        auto v2 = traits::vector_pack_load<vector_type2,
                            value_type2>::unaligned(first2);

                        auto msk1 = HPX_INVOKE(
                            pred, HPX_INVOKE(proj1, v1), HPX_INVOKE(proj2, v2));
                        auto msk2 = HPX_INVOKE(
                            pred, HPX_INVOKE(proj2, v2), HPX_INVOKE(proj1, v1));

                        int offset1 = traits::find_first_of(msk1);
                        int offset2 = traits::find_first_of(msk2);

                        int offset = -1;
                        if (offset1 != -1 && offset2 != -1)
                        {
                            offset = (std::min) (offset1, offset2);
                        }
                        else if (offset1 != -1)
                        {
                            offset = offset1;
                        }
                        else if (offset2 != -1)
                        {
                            offset = offset2;
                        }

                        if (offset != -1)
                        {
                            tok.cancel(base_idx + offset);
                            return;
                        }

                        std::advance(first1, num_lanes);
                        std::advance(first2, num_lanes);
                        base_idx += num_lanes;
                        part_count -= num_lanes;
                    }

                    while (part_count != 0)
                    {
                        if (tok.was_cancelled(base_idx))
                        {
                            return;
                        }

                        if (HPX_INVOKE(pred, HPX_INVOKE(proj1, *first1),
                                HPX_INVOKE(proj2, *first2)) ||
                            HPX_INVOKE(pred, HPX_INVOKE(proj2, *first2),
                                HPX_INVOKE(proj1, *first1)))
                        {
                            tok.cancel(base_idx);
                            return;
                        }

                        ++first1;
                        ++first2;
                        ++base_idx;
                        --part_count;
                    }
                }
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename Sent1, typename InIter2, typename Sent2, typename Pred,
        typename Proj1, typename Proj2>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr bool hpx_invoke(sequential_lexicographical_compare_t<ExPolicy>,
        InIter1 first1, Sent1 last1, InIter2 first2, Sent2 last2, Pred&& pred,
        Proj1&& proj1, Proj2&& proj2)
    {
        constexpr bool is_contiguous =
            (std::contiguous_iterator<std::decay_t<InIter1>> ||
                hpx::traits::is_contiguous_iterator_v<std::decay_t<InIter1>>) &&
            (std::contiguous_iterator<std::decay_t<InIter2>> ||
                hpx::traits::is_contiguous_iterator_v<std::decay_t<InIter2>>);

        constexpr bool is_datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<
                InIter1> &&
            hpx::parallel::util::detail::iterator_datapar_compatible_v<InIter2>;

        if constexpr (is_contiguous && is_datapar_compatible)
        {
            return datapar_lexicographical_compare<ExPolicy>::call(first1,
                last1, first2, last2, HPX_FORWARD(Pred, pred),
                HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_lexicographical_compare<base_policy_type>(first1,
                last1, first2, last2, HPX_FORWARD(Pred, pred),
                HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename ZipIterator,
        typename Token, typename Pred, typename Proj1, typename Proj2>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr void hpx_invoke(sequential_lexicographical_compare_t<ExPolicy>,
        std::size_t base_idx, ZipIterator it, std::size_t part_count,
        Token& tok, Pred&& pred, Proj1&& proj1, Proj2&& proj2)
    {
        using Iter1 = typename hpx::tuple_element<0,
            typename ZipIterator::iterator_tuple_type>::type;
        using Iter2 = typename hpx::tuple_element<1,
            typename ZipIterator::iterator_tuple_type>::type;

        constexpr bool is_contiguous =
            (std::contiguous_iterator<std::decay_t<Iter1>> ||
                hpx::traits::is_contiguous_iterator_v<std::decay_t<Iter1>>) &&
            (std::contiguous_iterator<std::decay_t<Iter2>> ||
                hpx::traits::is_contiguous_iterator_v<std::decay_t<Iter2>>);

        constexpr bool is_datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<Iter1> &&
            hpx::parallel::util::detail::iterator_datapar_compatible_v<Iter2>;

        if constexpr (is_contiguous && is_datapar_compatible)
        {
            return datapar_lexicographical_compare<ExPolicy>::call(base_idx, it,
                part_count, tok, HPX_FORWARD(Pred, pred),
                HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_lexicographical_compare<base_policy_type>(
                base_idx, it, part_count, tok, HPX_FORWARD(Pred, pred),
                HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
        }
    }

}    // namespace hpx::parallel::detail

#endif
