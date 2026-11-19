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
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/exclusive_scan.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    template <typename Op, typename T, typename ValueType>
    inline constexpr bool is_exclusive_scan_datapar_compatible_v =
        std::is_invocable_v<Op&, T&, ValueType&>;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_exclusive_scan
    {
        template <typename InIter, typename Sent, typename OutIter, typename T,
            typename Op>
        static util::in_out_result<InIter, OutIter> call(
            InIter first, Sent last, OutIter dest, T init, Op&& op)
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

            constexpr bool can_vectorize_values =
                is_vectorizable_value_type_v<value_type>;

            if constexpr (!can_vectorize_values)
            {
                using base_policy_type =
                    decltype(hpx::execution::experimental::to_non_simd(
                        std::declval<ExPolicy>()));
                return sequential_exclusive_scan<base_policy_type>(
                    first, last, dest, init, HPX_FORWARD(Op, op));
            }
            else
            {
                using vector_type =
                    traits::vector_pack_type_t<value_type, lanes>;
                constexpr std::size_t num_lanes =
                    traits::vector_pack_size_v<vector_type>;

                constexpr bool can_vectorize = (num_lanes > 1) &&
                    is_exclusive_scan_datapar_compatible_v<Op, T, value_type>;

                if constexpr (!can_vectorize)
                {
                    using base_policy_type =
                        decltype(hpx::execution::experimental::to_non_simd(
                            std::declval<ExPolicy>()));
                    return sequential_exclusive_scan<base_policy_type>(
                        first, last, dest, init, HPX_FORWARD(Op, op));
                }
                else
                {
                    auto count = detail::distance(first, last);
                    T temp = init;

                    while (count >= static_cast<decltype(count)>(num_lanes))
                    {
                        auto v = traits::vector_pack_load<vector_type,
                            value_type>::unaligned(first);

                        for (std::size_t i = 0; i != num_lanes; ++i)
                        {
                            *dest++ = temp;
                            init = HPX_INVOKE(op, init, traits::get(v, i));
                            temp = init;
                        }

                        std::advance(first, num_lanes);
                        count -= num_lanes;
                    }

                    while (first != last)
                    {
                        *dest++ = temp;
                        init = HPX_INVOKE(op, init, *first);
                        temp = init;
                        ++first;
                    }

                    return util::in_out_result<InIter, OutIter>{
                        HPX_MOVE(first), HPX_MOVE(dest)};
                }
            }
        }

        template <typename InIter, typename OutIter, typename T, typename Op>
        static T call(
            InIter first, std::size_t count, OutIter dest, T init, Op&& op)
        {
            if (count == 0)
            {
                return init;
            }

            using policy_t = std::decay_t<ExPolicy>;
            constexpr std::size_t lanes =
                hpx::execution::policy_traits<policy_t>::num_lanes;
            using value_type =
                typename std::iterator_traits<InIter>::value_type;

            constexpr bool can_vectorize_values =
                is_vectorizable_value_type_v<value_type>;

            if constexpr (!can_vectorize_values)
            {
                using base_policy_type =
                    decltype(hpx::execution::experimental::to_non_simd(
                        std::declval<ExPolicy>()));
                return sequential_exclusive_scan_n<base_policy_type>(
                    first, count, dest, init, HPX_FORWARD(Op, op));
            }
            else
            {
                using vector_type =
                    traits::vector_pack_type_t<value_type, lanes>;
                constexpr std::size_t num_lanes =
                    traits::vector_pack_size_v<vector_type>;

                constexpr bool can_vectorize = (num_lanes > 1) &&
                    is_exclusive_scan_datapar_compatible_v<Op, T, value_type>;

                if constexpr (!can_vectorize)
                {
                    using base_policy_type =
                        decltype(hpx::execution::experimental::to_non_simd(
                            std::declval<ExPolicy>()));
                    return sequential_exclusive_scan_n<base_policy_type>(
                        first, count, dest, init, HPX_FORWARD(Op, op));
                }
                else
                {
                    T temp = init;

                    while (count >= num_lanes)
                    {
                        auto v = traits::vector_pack_load<vector_type,
                            value_type>::unaligned(first);

                        for (std::size_t i = 0; i != num_lanes; ++i)
                        {
                            *dest++ = temp;
                            init = HPX_INVOKE(op, init, traits::get(v, i));
                            temp = init;
                        }

                        std::advance(first, num_lanes);
                        count -= num_lanes;
                    }

                    while (count-- != 0)
                    {
                        *dest++ = temp;
                        init = HPX_INVOKE(op, init, *first);
                        temp = init;
                        ++first;
                    }

                    return init;
                }
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename Sent, typename OutIter, typename T, typename Op>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy> &&
            !std::integral<Sent>)
    constexpr util::in_out_result<InIter, OutIter> hpx_invoke(
        sequential_exclusive_scan_t<ExPolicy>, InIter first, Sent last,
        OutIter dest, T init, Op&& op)
    {
        constexpr bool is_contiguous =
            std::contiguous_iterator<std::decay_t<InIter>> ||
            hpx::traits::is_contiguous_iterator_v<std::decay_t<InIter>>;

        constexpr bool is_datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<InIter>;

        if constexpr (is_contiguous && is_datapar_compatible)
        {
            return datapar_exclusive_scan<ExPolicy>::call(
                first, last, dest, init, HPX_FORWARD(Op, op));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_exclusive_scan<base_policy_type>(
                first, last, dest, init, HPX_FORWARD(Op, op));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename OutIter, typename T, typename Op>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr T hpx_invoke(sequential_exclusive_scan_t<ExPolicy>, InIter first,
        std::size_t count, OutIter dest, T init, Op&& op)
    {
        constexpr bool is_contiguous =
            std::contiguous_iterator<std::decay_t<InIter>> ||
            hpx::traits::is_contiguous_iterator_v<std::decay_t<InIter>>;

        constexpr bool is_datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<InIter>;

        if constexpr (is_contiguous && is_datapar_compatible)
        {
            return datapar_exclusive_scan<ExPolicy>::call(
                first, count, dest, init, HPX_FORWARD(Op, op));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_exclusive_scan_n<base_policy_type>(
                first, count, dest, init, HPX_FORWARD(Op, op));
        }
    }

}    // namespace hpx::parallel::detail

#endif
