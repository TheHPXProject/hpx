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
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/replace.hpp>
#include <hpx/parallel/datapar/handle_local_exceptions.hpp>
#include <hpx/parallel/datapar/loop.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx { namespace parallel { namespace detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_replace
    {
        template <typename InIter, typename T1, typename T2, typename Proj>
        HPX_HOST_DEVICE HPX_FORCEINLINE static auto call(
            [[maybe_unused]] ExPolicy&& policy, InIter first, InIter last,
            T1 const& old_value, T2 const& new_value, Proj&& proj)
        {
            if constexpr (hpx::is_sequenced_execution_policy_v<ExPolicy>)
            {
                return util::loop_ind<ExPolicy>(
                    first, last, [old_value, new_value, &proj](auto&& v) {
                        using var_type = std::decay_t<decltype(v)>;
                        traits::mask_assign(
                            HPX_INVOKE(proj, v) == var_type(old_value), v,
                            var_type(new_value));
                    });
            }
            else
            {
                return for_each_n<InIter>().call(
                    HPX_FORWARD(ExPolicy, policy), first,
                    detail::distance(first, last),
                    [old_value, new_value, proj = HPX_FORWARD(Proj, proj)](
                        auto& v) -> void {
                        using var_type = std::decay_t<decltype(v)>;
                        traits::mask_assign(
                            HPX_INVOKE(proj, v) == var_type(old_value), v,
                            var_type(new_value));
                    },
                    hpx::identity_v);
            }
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename T1, typename T2, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE auto tag_invoke(
        sequential_replace_t<ExPolicy>, ExPolicy&& policy, InIter first,
        InIter last, T1 const& old_value, T2 const& new_value, Proj&& proj)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          InIter>::value)
        {
            return datapar_replace<ExPolicy>::call(
                HPX_FORWARD(ExPolicy, policy), first, last, old_value,
                new_value, HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_replace<base_policy_type>(
                hpx::execution::experimental::to_non_simd(policy), first, last,
                old_value, new_value, HPX_FORWARD(Proj, proj));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_replace_if
    {
        template <typename InIter, typename Sent, typename F, typename T,
            typename Proj>
        HPX_HOST_DEVICE HPX_FORCEINLINE static auto call(
            [[maybe_unused]] ExPolicy&& policy, InIter first, Sent last, F&& f,
            T const& new_value, Proj&& proj)
        {
            if constexpr (hpx::is_sequenced_execution_policy_v<ExPolicy>)
            {
                return util::loop_ind<ExPolicy>(
                    first, last, [&f, new_value, &proj](auto&& v) {
                        using var_type = std::decay_t<decltype(v)>;
                        traits::mask_assign(HPX_INVOKE(f, HPX_INVOKE(proj, v)),
                            v, var_type(new_value));
                    });
            }
            else
            {
                return for_each_n<InIter>().call(
                    HPX_FORWARD(ExPolicy, policy), first,
                    detail::distance(first, last),
                    [new_value, f = HPX_FORWARD(F, f),
                        proj = HPX_FORWARD(Proj, proj)](
                        auto& v) mutable -> void {
                        using var_type = std::decay_t<decltype(v)>;
                        traits::mask_assign(
                            HPX_INVOKE(f, (HPX_INVOKE(proj, v))), v,
                            var_type(new_value));
                    },
                    hpx::identity_v);
            }
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename Sent, typename F, typename T, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE auto tag_invoke(
        sequential_replace_if_t<ExPolicy>, ExPolicy&& policy, InIter first,
        Sent last, F&& f, T const& new_value, Proj&& proj)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          InIter>::value)
        {
            return datapar_replace_if<ExPolicy>::call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(F, f),
                new_value, HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_replace_if<base_policy_type>(
                hpx::execution::experimental::to_non_simd(policy), first, last,
                HPX_FORWARD(F, f), new_value, HPX_FORWARD(Proj, proj));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_replace_copy
    {
        template <typename InIter, typename Sent, typename OutIter, typename T,
            typename Proj>
        HPX_HOST_DEVICE HPX_FORCEINLINE static auto call(ExPolicy&& policy,
            InIter first, Sent sent, OutIter dest, T const& old_value,
            T const& new_value, Proj&& proj)
        {
            if constexpr (hpx::is_sequenced_execution_policy_v<ExPolicy>)
            {
                return util::detail::get_in_out_result(
                    util::loop_n_ind<ExPolicy>(
                        hpx::util::zip_iterator(first, dest),
                        detail::distance(first, sent),
                        [old_value, new_value, proj = HPX_FORWARD(Proj, proj)](
                            auto& v) {
                            using var_type = std::decay_t<decltype(get<0>(v))>;
                            get<1>(v) =
                                traits::choose(HPX_INVOKE(proj, get<0>(v)) ==
                                        var_type(old_value),
                                    var_type(new_value), get<0>(v));
                        }));
            }
            else
            {
                typedef hpx::util::zip_iterator<InIter, OutIter> zip_iterator;

                return util::detail::get_in_out_result(
                    for_each_n<zip_iterator>().call(
                        HPX_FORWARD(ExPolicy, policy),
                        hpx::util::zip_iterator(first, dest),
                        detail::distance(first, sent),
                        [old_value, new_value, proj = HPX_FORWARD(Proj, proj)](
                            auto& v) -> void {
                            using hpx::get;
                            using var_type = std::decay_t<decltype(get<0>(v))>;
                            get<1>(v) =
                                traits::choose(HPX_INVOKE(proj, get<0>(v)) ==
                                        var_type(old_value),
                                    var_type(new_value), get<0>(v));
                        },
                        hpx::identity_v));
            }
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename Sent, typename OutIter, typename T, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE auto tag_invoke(
        sequential_replace_copy_t<ExPolicy>, ExPolicy&& policy, InIter first,
        Sent sent, OutIter dest, T const& old_value, T const& new_value,
        Proj&& proj)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          InIter>::value)
        {
            return datapar_replace_copy<ExPolicy>::call(
                HPX_FORWARD(ExPolicy, policy), first, sent, dest, old_value,
                new_value, HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_replace_copy<base_policy_type>(
                hpx::execution::experimental::to_non_simd(policy), first, sent,
                dest, old_value, new_value, HPX_FORWARD(Proj, proj));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_replace_copy_if
    {
        template <typename InIter, typename Sent, typename OutIter, typename F,
            typename T, typename Proj>
        HPX_HOST_DEVICE HPX_FORCEINLINE static auto call(ExPolicy&& policy,
            InIter first, Sent last, OutIter dest, F&& f, T const& new_value,
            Proj&& proj)
        {
            if constexpr (hpx::is_sequenced_execution_policy_v<ExPolicy>)
            {
                return util::detail::get_in_out_result(
                    util::loop_n_ind<ExPolicy>(
                        hpx::util::zip_iterator(first, dest),
                        detail::distance(first, last),
                        [new_value, f = HPX_FORWARD(F, f),
                            proj = HPX_FORWARD(Proj, proj)](
                            auto& v) mutable -> void {
                            using hpx::get;
                            using var_type = std::decay_t<decltype(get<0>(v))>;
                            get<1>(v) = traits::choose(
                                HPX_INVOKE(f, HPX_INVOKE(proj, get<0>(v))),
                                var_type(new_value), get<0>(v));
                        }));
            }
            else
            {
                typedef hpx::util::zip_iterator<InIter, OutIter> zip_iterator;

                return util::detail::get_in_out_result(
                    for_each_n<zip_iterator>().call(
                        HPX_FORWARD(ExPolicy, policy),
                        hpx::util::zip_iterator(first, dest),
                        detail::distance(first, last),
                        [new_value, f = HPX_FORWARD(F, f),
                            proj = HPX_FORWARD(Proj, proj)](
                            auto& v) mutable -> void {
                            using hpx::get;
                            using var_type = std::decay_t<decltype(get<0>(v))>;
                            get<1>(v) = traits::choose(
                                HPX_INVOKE(f, HPX_INVOKE(proj, get<0>(v))),
                                var_type(new_value), get<0>(v));
                        },
                        hpx::identity_v));
            }
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename Sent, typename OutIter, typename F, typename T, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE auto tag_invoke(
        sequential_replace_copy_if_t<ExPolicy>, ExPolicy&& policy, InIter first,
        Sent last, OutIter dest, F&& f, T const& new_value, Proj&& proj)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          InIter>::value)
        {
            return datapar_replace_copy_if<ExPolicy>::call(
                HPX_FORWARD(ExPolicy, policy), first, last, dest,
                HPX_FORWARD(F, f), new_value, HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_replace_copy_if<base_policy_type>(
                hpx::execution::experimental::to_non_simd(policy), first, last,
                dest, HPX_FORWARD(F, f), new_value, HPX_FORWARD(Proj, proj));
        }
    }
}}}    // namespace hpx::parallel::detail

#endif
