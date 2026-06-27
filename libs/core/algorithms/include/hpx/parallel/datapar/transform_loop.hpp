//  Copyright (c) 2007-2023 Hartmut Kaiser
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
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/util/transform_loop.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::util {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct datapar_transform_loop_n
        {
            using iterator_type = std::decay_t<Iterator>;

            using V = traits::vector_pack_type_t<
                typename std::iterator_traits<iterator_type>::value_type>;

            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter first, std::size_t count, OutIter dest, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterators_datapar_compatible_v<InIter, OutIter> &&
                    iterator_datapar_compatible_v<InIter> &&
                    iterator_datapar_compatible_v<OutIter>;

                if constexpr (datapar_compatible)
                {
                    std::size_t len = count;

                    for (/* */;
                        !(is_data_aligned(first) && is_data_aligned(dest)) &&
                        len != 0;
                        --len)
                    {
                        datapar_transform_loop_step::call1(f, first, dest);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    for (std::int64_t len_v =
                             static_cast<std::int64_t>(len - size + 1);
                        len_v > 0;
                        len_v -= static_cast<std::int64_t>(size), len -= size)
                    {
                        datapar_transform_loop_step::callv(f, first, dest);
                    }

                    for (/* */; len != 0; --len)
                    {
                        datapar_transform_loop_step::call1(f, first, dest);
                    }

                    return std::make_pair(HPX_MOVE(first), HPX_MOVE(dest));
                }
                else
                {
                    return util::transform_loop_n<
                        hpx::execution::sequenced_policy>(
                        first, count, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_vectorpack_execution_policy_v<ExPolicy>,
        std::pair<Iter, OutIter>>
    tag_invoke(hpx::parallel::util::transform_loop_n_t<ExPolicy>, Iter it,
        std::size_t count, OutIter dest, F&& f)
    {
        return detail::datapar_transform_loop_n<Iter>::call(
            it, count, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct datapar_transform_loop_n_ind
        {
            using iterator_type = std::decay_t<Iterator>;

            using V = typename traits::vector_pack_type<
                typename std::iterator_traits<iterator_type>::value_type>::type;

            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter first, std::size_t count, OutIter dest, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterators_datapar_compatible_v<InIter, OutIter> &&
                    iterator_datapar_compatible_v<InIter> &&
                    iterator_datapar_compatible_v<OutIter>;

                if constexpr (datapar_compatible)
                {
                    std::size_t len = count;

                    for (/* */;
                        !(is_data_aligned(first) && is_data_aligned(dest)) &&
                        len != 0;
                        --len)
                    {
                        datapar_transform_loop_step_ind::call1(f, first, dest);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    for (std::int64_t len_v =
                             static_cast<std::int64_t>(len - size + 1);
                        len_v > 0;
                        len_v -= static_cast<std::int64_t>(size), len -= size)
                    {
                        datapar_transform_loop_step_ind::callv(f, first, dest);
                    }

                    for (/* */; len != 0; --len)
                    {
                        datapar_transform_loop_step_ind::call1(f, first, dest);
                    }

                    return std::make_pair(HPX_MOVE(first), HPX_MOVE(dest));
                }
                else
                {
                    return util::transform_loop_n_ind<
                        hpx::execution::sequenced_policy>(
                        first, count, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_vectorpack_execution_policy_v<ExPolicy>,
        std::pair<Iter, OutIter>>
    tag_invoke(hpx::parallel::util::transform_loop_n_ind_t<ExPolicy>, Iter it,
        std::size_t count, OutIter dest, F&& f)
    {
        return detail::datapar_transform_loop_n_ind<Iter>::call(
            it, count, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct datapar_transform_loop
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type =
                typename std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type>;
            using V1 = traits::vector_pack_type_t<value_type, 1>;

            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter first, InIter last, OutIter dest, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterators_datapar_compatible_v<InIter, OutIter> &&
                    iterator_datapar_compatible_v<InIter> &&
                    iterator_datapar_compatible_v<OutIter>;

                if constexpr (datapar_compatible)
                {
                    return util::transform_loop_n<hpx::execution::simd_policy>(
                        first, hpx::parallel::detail::distance(first, last),
                        dest, HPX_FORWARD(F, f));
                }
                else
                {
                    return util::transform_loop(hpx::execution::seq, first,
                        last, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename IterB, typename IterE,
        typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        tag_invoke(hpx::parallel::util::transform_loop_t,
            hpx::execution::simd_policy, IterB it, IterE end, OutIter dest,
            F&& f)
    {
        auto ret = detail::datapar_transform_loop<IterB>::call(
            it, end, dest, HPX_FORWARD(F, f));

        return util::in_out_result<IterB, OutIter>{
            HPX_MOVE(ret.first), HPX_MOVE(ret.second)};
    }

    HPX_CXX_CORE_EXPORT template <typename IterB, typename IterE,
        typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        tag_invoke(hpx::parallel::util::transform_loop_t,
            hpx::execution::simd_task_policy, IterB it, IterE end, OutIter dest,
            F&& f)
    {
        auto ret = detail::datapar_transform_loop<IterB>::call(
            it, end, dest, HPX_FORWARD(F, f));

        return util::in_out_result<IterB, OutIter>{
            HPX_MOVE(ret.first), HPX_MOVE(ret.second)};
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct datapar_transform_loop_ind
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type =
                typename std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type>;
            using V1 = traits::vector_pack_type_t<value_type, 1>;

            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter first, InIter last, OutIter dest, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterators_datapar_compatible_v<InIter, OutIter> &&
                    iterator_datapar_compatible_v<InIter> &&
                    iterator_datapar_compatible_v<OutIter>;

                if constexpr (datapar_compatible)
                {
                    return util::transform_loop_n_ind<
                        hpx::execution::simd_policy>(first,
                        hpx::parallel::detail::distance(first, last), dest,
                        HPX_FORWARD(F, f));
                }
                else
                {
                    auto ret = util::transform_loop_ind(hpx::execution::seq,
                        first, last, dest, HPX_FORWARD(F, f));
                    return std::pair<InIter, OutIter>{
                        HPX_MOVE(ret.in), HPX_MOVE(ret.out)};
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename IterB, typename IterE,
        typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        tag_invoke(hpx::parallel::util::transform_loop_ind_t,
            hpx::execution::simd_policy, IterB it, IterE end, OutIter dest,
            F&& f)
    {
        auto ret = detail::datapar_transform_loop_ind<IterB>::call(
            it, end, dest, HPX_FORWARD(F, f));

        return util::in_out_result<IterB, OutIter>{
            HPX_MOVE(ret.first), HPX_MOVE(ret.second)};
    }

    HPX_CXX_CORE_EXPORT template <typename IterB, typename IterE,
        typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        tag_invoke(hpx::parallel::util::transform_loop_ind_t,
            hpx::execution::simd_task_policy, IterB it, IterE end, OutIter dest,
            F&& f)
    {
        auto ret = detail::datapar_transform_loop_ind<IterB>::call(
            it, end, dest, HPX_FORWARD(F, f));

        return util::in_out_result<IterB, OutIter>{
            HPX_MOVE(ret.first), HPX_MOVE(ret.second)};
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
        struct datapar_transform_binary_loop_n
        {
            using iterator1_type = std::decay_t<Iter1>;
            using value_type =
                typename std::iterator_traits<iterator1_type>::value_type;

            using V = traits::vector_pack_type_t<value_type>;

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr hpx::tuple<InIter1,
                InIter2, OutIter>
            call(InIter1 first1, std::size_t count, InIter2 first2,
                OutIter dest, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterators_datapar_compatible_v<InIter1, OutIter> &&
                    iterators_datapar_compatible_v<InIter2, OutIter> &&
                    iterator_datapar_compatible_v<InIter1> &&
                    iterator_datapar_compatible_v<InIter2> &&
                    iterator_datapar_compatible_v<OutIter>;

                if constexpr (datapar_compatible)
                {
                    std::size_t len = count;

                    for (/* */;
                        !(is_data_aligned(first1) && is_data_aligned(first2) &&
                            is_data_aligned(dest)) &&
                        len != 0;
                        --len)
                    {
                        datapar_transform_loop_step::call1(
                            f, first1, first2, dest);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    for (auto len_v = static_cast<std::int64_t>(len - size + 1);
                        len_v > 0;
                        len_v -= static_cast<std::int64_t>(size), len -= size)
                    {
                        datapar_transform_loop_step::callv(
                            f, first1, first2, dest);
                    }

                    for (/* */; len != 0; --len)
                    {
                        datapar_transform_loop_step::call1(
                            f, first1, first2, dest);
                    }

                    return hpx::make_tuple(
                        HPX_MOVE(first1), HPX_MOVE(first2), HPX_MOVE(dest));
                }
                else
                {
                    return util::transform_binary_loop_n<
                        hpx::execution::sequenced_policy>(
                        first1, count, first2, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_vectorpack_execution_policy_v<ExPolicy>,
        hpx::tuple<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::parallel::util::transform_binary_loop_n_t<ExPolicy>,
        InIter1 first1, std::size_t count, InIter2 first2, OutIter dest, F&& f)
    {
        return detail::datapar_transform_binary_loop_n<InIter1, InIter2>::call(
            first1, count, first2, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
        struct datapar_transform_binary_loop
        {
            using iterator1_type = std::decay_t<Iter1>;
            using iterator2_type = std::decay_t<Iter2>;

            using value1_type =
                typename std::iterator_traits<iterator1_type>::value_type;
            using value2_type =
                typename std::iterator_traits<iterator2_type>::value_type;

            using V11 = traits::vector_pack_type_t<value1_type, 1>;
            using V12 = traits::vector_pack_type_t<value2_type, 1>;

            using V1 = traits::vector_pack_type_t<value1_type>;
            using V2 = traits::vector_pack_type_t<value2_type>;

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr util::in_in_out_result<InIter1,
                    InIter2, OutIter>
                call(InIter1 first1, InIter1 last1, InIter2 first2,
                    OutIter dest, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterators_datapar_compatible_v<InIter1, OutIter> &&
                    iterators_datapar_compatible_v<InIter2, OutIter> &&
                    iterator_datapar_compatible_v<InIter1> &&
                    iterator_datapar_compatible_v<InIter2> &&
                    iterator_datapar_compatible_v<OutIter>;

                if constexpr (datapar_compatible)
                {
                    auto ret = util::transform_binary_loop_n<
                        hpx::execution::par_simd_policy>(first1,
                        hpx::parallel::detail::distance(first1, last1), first2,
                        dest, HPX_FORWARD(F, f));

                    return util::in_in_out_result<InIter1, InIter2, OutIter>{
                        hpx::get<0>(ret), hpx::get<1>(ret), hpx::get<2>(ret)};
                }
                else
                {
                    return util::transform_binary_loop<
                        hpx::execution::sequenced_policy>(
                        first1, last1, first2, dest, HPX_FORWARD(F, f));
                }
            }

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr util::in_in_out_result<InIter1,
                    InIter2, OutIter>
                call(InIter1 first1, InIter1 last1, InIter2 first2,
                    InIter2 last2, OutIter dest, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterators_datapar_compatible_v<InIter1, OutIter> &&
                    iterators_datapar_compatible_v<InIter2, OutIter> &&
                    iterator_datapar_compatible_v<InIter1> &&
                    iterator_datapar_compatible_v<InIter2> &&
                    iterator_datapar_compatible_v<OutIter>;

                if constexpr (datapar_compatible)
                {
                    // different versions of clang-format do different things
                    // clang-format off
                    std::size_t count = (std::min)(
                        hpx::parallel::detail::distance(first1, last1),
                        hpx::parallel::detail::distance(first2, last2));
                    // clang-format on

                    auto ret = util::transform_binary_loop_n<
                        hpx::execution::par_simd_policy>(
                        first1, count, first2, dest, HPX_FORWARD(F, f));

                    return util::in_in_out_result<InIter1, InIter2, OutIter>{
                        hpx::get<0>(ret), hpx::get<1>(ret), hpx::get<2>(ret)};
                }
                else
                {
                    return util::transform_binary_loop<
                        hpx::execution::sequenced_policy>(
                        first1, last1, first2, last2, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_vectorpack_execution_policy_v<ExPolicy>,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::parallel::util::transform_binary_loop_t<ExPolicy>,
        InIter1 first1, InIter1 last1, InIter2 first2, OutIter dest, F&& f)
    {
        return detail::datapar_transform_binary_loop<InIter1, InIter2>::call(
            first1, last1, first2, dest, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_vectorpack_execution_policy_v<ExPolicy>,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::parallel::util::transform_binary_loop_t<ExPolicy>,
        InIter1 first1, InIter1 last1, InIter2 first2, InIter2 last2,
        OutIter dest, F&& f)
    {
        return detail::datapar_transform_binary_loop<InIter1, InIter2>::call(
            first1, last1, first2, last2, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
        struct datapar_transform_binary_loop_ind_n
        {
            using iterator1_type = std::decay_t<Iter1>;
            using value_type =
                typename std::iterator_traits<iterator1_type>::value_type;

            using V = traits::vector_pack_type_t<value_type>;

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr hpx::tuple<InIter1,
                InIter2, OutIter>
            call(InIter1 first1, std::size_t count, InIter2 first2,
                OutIter dest, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterators_datapar_compatible_v<InIter1, OutIter> &&
                    iterators_datapar_compatible_v<InIter2, OutIter> &&
                    iterator_datapar_compatible_v<InIter1> &&
                    iterator_datapar_compatible_v<InIter2> &&
                    iterator_datapar_compatible_v<OutIter>;

                if constexpr (datapar_compatible)
                {
                    std::size_t len = count;

                    for (/* */;
                        !(is_data_aligned(first1) && is_data_aligned(first2) &&
                            is_data_aligned(dest)) &&
                        len != 0;
                        --len)
                    {
                        datapar_transform_loop_step_ind::call1(
                            f, first1, first2, dest);
                    }

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;

                    for (auto len_v = static_cast<std::int64_t>(len - size + 1);
                        len_v > 0;
                        len_v -= static_cast<std::int64_t>(size), len -= size)
                    {
                        datapar_transform_loop_step_ind::callv(
                            f, first1, first2, dest);
                    }

                    for (/* */; len != 0; --len)
                    {
                        datapar_transform_loop_step_ind::call1(
                            f, first1, first2, dest);
                    }

                    return hpx::make_tuple(
                        HPX_MOVE(first1), HPX_MOVE(first2), HPX_MOVE(dest));
                }
                else
                {
                    return util::transform_binary_loop_ind_n<
                        hpx::execution::sequenced_policy>(
                        first1, count, first2, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_vectorpack_execution_policy_v<ExPolicy>,
        hpx::tuple<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::parallel::util::transform_binary_loop_ind_n_t<ExPolicy>,
        InIter1 first1, std::size_t count, InIter2 first2, OutIter dest, F&& f)
    {
        return detail::datapar_transform_binary_loop_ind_n<InIter1,
            InIter2>::call(first1, count, first2, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
        struct datapar_transform_binary_loop_ind
        {
            using iterator1_type = std::decay_t<Iter1>;
            using iterator2_type = std::decay_t<Iter2>;

            using value1_type =
                typename std::iterator_traits<iterator1_type>::value_type;
            using value2_type =
                typename std::iterator_traits<iterator2_type>::value_type;

            using V11 = traits::vector_pack_type_t<value1_type, 1>;
            using V12 = traits::vector_pack_type_t<value2_type, 1>;

            using V1 = traits::vector_pack_type_t<value1_type>;
            using V2 = traits::vector_pack_type_t<value2_type>;

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
                iterators_datapar_compatible_v<InIter1, OutIter> &&
                    iterators_datapar_compatible_v<InIter2, OutIter> &&
                    iterator_datapar_compatible_v<InIter1> &&
                    iterator_datapar_compatible_v<InIter2> &&
                    iterator_datapar_compatible_v<OutIter>,
                util::in_in_out_result<InIter1, InIter2, OutIter>>
            call(InIter1 first1, InIter1 last1, InIter2 first2, OutIter dest,
                F&& f)
            {
                auto ret = util::transform_binary_loop_ind_n<
                    hpx::execution::par_simd_policy>(first1,
                    hpx::parallel::detail::distance(first1, last1), first2,
                    dest, HPX_FORWARD(F, f));

                return util::in_in_out_result<InIter1, InIter2, OutIter>{
                    hpx::get<0>(ret), hpx::get<1>(ret), hpx::get<2>(ret)};
            }

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
                !iterators_datapar_compatible_v<InIter1, OutIter> ||
                    !iterators_datapar_compatible_v<InIter2, OutIter> ||
                    !iterator_datapar_compatible_v<InIter1> ||
                    !iterator_datapar_compatible_v<InIter2> ||
                    !iterator_datapar_compatible_v<OutIter>,
                util::in_in_out_result<InIter1, InIter2, OutIter>>
            call(InIter1 first1, InIter1 last1, InIter2 first2, OutIter dest,
                F&& f)
            {
                return util::transform_binary_loop_ind<
                    hpx::execution::sequenced_policy>(
                    first1, last1, first2, dest, HPX_FORWARD(F, f));
            }

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
                iterators_datapar_compatible_v<InIter1, OutIter> &&
                    iterators_datapar_compatible_v<InIter2, OutIter> &&
                    iterator_datapar_compatible_v<InIter1> &&
                    iterator_datapar_compatible_v<InIter2> &&
                    iterator_datapar_compatible_v<OutIter>,
                util::in_in_out_result<InIter1, InIter2, OutIter>>
            call(InIter1 first1, InIter1 last1, InIter2 first2, InIter2 last2,
                OutIter dest, F&& f)
            {
                std::size_t count =
                    (std::min) (hpx::parallel::detail::distance(first1, last1),
                        hpx::parallel::detail::distance(first2, last2));

                auto ret = util::transform_binary_loop_ind_n<
                    hpx::execution::par_simd_policy>(
                    first1, count, first2, dest, HPX_FORWARD(F, f));

                return util::in_in_out_result<InIter1, InIter2, OutIter>{
                    hpx::get<0>(ret), hpx::get<1>(ret), hpx::get<2>(ret)};
            }

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
                !iterators_datapar_compatible_v<InIter1, OutIter> ||
                    !iterators_datapar_compatible_v<InIter2, OutIter> ||
                    !iterator_datapar_compatible_v<InIter1> ||
                    !iterator_datapar_compatible_v<InIter2> ||
                    !iterator_datapar_compatible_v<OutIter>,
                util::in_in_out_result<InIter1, InIter2, OutIter>>
            call(InIter1 first1, InIter1 last1, InIter2 first2, InIter2 last2,
                OutIter dest, F&& f)
            {
                return util::transform_binary_loop_ind<
                    hpx::execution::sequenced_policy>(
                    first1, last1, first2, last2, dest, HPX_FORWARD(F, f));
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_vectorpack_execution_policy_v<ExPolicy>,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::parallel::util::transform_binary_loop_ind_t<ExPolicy>,
        InIter1 first1, InIter1 last1, InIter2 first2, OutIter dest, F&& f)
    {
        return detail::datapar_transform_binary_loop_ind<InIter1,
            InIter2>::call(first1, last1, first2, dest, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_vectorpack_execution_policy_v<ExPolicy>,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::parallel::util::transform_binary_loop_ind_t<ExPolicy>,
        InIter1 first1, InIter1 last1, InIter2 first2, InIter2 last2,
        OutIter dest, F&& f)
    {
        return detail::datapar_transform_binary_loop_ind<InIter1,
            InIter2>::call(first1, last1, first2, last2, dest,
            HPX_FORWARD(F, f));
    }
}    // namespace hpx::parallel::util

#endif
