//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/unique.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/datapar/loop.hpp>
#include <hpx/parallel/datapar/zip_iterator.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    template <typename Pred, typename Proj, typename Vector>
    inline constexpr bool is_unique_datapar_compatible_v =
        requires(Pred&& pred, Proj&& proj, Vector&& v) {
            requires !std::is_same_v<
                std::decay_t<decltype(HPX_INVOKE(
                    pred, HPX_INVOKE(proj, v), HPX_INVOKE(proj, v)))>,
                bool>;
            traits::all_of(
                HPX_INVOKE(pred, HPX_INVOKE(proj, v), HPX_INVOKE(proj, v)));
        };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_unique
    {
        template <typename FwdIter, typename Sent, typename Pred, typename Proj>
        static FwdIter call(FwdIter first, Sent last, Pred&& pred, Proj&& proj)
        {
            if (first == last)
                return first;

            using policy_t = std::decay_t<ExPolicy>;
            constexpr std::size_t lanes =
                hpx::execution::policy_traits<policy_t>::num_lanes;
            using value_type =
                typename std::iterator_traits<FwdIter>::value_type;
            using vector_type = traits::vector_pack_type_t<value_type, lanes>;

            constexpr std::size_t num_lanes =
                traits::vector_pack_size_v<vector_type>;

            constexpr bool can_vectorize = (num_lanes > 1) &&
                is_unique_datapar_compatible_v<Pred, Proj, vector_type>;

            if constexpr (!can_vectorize)
            {
                using base_policy_type =
                    decltype(hpx::execution::experimental::to_non_simd(
                        std::declval<ExPolicy>()));
                return sequential_unique<base_policy_type>(first, last,
                    HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
            }
            else
            {
                auto count = detail::distance(first, last);
                if (count < 2)
                {
                    std::advance(first, count);
                    return first;
                }

                FwdIter dest = first;
                FwdIter prev = first;
                ++first;
                --count;

                while (count >= num_lanes)
                {
                    auto v_prev = traits::vector_pack_load<vector_type,
                        value_type>::unaligned(prev);
                    auto v_curr = traits::vector_pack_load<vector_type,
                        value_type>::unaligned(first);

                    auto msk = HPX_INVOKE(pred, HPX_INVOKE(proj, v_prev),
                        HPX_INVOKE(proj, v_curr));

                    if (!traits::all_of(msk))
                    {
                        for (std::size_t i = 0; i != num_lanes; ++i)
                        {
                            if (!msk[i])
                            {
                                FwdIter it = std::next(first, i);
                                if (++dest != it)
                                {
                                    *dest = std::ranges::iter_move(it);
                                }
                            }
                        }
                    }

                    std::advance(first, num_lanes);
                    std::advance(prev, num_lanes);
                    count -= num_lanes;
                }

                while (first != last)
                {
                    if (!HPX_INVOKE(pred, HPX_INVOKE(proj, *prev),
                            HPX_INVOKE(proj, *first)))
                    {
                        if (++dest != first)
                        {
                            *dest = std::ranges::iter_move(first);
                        }
                    }
                    prev = first++;
                }

                return ++dest;
            }
        }

        template <typename ZipIter, typename Pred, typename Proj>
        static void call(
            ZipIter part_begin, std::size_t part_size, Pred&& pred, Proj&& proj)
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            sequential_unique<base_policy_type>(part_begin, part_size,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_unique_copy
    {
        template <typename InIter, typename Sent, typename OutIter,
            typename Pred, typename Proj, typename IsForwardIter>
        static util::in_out_result<InIter, OutIter> call(InIter first,
            Sent last, OutIter dest, Pred&& pred, Proj&& proj, IsForwardIter)
        {
            if (first == last)
            {
                return util::in_out_result<InIter, OutIter>{
                    HPX_MOVE(first), HPX_MOVE(dest)};
            }

            using policy_t = std::decay_t<ExPolicy>;
            constexpr std::size_t lanes =
                hpx::execution::policy_traits<policy_t>::num_lanes;
            using value_type =
                typename std::iterator_traits<InIter>::value_type;
            using vector_type = traits::vector_pack_type_t<value_type, lanes>;

            constexpr std::size_t num_lanes =
                traits::vector_pack_size_v<vector_type>;

            constexpr bool can_vectorize = (num_lanes > 1) &&
                is_unique_datapar_compatible_v<Pred, Proj, vector_type>;

            if constexpr (!can_vectorize)
            {
                using base_policy_type =
                    decltype(hpx::execution::experimental::to_non_simd(
                        std::declval<ExPolicy>()));
                return sequential_unique_copy<base_policy_type>(first, last,
                    dest, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj),
                    IsForwardIter{});
            }
            else
            {
                *dest++ = *first;

                auto count = detail::distance(first, last) - 1;
                InIter prev = first;
                ++first;

                while (count >= num_lanes)
                {
                    auto v_prev = traits::vector_pack_load<vector_type,
                        value_type>::unaligned(prev);
                    auto v_curr = traits::vector_pack_load<vector_type,
                        value_type>::unaligned(first);

                    auto msk = HPX_INVOKE(pred, HPX_INVOKE(proj, v_prev),
                        HPX_INVOKE(proj, v_curr));

                    if (!traits::all_of(msk))
                    {
                        for (std::size_t i = 0; i != num_lanes; ++i)
                        {
                            if (!msk[i])
                            {
                                *dest++ = *std::next(first, i);
                            }
                        }
                    }

                    std::advance(first, num_lanes);
                    std::advance(prev, num_lanes);
                    count -= num_lanes;
                }

                while (first != last)
                {
                    if (!HPX_INVOKE(pred, HPX_INVOKE(proj, *prev),
                            HPX_INVOKE(proj, *first)))
                    {
                        *dest++ = *first;
                    }
                    prev = first++;
                }

                return util::in_out_result<InIter, OutIter>{
                    HPX_MOVE(first), HPX_MOVE(dest)};
            }
        }

        template <typename ZipIter, typename Pred, typename Proj>
        static std::size_t call(
            ZipIter part_begin, std::size_t part_size, Pred&& pred, Proj&& proj)
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_unique_copy<base_policy_type>(part_begin,
                part_size, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename Sent, typename Pred, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr InIter hpx_invoke(sequential_unique_t<ExPolicy>, InIter first,
        Sent last, Pred&& pred, Proj&& proj)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          InIter>::value)
        {
            return datapar_unique<ExPolicy>::call(
                first, last, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_unique<base_policy_type>(
                first, last, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename ZipIter,
        typename Pred, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr void hpx_invoke(sequential_unique_t<ExPolicy>, ZipIter part_begin,
        std::size_t part_size, Pred&& pred, Proj&& proj)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          ZipIter>::value)
        {
            datapar_unique<ExPolicy>::call(part_begin, part_size,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            sequential_unique<base_policy_type>(part_begin, part_size,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename Sent, typename OutIter, typename Pred, typename Proj,
        typename IsForwardIter>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr util::in_out_result<InIter, OutIter> hpx_invoke(
        sequential_unique_copy_t<ExPolicy>, InIter first, Sent last,
        OutIter dest, Pred&& pred, Proj&& proj, IsForwardIter is_fwd)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          InIter>::value)
        {
            return datapar_unique_copy<ExPolicy>::call(first, last, dest,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj), is_fwd);
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_unique_copy<base_policy_type>(first, last, dest,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj), is_fwd);
        }
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename ZipIter,
        typename Pred, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr std::size_t hpx_invoke(sequential_unique_copy_t<ExPolicy>,
        ZipIter part_begin, std::size_t part_size, Pred&& pred, Proj&& proj)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          ZipIter>::value)
        {
            return datapar_unique_copy<ExPolicy>::call(part_begin, part_size,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_unique_copy<base_policy_type>(part_begin,
                part_size, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
    }

}    // namespace hpx::parallel::detail

#endif
