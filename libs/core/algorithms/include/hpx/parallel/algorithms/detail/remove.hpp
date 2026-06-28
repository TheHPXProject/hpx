//  Copyright (c) 2026 Bhoomish Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/find.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_remove_if_t final
      : hpx::functional::detail::tag_fallback<sequential_remove_if_t<ExPolicy>>
    {
    private:
        template <typename Iter, typename Sent, typename Pred, typename Proj>
        friend constexpr Iter tag_fallback_invoke(sequential_remove_if_t,
            ExPolicy&&, Iter first, Sent last, Pred pred, Proj proj)
        {
            first = hpx::parallel::detail::sequential_find_if<ExPolicy>(
                first, last, pred, proj);

            if (first != last)
            {
                for (Iter i = first; ++i != last;)
                    if (!HPX_INVOKE(pred, HPX_INVOKE(proj, *i)))
                    {
                        *first++ = std::ranges::iter_move(i);
                    }
            }
            return first;
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_remove_if_t<ExPolicy> sequential_remove_if =
        sequential_remove_if_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename... Args>
    HPX_HOST_DEVICE HPX_FORCEINLINE auto sequential_remove_if(Args&&... args)
    {
        return sequential_remove_if_t<ExPolicy>{}(std::forward<Args>(args)...);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_remove_t final
      : hpx::functional::detail::tag_fallback<sequential_remove_t<ExPolicy>>
    {
    private:
        template <typename Iter, typename Sent, typename T, typename Proj>
        friend constexpr Iter tag_fallback_invoke(sequential_remove_t,
            ExPolicy&& policy, Iter first, Sent last, T const& value, Proj proj)
        {
            return sequential_remove_if<ExPolicy>(
                HPX_FORWARD(ExPolicy, policy), first, last,
                [&value](auto const& a) { return value == a; }, proj);
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_remove_t<ExPolicy> sequential_remove =
        sequential_remove_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename... Args>
    HPX_HOST_DEVICE HPX_FORCEINLINE auto sequential_remove(Args&&... args)
    {
        return sequential_remove_t<ExPolicy>{}(std::forward<Args>(args)...);
    }
#endif

}    // namespace hpx::parallel::detail
