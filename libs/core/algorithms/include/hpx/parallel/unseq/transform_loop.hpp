//  Copyright (c) 2022 A Kishore Kumar
//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/transform_loop.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::util {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_transform_loop_n
        {
            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static std::pair<InIter, OutIter>
            call(InIter HPX_RESTRICT it, std::size_t num,
                OutIter HPX_RESTRICT dest, F&& f)
            {
                constexpr bool iterators_are_random_access =
                    std::random_access_iterator<InIter> &&
                    std::random_access_iterator<OutIter>;

                if constexpr (iterators_are_random_access)
                {
                    // clang-format off
                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (std::size_t i = 0; i != num; ++i)
                    {
                        *dest = HPX_INVOKE(f, it);
                        ++it, ++dest;
                    }
                    // clang-format on

                    return std::make_pair(HPX_MOVE(it), HPX_MOVE(dest));
                }
                else
                {
                    return util::transform_loop_n<
                        hpx::execution::sequenced_policy>(
                        it, num, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_unsequenced_execution_policy_v<ExPolicy>,
        std::pair<Iter, OutIter>>
    tag_invoke(hpx::parallel::util::transform_loop_n_t<ExPolicy>,
        Iter HPX_RESTRICT it, std::size_t count, OutIter HPX_RESTRICT dest,
        F&& f)
    {
        return detail::unseq_transform_loop_n::call(
            it, count, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_transform_loop_n_ind
        {
            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE HPX_FORCEINLINE static std::pair<InIter, OutIter>
            call(InIter HPX_RESTRICT it, std::size_t num,
                OutIter HPX_RESTRICT dest, F&& f)
            {
                constexpr bool iterators_are_random_access =
                    std::random_access_iterator<InIter> &&
                    std::random_access_iterator<OutIter>;

                if constexpr (iterators_are_random_access)
                {
                    // clang-format off
                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (std::size_t i = 0; i != num; ++i)
                    {
                        *dest = HPX_INVOKE(f, *it);
                        ++it, ++dest;
                    }
                    // clang-format on

                    return std::make_pair(HPX_MOVE(it), HPX_MOVE(dest));
                }
                else
                {
                    return util::transform_loop_n_ind<
                        hpx::execution::sequenced_policy>(
                        it, num, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_unsequenced_execution_policy_v<ExPolicy>,
        std::pair<Iter, OutIter>>
    tag_invoke(hpx::parallel::util::transform_loop_n_ind_t<ExPolicy>,
        Iter HPX_RESTRICT it, std::size_t count, OutIter HPX_RESTRICT dest,
        F&& f)
    {
        return detail::unseq_transform_loop_n_ind::call(
            it, count, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_transform_loop
        {
            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr util::in_out_result<InIter,
                    OutIter>
                call(InIter HPX_RESTRICT it, InIter HPX_RESTRICT last,
                    OutIter HPX_RESTRICT dest, F&& f)
            {
                constexpr bool iterators_are_random_access =
                    std::random_access_iterator<InIter> &&
                    std::random_access_iterator<OutIter>;

                if constexpr (iterators_are_random_access)
                {
                    auto&& in_out = util::transform_loop_n<
                        hpx::execution::unsequenced_policy>(it,
                        hpx::parallel::detail::distance(it, last), dest,
                        HPX_FORWARD(F, f));

                    return util::in_out_result<InIter, OutIter>{
                        HPX_MOVE(hpx::get<0>(in_out)),
                        HPX_MOVE(hpx::get<1>(in_out))};
                }
                else
                {
                    return util::transform_loop(
                        hpx::execution::seq, it, last, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename IterB, typename IterE,
        typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        tag_invoke(hpx::parallel::util::transform_loop_t,
            hpx::execution::unsequenced_policy, IterB HPX_RESTRICT it,
            IterE HPX_RESTRICT end, OutIter HPX_RESTRICT dest, F&& f)
    {
        return detail::unseq_transform_loop::call(
            it, end, dest, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename IterB, typename IterE,
        typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        tag_invoke(hpx::parallel::util::transform_loop_t,
            hpx::execution::unsequenced_task_policy, IterB HPX_RESTRICT it,
            IterE HPX_RESTRICT end, OutIter HPX_RESTRICT dest, F&& f)
    {
        return detail::unseq_transform_loop::call(
            it, end, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_transform_loop_ind
        {
            template <typename InIter, typename OutIter, typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr util::in_out_result<InIter,
                    OutIter>
                call(InIter HPX_RESTRICT it, InIter HPX_RESTRICT last,
                    OutIter HPX_RESTRICT dest, F&& f)
            {
                constexpr bool iterators_are_random_access =
                    std::random_access_iterator<InIter> &&
                    std::random_access_iterator<OutIter>;

                if constexpr (iterators_are_random_access)
                {
                    auto&& in_out = util::transform_loop_n_ind<
                        hpx::execution::unsequenced_policy>(it,
                        hpx::parallel::detail::distance(it, last), dest,
                        HPX_FORWARD(F, f));

                    return util::in_out_result<InIter, OutIter>{
                        HPX_MOVE(hpx::get<0>(in_out)),
                        HPX_MOVE(hpx::get<1>(in_out))};
                }
                else
                {
                    return util::transform_loop_ind(
                        hpx::execution::seq, it, last, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename IterB, typename IterE,
        typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        tag_invoke(hpx::parallel::util::transform_loop_ind_t,
            hpx::execution::unsequenced_policy, IterB HPX_RESTRICT it,
            IterE HPX_RESTRICT end, OutIter HPX_RESTRICT dest, F&& f)
    {
        return detail::unseq_transform_loop_ind::call(
            it, end, dest, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename IterB, typename IterE,
        typename OutIter, typename F>
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<IterB, OutIter>
        tag_invoke(hpx::parallel::util::transform_loop_ind_t,
            hpx::execution::unsequenced_task_policy, IterB HPX_RESTRICT it,
            IterE HPX_RESTRICT end, OutIter HPX_RESTRICT dest, F&& f)
    {
        return detail::unseq_transform_loop_ind::call(
            it, end, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_transform_binary_loop_n
        {
            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static hpx::tuple<InIter1, InIter2, OutIter>
                call(InIter1 HPX_RESTRICT first1, std::size_t num,
                    InIter2 HPX_RESTRICT first2, OutIter HPX_RESTRICT dest,
                    F&& f)
            {
                constexpr bool iterators_are_random_access =
                    std::random_access_iterator<InIter1> &&
                    std::random_access_iterator<InIter2> &&
                    std::random_access_iterator<OutIter>;

                if constexpr (iterators_are_random_access)
                {
                    // clang-format off
                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (std::size_t i = 0; i != num; ++i)
                    {
                        *dest = HPX_INVOKE(f, first1, first2);
                        ++first1, ++first2, ++dest;
                    }
                    // clang-format on

                    return hpx::make_tuple(
                        HPX_MOVE(first1), HPX_MOVE(first2), HPX_MOVE(dest));
                }
                else
                {
                    return util::transform_binary_loop_n<
                        hpx::execution::sequenced_policy>(
                        first1, num, first2, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE
        std::enable_if_t<hpx::is_unsequenced_execution_policy_v<ExPolicy>,
            hpx::tuple<InIter1, InIter2, OutIter>>
        tag_invoke(hpx::parallel::util::transform_binary_loop_n_t<ExPolicy>,
            InIter1 HPX_RESTRICT first1, std::size_t count,
            InIter2 HPX_RESTRICT first2, OutIter HPX_RESTRICT dest, F&& f)
    {
        return detail::unseq_transform_binary_loop_n::call(
            first1, count, first2, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_transform_binary_loop
        {
            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr util::in_in_out_result<InIter1,
                    InIter2, OutIter>
                call(InIter1 HPX_RESTRICT first1, InIter1 HPX_RESTRICT last1,
                    InIter2 HPX_RESTRICT first2, OutIter HPX_RESTRICT dest,
                    F&& f)
            {
                constexpr bool iterators_are_random_access =
                    std::random_access_iterator<InIter1> &&
                    std::random_access_iterator<InIter2> &&
                    std::random_access_iterator<OutIter>;

                if constexpr (iterators_are_random_access)
                {
                    auto&& in_in_out = util::transform_binary_loop_n<
                        hpx::execution::unsequenced_policy>(first1,
                        hpx::parallel::detail::distance(first1, last1), first2,
                        dest, HPX_FORWARD(F, f));

                    return util::in_in_out_result<InIter1, InIter2, OutIter>{
                        HPX_MOVE(hpx::get<0>(in_in_out)),
                        HPX_MOVE(hpx::get<1>(in_in_out)),
                        HPX_MOVE(hpx::get<2>(in_in_out))};
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
                call(InIter1 HPX_RESTRICT first1, InIter1 HPX_RESTRICT last1,
                    InIter2 HPX_RESTRICT first2, InIter2 HPX_RESTRICT last2,
                    OutIter dest, F&& f)
            {
                constexpr bool iterators_are_random_access =
                    std::random_access_iterator<InIter1> &&
                    std::random_access_iterator<InIter2> &&
                    std::random_access_iterator<OutIter>;

                if constexpr (iterators_are_random_access)
                {
                    // clang-format off
                    std::size_t count = (std::min) (
                        hpx::parallel::detail::distance(first1, last1),
                        hpx::parallel::detail::distance(first2, last2));
                    // clang-format on

                    auto&& in_in_out = util::transform_binary_loop_n<
                        hpx::execution::unsequenced_policy>(
                        first1, count, first2, dest, HPX_FORWARD(F, f));

                    return util::in_in_out_result<InIter1, InIter2, OutIter>{
                        HPX_MOVE(hpx::get<0>(in_in_out)),
                        HPX_MOVE(hpx::get<1>(in_in_out)),
                        HPX_MOVE(hpx::get<2>(in_in_out))};
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
        hpx::is_unsequenced_execution_policy_v<ExPolicy>,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::parallel::util::transform_binary_loop_t<ExPolicy>,
        InIter1 HPX_RESTRICT first1, InIter1 HPX_RESTRICT last1,
        InIter2 HPX_RESTRICT first2, OutIter HPX_RESTRICT dest, F&& f)
    {
        return detail::unseq_transform_binary_loop::call(
            first1, last1, first2, dest, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_unsequenced_execution_policy_v<ExPolicy>,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::parallel::util::transform_binary_loop_t<ExPolicy>,
        InIter1 HPX_RESTRICT first1, InIter1 HPX_RESTRICT last1,
        InIter2 HPX_RESTRICT first2, InIter2 HPX_RESTRICT last2, OutIter dest,
        F&& f)
    {
        return detail::unseq_transform_binary_loop::call(
            first1, last1, first2, last2, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_transform_binary_loop_ind_n
        {
            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static hpx::tuple<InIter1, InIter2, OutIter>
                call(InIter1 HPX_RESTRICT first1, std::size_t num,
                    InIter2 HPX_RESTRICT first2, OutIter HPX_RESTRICT dest,
                    F&& f)
            {
                constexpr bool iterators_are_random_access =
                    std::random_access_iterator<InIter1> &&
                    std::random_access_iterator<InIter2> &&
                    std::random_access_iterator<OutIter>;

                if constexpr (iterators_are_random_access)
                {
                    // clang-format off
                    HPX_IVDEP HPX_UNROLL HPX_VECTORIZE
                    for (std::size_t i = 0; i != num; ++i)
                    {
                        *dest = HPX_INVOKE(f, *first1, *first2);
                        ++first1, ++first2, ++dest;
                    }
                    // clang-format on

                    return hpx::make_tuple(
                        HPX_MOVE(first1), HPX_MOVE(first2), HPX_MOVE(dest));
                }
                else
                {
                    return util::transform_binary_loop_ind_n<
                        hpx::execution::sequenced_policy>(
                        first1, num, first2, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_unsequenced_execution_policy_v<ExPolicy>,
        hpx::tuple<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::parallel::util::transform_binary_loop_ind_n_t<ExPolicy>,
        InIter1 HPX_RESTRICT first1, std::size_t count,
        InIter2 HPX_RESTRICT first2, OutIter HPX_RESTRICT dest, F&& f)
    {
        return detail::unseq_transform_binary_loop_ind_n::call(
            first1, count, first2, dest, HPX_FORWARD(F, f));
    }

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct unseq_transform_binary_loop_ind
        {
            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr util::in_in_out_result<InIter1,
                    InIter2, OutIter>
                call(InIter1 HPX_RESTRICT first1, InIter1 HPX_RESTRICT last1,
                    InIter2 HPX_RESTRICT first2, OutIter HPX_RESTRICT dest,
                    F&& f)
            {
                constexpr bool iterators_are_random_access =
                    std::random_access_iterator<InIter1> &&
                    std::random_access_iterator<InIter2> &&
                    std::random_access_iterator<OutIter>;

                if constexpr (iterators_are_random_access)
                {
                    auto&& in_in_out = util::transform_binary_loop_ind_n<
                        hpx::execution::unsequenced_policy>(first1,
                        hpx::parallel::detail::distance(first1, last1), first2,
                        dest, HPX_FORWARD(F, f));

                    return util::in_in_out_result<InIter1, InIter2, OutIter>{
                        HPX_MOVE(hpx::get<0>(in_in_out)),
                        HPX_MOVE(hpx::get<1>(in_in_out)),
                        HPX_MOVE(hpx::get<2>(in_in_out))};
                }
                else
                {
                    return util::transform_binary_loop_ind<
                        hpx::execution::sequenced_policy>(
                        first1, last1, first2, dest, HPX_FORWARD(F, f));
                }
            }

            template <typename InIter1, typename InIter2, typename OutIter,
                typename F>
            HPX_HOST_DEVICE
                HPX_FORCEINLINE static constexpr util::in_in_out_result<InIter1,
                    InIter2, OutIter>
                call(InIter1 HPX_RESTRICT first1, InIter1 HPX_RESTRICT last1,
                    InIter2 HPX_RESTRICT first2, InIter2 HPX_RESTRICT last2,
                    OutIter HPX_RESTRICT dest, F&& f)
            {
                constexpr bool iterators_are_random_access =
                    std::random_access_iterator<InIter1> &&
                    std::random_access_iterator<InIter2> &&
                    std::random_access_iterator<OutIter>;

                if constexpr (iterators_are_random_access)
                {
                    // clang-format off
                    std::size_t count = (std::min) (
                        hpx::parallel::detail::distance(first1, last1),
                        hpx::parallel::detail::distance(first2, last2));
                    // clang-format on

                    auto&& in_in_out = util::transform_binary_loop_ind_n<
                        hpx::execution::unsequenced_policy>(
                        first1, count, first2, dest, HPX_FORWARD(F, f));

                    return util::in_in_out_result<InIter1, InIter2, OutIter>{
                        HPX_MOVE(hpx::get<0>(in_in_out)),
                        HPX_MOVE(hpx::get<1>(in_in_out)),
                        HPX_MOVE(hpx::get<2>(in_in_out))};
                }
                else
                {
                    return util::transform_binary_loop_ind<
                        hpx::execution::sequenced_policy>(
                        first1, last1, first2, last2, dest, HPX_FORWARD(F, f));
                }
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_unsequenced_execution_policy_v<ExPolicy>,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::parallel::util::transform_binary_loop_ind_t<ExPolicy>,
        InIter1 HPX_RESTRICT first1, InIter1 HPX_RESTRICT last1,
        InIter2 HPX_RESTRICT first2, OutIter HPX_RESTRICT dest, F&& f)
    {
        return detail::unseq_transform_binary_loop_ind::call(
            first1, last1, first2, dest, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::enable_if_t<
        hpx::is_unsequenced_execution_policy_v<ExPolicy>,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::parallel::util::transform_binary_loop_ind_t<ExPolicy>,
        InIter1 HPX_RESTRICT first1, InIter1 HPX_RESTRICT last1,
        InIter2 HPX_RESTRICT first2, InIter2 HPX_RESTRICT last2,
        OutIter HPX_RESTRICT dest, F&& f)
    {
        return detail::unseq_transform_binary_loop_ind::call(
            first1, last1, first2, last2, dest, HPX_FORWARD(F, f));
    }
}    // namespace hpx::parallel::util
