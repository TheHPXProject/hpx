//  Copyright (c) 2007-2026 Hartmut Kaiser
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
        HPX_CXX_CORE_EXPORT template <typename Iterator, std::size_t N>
        struct datapar_transform_loop_n
        {
            using iterator_type = std::decay_t<Iterator>;

            using V = traits::vector_pack_type_t<
                typename std::iterator_traits<iterator_type>::value_type, N>;

            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter first, std::size_t count, OutIter dest, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterators_datapar_compatible_v<InIter, OutIter> &&
                    iterator_datapar_compatible_v<InIter> &&
                    iterator_datapar_compatible_v<OutIter>;

                std::size_t len = count;
                if constexpr (datapar_compatible && N != 1)
                {
                    using out_value_t = std::remove_cv_t<
                        std::remove_reference_t<decltype(*dest)>>;
                    using VOut = traits::vector_pack_type_t<out_value_t, N>;

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    static_assert(size == traits::vector_pack_size_v<VOut>,
                        "input and output packs must have the same lane count");

                    for (/* */; len != 0 && !is_pack_aligned<V>(first); --len)
                    {
                        datapar_transform_loop_step<N>::call1(f, first, dest);
                    }

                    // one-time check: vectorize only if dest is also aligned
                    // now
                    if (is_pack_aligned<VOut>(dest))
                    {
                        for (/* */; len >= size; len -= size)
                        {
                            datapar_transform_loop_step<N>::callv(
                                f, first, dest);
                        }
                    }
                }

                for (/* */; len != 0; --len)
                {
                    datapar_transform_loop_step<N>::call1(f, first, dest);
                }

                return std::make_pair(HPX_MOVE(first), HPX_MOVE(dest));
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename OutIter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::pair<Iter, OutIter>
    hpx_invoke(hpx::parallel::util::transform_loop_n_t<ExPolicy>, Iter it,
        std::size_t count, OutIter dest, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_transform_loop_n<Iter, num_lanes>::call(
            it, count, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator, std::size_t N>
        struct datapar_transform_loop_n_ind
        {
            using iterator_type = std::decay_t<Iterator>;

            using V = traits::vector_pack_type_t<
                typename std::iterator_traits<iterator_type>::value_type, N>;

            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter first, std::size_t count, OutIter dest, F&& f)
            {
                constexpr bool datapar_compatible =
                    iterators_datapar_compatible_v<InIter, OutIter> &&
                    iterator_datapar_compatible_v<InIter> &&
                    iterator_datapar_compatible_v<OutIter>;

                std::size_t len = count;
                if constexpr (datapar_compatible && N != 1)
                {
                    using out_value_t = std::remove_cv_t<
                        std::remove_reference_t<decltype(*dest)>>;
                    using VOut = traits::vector_pack_type_t<out_value_t, N>;

                    constexpr std::size_t size = traits::vector_pack_size_v<V>;
                    static_assert(size == traits::vector_pack_size_v<VOut>,
                        "input and output packs must have the same lane count");

                    for (/* */; len != 0 && !is_pack_aligned<V>(first); --len)
                    {
                        datapar_transform_loop_step_ind<N>::call1(
                            f, first, dest);
                    }

                    // one-time check: vectorize only if dest is also aligned
                    // now
                    if (is_pack_aligned<VOut>(dest))
                    {
                        for (/* */; len >= size; len -= size)
                        {
                            datapar_transform_loop_step_ind<N>::callv(
                                f, first, dest);
                        }
                    }
                }

                for (/* */; len != 0; --len)
                {
                    datapar_transform_loop_step_ind<N>::call1(f, first, dest);
                }

                return std::make_pair(HPX_MOVE(first), HPX_MOVE(dest));
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename OutIter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::pair<Iter, OutIter>
    hpx_invoke(hpx::parallel::util::transform_loop_n_ind_t<ExPolicy>, Iter it,
        std::size_t count, OutIter dest, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_transform_loop_n_ind<Iter, num_lanes>::call(
            it, count, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator, std::size_t N>
        struct datapar_transform_loop
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type, N>;

            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter first, InIter last, OutIter dest, F&& f)
            {
                return util::transform_loop_n<
                    hpx::execution::fixed_size_simd_policy<N>>(first,
                    hpx::parallel::detail::distance(first, last), dest,
                    HPX_FORWARD(F, f));
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename IterB,
        typename IterE, typename OutIter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        hpx_invoke(hpx::parallel::util::transform_loop_t, ExPolicy&&, IterB it,
            IterE end, OutIter dest, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;

        auto ret = detail::datapar_transform_loop<IterB, num_lanes>::call(
            it, end, dest, HPX_FORWARD(F, f));

        return util::in_out_result<IterB, OutIter>{
            HPX_MOVE(ret.first), HPX_MOVE(ret.second)};
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iterator, std::size_t N>
        struct datapar_transform_loop_ind
        {
            using iterator_type = std::decay_t<Iterator>;
            using value_type = std::iterator_traits<iterator_type>::value_type;

            using V = traits::vector_pack_type_t<value_type, N>;

            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr std::pair<InIter, OutIter>
                call(InIter first, InIter last, OutIter dest, F&& f)
            {
                return util::transform_loop_n_ind<
                    hpx::execution::fixed_size_simd_policy<N>>(first,
                    hpx::parallel::detail::distance(first, last), dest,
                    HPX_FORWARD(F, f));
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename IterB,
        typename IterE, typename OutIter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        hpx_invoke(hpx::parallel::util::transform_loop_ind_t, ExPolicy&&,
            IterB it, IterE end, OutIter dest, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;

        auto ret = detail::datapar_transform_loop_ind<IterB, num_lanes>::call(
            it, end, dest, HPX_FORWARD(F, f));

        return util::in_out_result<IterB, OutIter>{
            HPX_MOVE(ret.first), HPX_MOVE(ret.second)};
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2,
            std::size_t N>
        struct datapar_transform_binary_loop_n
        {
            using iterator1_type = std::decay_t<Iter1>;
            using iterator2_type = std::decay_t<Iter2>;

            using value_type1 =
                std::iterator_traits<iterator1_type>::value_type;
            using value_type2 =
                std::iterator_traits<iterator2_type>::value_type;

            using V1 = traits::vector_pack_type_t<value_type1, N>;
            using V2 = traits::vector_pack_type_t<value_type2, N>;

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

                std::size_t len = count;
                if constexpr (datapar_compatible && N != 1)
                {
                    using out_value_t = std::remove_cv_t<
                        std::remove_reference_t<decltype(*dest)>>;
                    using VOut = traits::vector_pack_type_t<out_value_t, N>;

                    constexpr std::size_t size = traits::vector_pack_size_v<V1>;
                    static_assert(size == traits::vector_pack_size_v<V2>,
                        "input and output packs must have the same lane count");
                    static_assert(size == traits::vector_pack_size_v<VOut>,
                        "input and output packs must have the same lane count");

                    for (/* */; len != 0 && !is_pack_aligned<V1>(first1); --len)
                    {
                        datapar_transform_loop_step<N>::call1(
                            f, first1, first2, dest);
                    }

                    // one-time check: vectorize only if first2 and dest are
                    // also aligned now
                    if (len >= size && is_pack_aligned<V2>(first2) &&
                        is_pack_aligned<VOut>(dest))
                    {
                        for (/* */; len >= size; len -= size)
                        {
                            datapar_transform_loop_step<N>::callv(
                                f, first1, first2, dest);
                        }
                    }
                }

                for (/* */; len != 0; --len)
                {
                    datapar_transform_loop_step<N>::call1(
                        f, first1, first2, dest);
                }

                return hpx::make_tuple(
                    HPX_MOVE(first1), HPX_MOVE(first2), HPX_MOVE(dest));
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr hpx::tuple<InIter1, InIter2, OutIter>
        hpx_invoke(hpx::parallel::util::transform_binary_loop_n_t<ExPolicy>,
            InIter1 first1, std::size_t count, InIter2 first2, OutIter dest,
            F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_transform_binary_loop_n<InIter1, InIter2,
            num_lanes>::call(first1, count, first2, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2,
            std::size_t N>
        struct datapar_transform_binary_loop
        {
            using iterator1_type = std::decay_t<Iter1>;
            using iterator2_type = std::decay_t<Iter2>;

            using value1_type =
                std::iterator_traits<iterator1_type>::value_type;
            using value2_type =
                std::iterator_traits<iterator2_type>::value_type;

            using V1 = traits::vector_pack_type_t<value1_type, N>;
            using V2 = traits::vector_pack_type_t<value2_type, N>;

            static_assert(traits::vector_pack_size_v<V1> ==
                    traits::vector_pack_size_v<V2>,
                "the sizes of the vector-packs should be equal");

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr util::in_in_out_result<InIter1,
                    InIter2, OutIter>
                call(InIter1 first1, InIter1 last1, InIter2 first2,
                    OutIter dest, F&& f)
            {
                auto ret = util::transform_binary_loop_n<
                    hpx::execution::par_fixed_size_simd_policy<N>>(first1,
                    hpx::parallel::detail::distance(first1, last1), first2,
                    dest, HPX_FORWARD(F, f));

                return util::in_in_out_result<InIter1, InIter2, OutIter>{
                    hpx::get<0>(ret), hpx::get<1>(ret), hpx::get<2>(ret)};
            }

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr util::in_in_out_result<InIter1,
                    InIter2, OutIter>
                call(InIter1 first1, InIter1 last1, InIter2 first2,
                    InIter2 last2, OutIter dest, F&& f)
            {
                std::size_t count =
                    (std::min) (hpx::parallel::detail::distance(first1, last1),
                        hpx::parallel::detail::distance(first2, last2));

                auto ret = util::transform_binary_loop_n<
                    hpx::execution::par_fixed_size_simd_policy<N>>(
                    first1, count, first2, dest, HPX_FORWARD(F, f));

                return util::in_in_out_result<InIter1, InIter2, OutIter>{
                    hpx::get<0>(ret), hpx::get<1>(ret), hpx::get<2>(ret)};
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr util::in_in_out_result<InIter1,
        InIter2, OutIter>
    hpx_invoke(hpx::parallel::util::transform_binary_loop_t<ExPolicy>,
        InIter1 first1, InIter1 last1, InIter2 first2, OutIter dest, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_transform_binary_loop<InIter1, InIter2,
            num_lanes>::call(first1, last1, first2, dest, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr util::in_in_out_result<InIter1,
        InIter2, OutIter>
    hpx_invoke(hpx::parallel::util::transform_binary_loop_t<ExPolicy>,
        InIter1 first1, InIter1 last1, InIter2 first2, InIter2 last2,
        OutIter dest, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_transform_binary_loop<InIter1, InIter2,
            num_lanes>::call(first1, last1, first2, last2, dest,
            HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2,
            std::size_t N>
        struct datapar_transform_binary_loop_ind_n
        {
            using iterator1_type = std::decay_t<Iter1>;
            using iterator2_type = std::decay_t<Iter2>;

            using value1_type =
                std::iterator_traits<iterator1_type>::value_type;
            using value2_type =
                std::iterator_traits<iterator2_type>::value_type;

            using V1 = traits::vector_pack_type_t<value1_type, N>;
            using V2 = traits::vector_pack_type_t<value2_type, N>;

            static_assert(traits::vector_pack_size_v<V1> ==
                    traits::vector_pack_size_v<V2>,
                "the sizes of the vector-packs should be equal");

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

                std::size_t len = count;
                if constexpr (datapar_compatible && N != 1)
                {
                    using out_value_t = std::remove_cv_t<
                        std::remove_reference_t<decltype(*dest)>>;
                    using VOut = traits::vector_pack_type_t<out_value_t, N>;

                    constexpr std::size_t size = traits::vector_pack_size_v<V1>;
                    static_assert(size == traits::vector_pack_size_v<V2>,
                        "input and output packs must have the same lane count");
                    static_assert(size == traits::vector_pack_size_v<VOut>,
                        "input and output packs must have the same lane count");

                    for (/* */; len != 0 && !is_pack_aligned<V1>(first1); --len)
                    {
                        datapar_transform_loop_step_ind<N>::call1(
                            f, first1, first2, dest);
                    }

                    // one-time check: vectorize only if first2 and dest are
                    // also aligned now
                    if (len >= size && is_pack_aligned<V2>(first2) &&
                        is_pack_aligned<VOut>(dest))
                    {
                        for (/* */; len >= size; len -= size)
                        {
                            datapar_transform_loop_step_ind<N>::callv(
                                f, first1, first2, dest);
                        }
                    }
                }

                for (/* */; len != 0; --len)
                {
                    datapar_transform_loop_step_ind<N>::call1(
                        f, first1, first2, dest);
                }

                return hpx::make_tuple(
                    HPX_MOVE(first1), HPX_MOVE(first2), HPX_MOVE(dest));
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr hpx::tuple<InIter1, InIter2, OutIter>
        hpx_invoke(hpx::parallel::util::transform_binary_loop_ind_n_t<ExPolicy>,
            InIter1 first1, std::size_t count, InIter2 first2, OutIter dest,
            F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_transform_binary_loop_ind_n<InIter1, InIter2,
            num_lanes>::call(first1, count, first2, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2,
            std::size_t N>
        struct datapar_transform_binary_loop_ind
        {
            using iterator1_type = std::decay_t<Iter1>;
            using iterator2_type = std::decay_t<Iter2>;

            using value1_type =
                std::iterator_traits<iterator1_type>::value_type;
            using value2_type =
                std::iterator_traits<iterator2_type>::value_type;

            using V1 = traits::vector_pack_type_t<value1_type, N>;
            using V2 = traits::vector_pack_type_t<value2_type, N>;

            static_assert(traits::vector_pack_size_v<V1> ==
                    traits::vector_pack_size_v<V2>,
                "the sizes of the vector-packs should be equal");

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
                requires(iterators_datapar_compatible_v<InIter1, OutIter> &&
                    iterators_datapar_compatible_v<InIter2, OutIter> &&
                    iterator_datapar_compatible_v<InIter1> &&
                    iterator_datapar_compatible_v<InIter2> &&
                    iterator_datapar_compatible_v<OutIter>)
            HPX_HOST_DEVICE HPX_FORCEINLINE static util::in_in_out_result<
                InIter1, InIter2, OutIter>
            call(InIter1 first1, InIter1 last1, InIter2 first2, OutIter dest,
                F&& f)
            {
                auto ret = util::transform_binary_loop_ind_n<
                    hpx::execution::par_fixed_size_simd_policy<N>>(first1,
                    hpx::parallel::detail::distance(first1, last1), first2,
                    dest, HPX_FORWARD(F, f));

                return util::in_in_out_result<InIter1, InIter2, OutIter>{
                    hpx::get<0>(ret), hpx::get<1>(ret), hpx::get<2>(ret)};
            }

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
                requires(!iterators_datapar_compatible_v<InIter1, OutIter> ||
                    !iterators_datapar_compatible_v<InIter2, OutIter> ||
                    !iterator_datapar_compatible_v<InIter1> ||
                    !iterator_datapar_compatible_v<InIter2> ||
                    !iterator_datapar_compatible_v<OutIter>)
            HPX_HOST_DEVICE HPX_FORCEINLINE static util::in_in_out_result<
                InIter1, InIter2, OutIter>
            call(InIter1 first1, InIter1 last1, InIter2 first2, OutIter dest,
                F&& f)
            {
                return util::transform_binary_loop_ind<
                    hpx::execution::sequenced_policy>(
                    first1, last1, first2, dest, HPX_FORWARD(F, f));
            }

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
                requires(iterators_datapar_compatible_v<InIter1, OutIter> &&
                    iterators_datapar_compatible_v<InIter2, OutIter> &&
                    iterator_datapar_compatible_v<InIter1> &&
                    iterator_datapar_compatible_v<InIter2> &&
                    iterator_datapar_compatible_v<OutIter>)
            HPX_HOST_DEVICE HPX_FORCEINLINE static util::in_in_out_result<
                InIter1, InIter2, OutIter>
            call(InIter1 first1, InIter1 last1, InIter2 first2, InIter2 last2,
                OutIter dest, F&& f)
            {
                std::size_t count =
                    (std::min) (hpx::parallel::detail::distance(first1, last1),
                        hpx::parallel::detail::distance(first2, last2));

                auto ret = util::transform_binary_loop_ind_n<
                    hpx::execution::par_fixed_size_simd_policy<N>>(
                    first1, count, first2, dest, HPX_FORWARD(F, f));

                return util::in_in_out_result<InIter1, InIter2, OutIter>{
                    hpx::get<0>(ret), hpx::get<1>(ret), hpx::get<2>(ret)};
            }

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
                requires(!iterators_datapar_compatible_v<InIter1, OutIter> ||
                    !iterators_datapar_compatible_v<InIter2, OutIter> ||
                    !iterator_datapar_compatible_v<InIter1> ||
                    !iterator_datapar_compatible_v<InIter2> ||
                    !iterator_datapar_compatible_v<OutIter>)
            HPX_HOST_DEVICE HPX_FORCEINLINE static util::in_in_out_result<
                InIter1, InIter2, OutIter>
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
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr util::in_in_out_result<InIter1,
        InIter2, OutIter>
    hpx_invoke(hpx::parallel::util::transform_binary_loop_ind_t<ExPolicy>,
        InIter1 first1, InIter1 last1, InIter2 first2, OutIter dest, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_transform_binary_loop_ind<InIter1, InIter2,
            num_lanes>::call(first1, last1, first2, dest, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr util::in_in_out_result<InIter1,
        InIter2, OutIter>
    hpx_invoke(hpx::parallel::util::transform_binary_loop_ind_t<ExPolicy>,
        InIter1 first1, InIter1 last1, InIter2 first2, InIter2 last2,
        OutIter dest, F&& f)
    {
        constexpr std::size_t num_lanes =
            hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes;
        return detail::datapar_transform_binary_loop_ind<InIter1, InIter2,
            num_lanes>::call(first1, last1, first2, last2, dest,
            HPX_FORWARD(F, f));
    }
}    // namespace hpx::parallel::util

#endif
