//  Copyright (c) 2022 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/for_each.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_replace_t final
      : hpx::functional::detail::tag_fallback<sequential_replace_t<ExPolicy>>
    {
    private:
        template <typename InIter, typename T1, typename T2, typename Proj>
        friend constexpr auto tag_fallback_invoke(sequential_replace_t,
            ExPolicy&& policy, InIter first, InIter last, T1 const& old_value,
            T2 const& new_value, Proj&& proj)
        {
            if constexpr (hpx::is_sequenced_execution_policy_v<ExPolicy>)
            {
                return util::loop(HPX_FORWARD(ExPolicy, policy), first, last,
                    [old_value, new_value, &proj](auto&& v) {
                        if (HPX_INVOKE(proj, *v) == old_value)
                        {
                            *v = new_value;
                        }
                    });
            }
            else
            {
                using type = typename std::iterator_traits<InIter>::value_type;

                return for_each_n<InIter>().call(
                    HPX_FORWARD(ExPolicy, policy), first,
                    hpx::parallel::detail::distance(first, last),
                    [old_value, new_value, proj = HPX_FORWARD(Proj, proj)](
                        type& t) -> void {
                        if (HPX_INVOKE(proj, t) == old_value)
                        {
                            t = new_value;
                        }
                    },
                    hpx::identity_v);
            }
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_replace_t<ExPolicy> sequential_replace =
        sequential_replace_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename... Args>
    HPX_HOST_DEVICE HPX_FORCEINLINE auto sequential_replace(Args&&... args)
    {
        return sequential_replace_t<ExPolicy>{}(std::forward<Args>(args)...);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_replace_if_t final
      : hpx::functional::detail::tag_fallback<sequential_replace_if_t<ExPolicy>>
    {
    private:
        template <typename InIter, typename Sent, typename F, typename T,
            typename Proj>
        friend constexpr auto tag_fallback_invoke(sequential_replace_if_t,
            ExPolicy&& policy, InIter first, Sent last, F&& f,
            T const& new_value, Proj&& proj)
        {
            if constexpr (hpx::is_sequenced_execution_policy_v<ExPolicy>)
            {
                return util::loop(HPX_FORWARD(ExPolicy, policy), first, last,
                    [&f, new_value, &proj](auto&& v) {
                        if (HPX_INVOKE(f, HPX_INVOKE(proj, *v)))
                        {
                            *v = new_value;
                        }
                    });
            }
            else
            {
                using type = typename std::iterator_traits<InIter>::value_type;

                return for_each_n<InIter>().call(
                    HPX_FORWARD(ExPolicy, policy), first,
                    hpx::parallel::detail::distance(first, last),
                    [new_value, f = HPX_FORWARD(F, f),
                        proj = HPX_FORWARD(Proj, proj)](
                        type& t) mutable -> void {
                        if (HPX_INVOKE(f, HPX_INVOKE(proj, t)))
                        {
                            t = new_value;
                        }
                    },
                    hpx::identity_v);
            }
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_replace_if_t<ExPolicy> sequential_replace_if =
        sequential_replace_if_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename... Args>
    HPX_HOST_DEVICE HPX_FORCEINLINE auto sequential_replace_if(Args&&... args)
    {
        return sequential_replace_if_t<ExPolicy>{}(std::forward<Args>(args)...);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_replace_copy_t final
      : hpx::functional::detail::tag_fallback<
            sequential_replace_copy_t<ExPolicy>>
    {
    private:
        template <typename InIter, typename Sent, typename OutIter, typename T,
            typename Proj>
        friend constexpr auto tag_fallback_invoke(sequential_replace_copy_t,
            ExPolicy&& policy, InIter first, Sent sent, OutIter dest,
            T const& old_value, T const& new_value, Proj&& proj)
        {
            if constexpr (hpx::is_sequenced_execution_policy_v<ExPolicy>)
            {
                for (/* */; first != sent; ++first)
                {
                    if (HPX_INVOKE(proj, *first) == old_value)
                        *dest++ = new_value;
                    else
                        *dest++ = *first;
                }
                return util::in_out_result<InIter, OutIter>(first, dest);
            }
            else
            {
                using zip_iterator = hpx::util::zip_iterator<InIter, OutIter>;
                using reference = typename zip_iterator::reference;

                return util::detail::get_in_out_result(
                    for_each_n<zip_iterator>().call(
                        HPX_FORWARD(ExPolicy, policy),
                        zip_iterator(first, dest),
                        hpx::parallel::detail::distance(first, sent),
                        [old_value, new_value, proj = HPX_FORWARD(Proj, proj)](
                            reference t) -> void {
                            using hpx::get;
                            if (HPX_INVOKE(proj, get<0>(t)) == old_value)
                                get<1>(t) = new_value;
                            else
                                get<1>(t) = get<0>(t);    //-V573
                        },
                        hpx::identity_v));
            }
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_replace_copy_t<ExPolicy>
        sequential_replace_copy = sequential_replace_copy_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename... Args>
    HPX_HOST_DEVICE HPX_FORCEINLINE auto sequential_replace_copy(Args&&... args)
    {
        return sequential_replace_copy_t<ExPolicy>{}(
            std::forward<Args>(args)...);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_replace_copy_if_t final
      : hpx::functional::detail::tag_fallback<
            sequential_replace_copy_if_t<ExPolicy>>
    {
    private:
        template <typename InIter, typename Sent, typename OutIter, typename F,
            typename T, typename Proj>
        friend constexpr auto tag_fallback_invoke(sequential_replace_copy_if_t,
            ExPolicy&& policy, InIter first, Sent sent, OutIter dest, F&& f,
            T const& new_value, Proj&& proj)
        {
            if constexpr (hpx::is_sequenced_execution_policy_v<ExPolicy>)
            {
                for (/* */; first != sent; ++first)
                {
                    if (HPX_INVOKE(f, HPX_INVOKE(proj, *first)))
                    {
                        *dest++ = new_value;
                    }
                    else
                    {
                        *dest++ = *first;
                    }
                }
                return util::in_out_result<InIter, OutIter>{first, dest};
            }
            else
            {
                using zip_iterator = hpx::util::zip_iterator<InIter, OutIter>;
                using reference = typename zip_iterator::reference;

                return util::detail::get_in_out_result(
                    for_each_n<zip_iterator>().call(
                        HPX_FORWARD(ExPolicy, policy),
                        zip_iterator(first, dest),
                        hpx::parallel::detail::distance(first, sent),
                        [new_value, f = HPX_FORWARD(F, f),
                            proj = HPX_FORWARD(Proj, proj)](
                            reference t) mutable -> void {
                            using hpx::get;
                            if (HPX_INVOKE(f, HPX_INVOKE(proj, get<0>(t))))
                            {
                                get<1>(t) = new_value;
                            }
                            else
                            {
                                get<1>(t) = get<0>(t);    //-V573
                            }
                        },
                        hpx::identity_v));
            }
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_replace_copy_if_t<ExPolicy>
        sequential_replace_copy_if = sequential_replace_copy_if_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename... Args>
    HPX_HOST_DEVICE HPX_FORCEINLINE auto sequential_replace_copy_if(
        Args&&... args)
    {
        return sequential_replace_copy_if_t<ExPolicy>{}(
            std::forward<Args>(args)...);
    }
#endif

}    // namespace hpx::parallel::detail
