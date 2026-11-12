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
#include <hpx/parallel/algorithms/detail/copy_if.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/datapar/loop.hpp>
#include <hpx/parallel/datapar/zip_iterator.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename Pred, typename Proj, typename Vector>
    inline constexpr bool is_copy_if_datapar_compatible_v = requires(
        std::decay_t<Pred>& pred, std::decay_t<Proj>& proj, Vector& v) {
        requires(!std::is_convertible_v<
            std::decay_t<decltype(HPX_INVOKE(pred, HPX_INVOKE(proj, v)))>,
            bool>);
        traits::any_of(HPX_INVOKE(pred, HPX_INVOKE(proj, v)));
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_copy_if
    {
        template <typename Mask>
        static HPX_HOST_DEVICE HPX_FORCEINLINE bool mask_lane_is_true(
            Mask const& msk, std::size_t i) noexcept
        {
            if constexpr (requires(
                              Mask const& m, std::size_t idx) { m.get(idx); })
            {
                return static_cast<bool>(msk.get(i));
            }
            else if constexpr (requires(
                                   Mask const& m, std::size_t idx) { m[idx]; })
            {
                return static_cast<bool>(msk[i]);
            }
            else
            {
                static_assert(sizeof(Mask) == 0,
                    "mask_lane_is_true: unexpected mask type with no lane "
                    "access");
                return false;
            }
        }

        template <typename InIter1, typename InIter2, typename OutIter,
            typename Pred, typename Proj>
        static util::in_out_result<InIter1, OutIter> call(
            InIter1 first, InIter2 last, OutIter dest, Pred&& pred, Proj&& proj)
        {
            if (first == last)
            {
                return util::in_out_result<InIter1, OutIter>{
                    HPX_MOVE(first), HPX_MOVE(dest)};
            }

            using policy_t = std::decay_t<ExPolicy>;
            constexpr std::size_t lanes =
                hpx::execution::policy_traits<policy_t>::num_lanes;
            using value_type =
                typename std::iterator_traits<InIter1>::value_type;

            constexpr bool can_vectorize_values =
                is_vectorizable_value_type_v<value_type>;

            if constexpr (!can_vectorize_values)
            {
                using base_policy_type =
                    decltype(hpx::execution::experimental::to_non_simd(
                        std::declval<ExPolicy>()));
                return sequential_copy_if<base_policy_type>(first, last, dest,
                    HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
            }
            else
            {
                using vector_type =
                    traits::vector_pack_type_t<value_type, lanes>;
                constexpr std::size_t num_lanes =
                    traits::vector_pack_size_v<vector_type>;

                constexpr bool is_contiguous =
                    std::contiguous_iterator<std::decay_t<InIter1>> ||
                    hpx::traits::is_contiguous_iterator_v<
                        std::decay_t<InIter1>>;

                constexpr bool can_vectorize = (num_lanes > 1) &&
                    is_contiguous &&
                    is_copy_if_datapar_compatible_v<Pred, Proj, vector_type>;

                if constexpr (!can_vectorize)
                {
                    using base_policy_type =
                        decltype(hpx::execution::experimental::to_non_simd(
                            std::declval<ExPolicy>()));
                    return sequential_copy_if<base_policy_type>(first, last,
                        dest, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
                }
                else
                {
                    auto count = detail::distance(first, last);

                    while (count >= static_cast<decltype(count)>(num_lanes))
                    {
                        auto v = traits::vector_pack_load<vector_type,
                            value_type>::unaligned(first);

                        auto msk = HPX_INVOKE(pred, HPX_INVOKE(proj, v));

                        if (traits::all_of(msk))
                        {
                            for (std::size_t i = 0; i != num_lanes; ++i)
                            {
                                *dest++ = *std::next(first, i);
                            }
                        }
                        else if (traits::any_of(msk))
                        {
                            for (std::size_t i = 0; i != num_lanes; ++i)
                            {
                                if (mask_lane_is_true(msk, i))
                                {
                                    *dest++ = *std::next(first, i);
                                }
                            }
                        }

                        std::advance(first, num_lanes);
                        count -= num_lanes;
                    }

                    while (first != last)
                    {
                        if (HPX_INVOKE(pred, HPX_INVOKE(proj, *first)))
                        {
                            *dest++ = *first;
                        }
                        ++first;
                    }

                    return util::in_out_result<InIter1, OutIter>{
                        HPX_MOVE(first), HPX_MOVE(dest)};
                }
            }
        }

        template <typename ZipIter, typename Pred, typename Proj>
        static std::size_t call(
            ZipIter part_begin, std::size_t part_size, Pred&& pred, Proj&& proj)
        {
            if (part_size == 0)
            {
                return 0;
            }

            using policy_t = std::decay_t<ExPolicy>;
            constexpr std::size_t lanes =
                hpx::execution::policy_traits<policy_t>::num_lanes;

            using Iter1 = typename hpx::tuple_element<0,
                typename ZipIter::iterator_tuple_type>::type;
            using value_type = typename std::iterator_traits<Iter1>::value_type;

            constexpr bool can_vectorize_values =
                is_vectorizable_value_type_v<value_type>;

            if constexpr (!can_vectorize_values)
            {
                using base_policy_type =
                    decltype(hpx::execution::experimental::to_non_simd(
                        std::declval<ExPolicy>()));
                return sequential_copy_if<base_policy_type>(part_begin,
                    part_size, HPX_FORWARD(Pred, pred),
                    HPX_FORWARD(Proj, proj));
            }
            else
            {
                using vector_type =
                    traits::vector_pack_type_t<value_type, lanes>;
                constexpr std::size_t num_lanes =
                    traits::vector_pack_size_v<vector_type>;

                constexpr bool is_contiguous =
                    std::contiguous_iterator<std::decay_t<Iter1>> ||
                    hpx::traits::is_contiguous_iterator_v<std::decay_t<Iter1>>;

                constexpr bool can_vectorize = (num_lanes > 1) &&
                    is_contiguous &&
                    is_copy_if_datapar_compatible_v<Pred, Proj, vector_type>;

                if constexpr (!can_vectorize)
                {
                    using base_policy_type =
                        decltype(hpx::execution::experimental::to_non_simd(
                            std::declval<ExPolicy>()));
                    return sequential_copy_if<base_policy_type>(part_begin,
                        part_size, HPX_FORWARD(Pred, pred),
                        HPX_FORWARD(Proj, proj));
                }
                else
                {
                    auto first = hpx::get<0>(part_begin.get_iterator_tuple());
                    auto flags_ptr =
                        hpx::get<1>(part_begin.get_iterator_tuple());
                    std::size_t curr = 0;

                    while (part_size >= num_lanes)
                    {
                        auto v = traits::vector_pack_load<vector_type,
                            value_type>::unaligned(first);

                        auto msk = HPX_INVOKE(pred, HPX_INVOKE(proj, v));

                        if (traits::all_of(msk))
                        {
                            for (std::size_t i = 0; i != num_lanes; ++i)
                            {
                                *flags_ptr++ = true;
                            }
                            curr += num_lanes;
                        }
                        else if (!traits::any_of(msk))
                        {
                            for (std::size_t i = 0; i != num_lanes; ++i)
                            {
                                *flags_ptr++ = false;
                            }
                        }
                        else
                        {
                            for (std::size_t i = 0; i != num_lanes; ++i)
                            {
                                bool const f = mask_lane_is_true(msk, i);
                                *flags_ptr++ = f;
                                if (f)
                                {
                                    ++curr;
                                }
                            }
                        }

                        std::advance(first, num_lanes);
                        part_size -= num_lanes;
                    }

                    while (part_size != 0)
                    {
                        bool const f = static_cast<bool>(
                            hpx::invoke(pred, hpx::invoke(proj, *first)));
                        *flags_ptr++ = f;
                        if (f)
                        {
                            ++curr;
                        }
                        ++first;
                        --part_size;
                    }

                    return curr;
                }
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename Pred, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr util::in_out_result<InIter1, OutIter> hpx_invoke(
        sequential_copy_if_t<ExPolicy>, InIter1 first, InIter2 last,
        OutIter dest, Pred&& pred, Proj&& proj)
    {
        constexpr bool is_contiguous =
            std::contiguous_iterator<std::decay_t<InIter1>> ||
            hpx::traits::is_contiguous_iterator_v<std::decay_t<InIter1>>;

        constexpr bool is_datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<InIter1>;

        if constexpr (is_contiguous && is_datapar_compatible)
        {
            return datapar_copy_if<ExPolicy>::call(first, last, dest,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_copy_if<base_policy_type>(first, last, dest,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename ZipIter,
        typename Pred, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr std::size_t hpx_invoke(sequential_copy_if_t<ExPolicy>,
        ZipIter part_begin, std::size_t part_size, Pred&& pred, Proj&& proj)
    {
        using Iter1 = typename hpx::tuple_element<0,
            typename ZipIter::iterator_tuple_type>::type;

        constexpr bool is_contiguous =
            std::contiguous_iterator<std::decay_t<Iter1>> ||
            hpx::traits::is_contiguous_iterator_v<std::decay_t<Iter1>>;

        constexpr bool is_datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<Iter1>;

        if constexpr (is_contiguous && is_datapar_compatible)
        {
            return datapar_copy_if<ExPolicy>::call(part_begin, part_size,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_copy_if<base_policy_type>(part_begin, part_size,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
    }

}    // namespace hpx::parallel::detail

#endif
