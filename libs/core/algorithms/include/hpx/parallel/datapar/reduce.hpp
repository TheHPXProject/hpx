//  Copyright (c) 2022 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/reduce.hpp>
#include <hpx/parallel/datapar/handle_local_exceptions.hpp>
#include <hpx/parallel/datapar/loop.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_reduce
    {
        template <typename InIterB, typename InIterE, typename T,
            typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static T call(
            ExPolicy&&, InIterB first, InIterE last, T init, Reduce&& r)
        {
            util::loop_ind<ExPolicy>(
                first, last, [&init, &r](auto const& val) mutable {
                    T partial_res = hpx::parallel::traits::reduce(r, val);
                    init = r(init, partial_res);
                });
            return init;
        }

        template <typename T, typename FwdIterB, typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static T call(
            FwdIterB part_begin, std::size_t part_size, T init, Reduce r)
        {
            util::loop_n_ind<ExPolicy>(
                part_begin, part_size, [&init, &r](auto const& val) mutable {
                    T partial_res = hpx::parallel::traits::reduce(r, val);
                    init = r(init, partial_res);
                });
            return init;
        }

        template <typename Iter, typename Sent, typename T, typename Reduce,
            typename Convert>
        HPX_HOST_DEVICE HPX_FORCEINLINE static T call(ExPolicy&&, Iter first,
            Sent last, T init, Reduce&& r, Convert&& conv)
        {
            util::loop_ind<ExPolicy>(
                first, last, [&init, &r, &conv](auto const& v) mutable {
                    T partial_res = hpx::parallel::traits::reduce(r, conv(v));
                    init = r(init, partial_res);
                });
            return init;
        }

        template <typename T, typename Iter, typename Reduce, typename Convert>
        HPX_HOST_DEVICE HPX_FORCEINLINE static T call(Iter part_begin,
            std::size_t part_size, T init, Reduce r, Convert conv)
        {
            util::loop_n_ind<ExPolicy>(part_begin, part_size,
                [&init, &r, &conv](auto const& v) mutable {
                    T partial_res = hpx::parallel::traits::reduce(r, conv(v));
                    init = r(init, partial_res);
                });
            return init;
        }

        template <typename Iter1, typename Sent, typename Iter2, typename T,
            typename Reduce, typename Convert>
        HPX_HOST_DEVICE HPX_FORCEINLINE static T call(Iter1 first1, Sent last1,
            Iter2 first2, T init, Reduce&& r, Convert&& conv)
        {
            util::loop2<ExPolicy>(
                first1, last1, first2, [&init, &r, &conv](auto it1, auto it2) {
                    auto partial_res = hpx::parallel::traits::reduce(
                        r, HPX_INVOKE(conv, it1, it2));
                    init = r(init, partial_res);
                });
            return init;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIterB,
        typename InIterE, typename T, typename Reduce>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE T hpx_invoke(sequential_reduce_t<ExPolicy>,
        ExPolicy&& policy, InIterB first, InIterE last, T init, Reduce&& r)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          InIterB>::value &&
            hpx::parallel::util::detail::iterator_datapar_compatible<
                InIterE>::value)
        {
            return datapar_reduce<ExPolicy>::call(HPX_FORWARD(ExPolicy, policy),
                first, last, init, HPX_FORWARD(Reduce, r));
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_reduce<base_policy_type>(
                hpx::execution::experimental::to_non_simd(policy), first, last,
                init, HPX_FORWARD(Reduce, r));
        }
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T,
        typename FwdIter, typename Reduce>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE T hpx_invoke(sequential_reduce_t<ExPolicy>,
        FwdIter part_begin, std::size_t part_size, T init, Reduce r)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          FwdIter>::value)
        {
            return datapar_reduce<ExPolicy>::call(
                part_begin, part_size, init, r);
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_reduce<base_policy_type>(
                part_begin, part_size, init, r);
        }
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename T, typename Reduce, typename Convert>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE T hpx_invoke(sequential_reduce_t<ExPolicy>,
        ExPolicy&& policy, Iter first, Sent last, T init, Reduce&& r,
        Convert&& conv)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          Iter>::value &&
            hpx::parallel::util::detail::iterator_datapar_compatible<
                Sent>::value)
        {
            return datapar_reduce<ExPolicy>::call(HPX_FORWARD(ExPolicy, policy),
                first, last, init, HPX_FORWARD(Reduce, r),
                HPX_FORWARD(Convert, conv));
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_reduce<base_policy_type>(base_policy_type{},
                first, last, init, HPX_FORWARD(Reduce, r),
                HPX_FORWARD(Convert, conv));
        }
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T, typename Iter,
        typename Reduce, typename Convert>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE T hpx_invoke(sequential_reduce_t<ExPolicy>,
        Iter part_begin, std::size_t part_size, T init, Reduce r, Convert conv)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          Iter>::value)
        {
            return datapar_reduce<ExPolicy>::call(
                part_begin, part_size, init, r, conv);
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_reduce<base_policy_type>(
                part_begin, part_size, init, r, conv);
        }
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter1,
        typename Sent, typename Iter2, typename T, typename Reduce,
        typename Convert>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE T hpx_invoke(sequential_reduce_t<ExPolicy>,
        Iter1 first1, Sent last1, Iter2 first2, T init, Reduce&& r,
        Convert&& conv)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          Iter1>::value &&
            hpx::parallel::util::detail::iterator_datapar_compatible<
                Iter2>::value)
        {
            return datapar_reduce<ExPolicy>::call(first1, last1, first2, init,
                HPX_FORWARD(Reduce, r), HPX_FORWARD(Convert, conv));
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_reduce<base_policy_type>(first1, last1, first2,
                init, HPX_FORWARD(Reduce, r), HPX_FORWARD(Convert, conv));
        }
    }
}    // namespace hpx::parallel::detail

#endif
