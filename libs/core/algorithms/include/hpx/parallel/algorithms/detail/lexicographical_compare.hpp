//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <cstddef>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_lexicographical_compare_t final
      : hpx::detail::tag_dispatch<
            sequential_lexicographical_compare_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename InIter1, typename Sent1, typename InIter2,
            typename Sent2, typename Pred, typename Proj1, typename Proj2>
        static constexpr bool invoke_default(InIter1 first1, Sent1 last1,
            InIter2 first2, Sent2 last2, Pred&& pred, Proj1&& proj1,
            Proj2&& proj2)
        {
            for (; first1 != last1 && first2 != last2; ++first1, ++first2)
            {
                if (HPX_INVOKE(pred, HPX_INVOKE(proj1, *first1),
                        HPX_INVOKE(proj2, *first2)))
                {
                    return true;
                }
                if (HPX_INVOKE(pred, HPX_INVOKE(proj2, *first2),
                        HPX_INVOKE(proj1, *first1)))
                {
                    return false;
                }
            }
            return first1 == last1 && first2 != last2;
        }

        template <typename ZipIterator, typename Token, typename Pred,
            typename Proj1, typename Proj2>
        static constexpr void invoke_default(std::size_t base_idx,
            ZipIterator it, std::size_t part_count, Token& tok, Pred&& pred,
            Proj1&& proj1, Proj2&& proj2)
        {
            using reference = typename ZipIterator::reference;

            // Note: replacing hpx::invoke() with HPX_INVOKE() below
            // may make gcc/cuda generate errors
            util::loop_idx_n<ExPolicy>(base_idx, it, part_count, tok,
                [&pred, &tok, &proj1, &proj2](
                    reference t, std::size_t i) -> void {
                    using hpx::get;
                    if (hpx::invoke(pred, hpx::invoke(proj1, get<0>(t)),
                            hpx::invoke(proj2, get<1>(t))) ||
                        hpx::invoke(pred, hpx::invoke(proj2, get<1>(t)),
                            hpx::invoke(proj1, get<0>(t))))
                    {
                        tok.cancel(i);
                    }
                });
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_lexicographical_compare_t<ExPolicy>
        sequential_lexicographical_compare =
            sequential_lexicographical_compare_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename Sent1, typename InIter2, typename Sent2, typename Pred,
        typename Proj1, typename Proj2>
    HPX_HOST_DEVICE HPX_FORCEINLINE bool sequential_lexicographical_compare(
        InIter1 first1, Sent1 last1, InIter2 first2, Sent2 last2, Pred&& pred,
        Proj1&& proj1, Proj2&& proj2)
    {
        return sequential_lexicographical_compare_t<ExPolicy>{}(first1, last1,
            first2, last2, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj1, proj1),
            HPX_FORWARD(Proj2, proj2));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename ZipIterator,
        typename Token, typename Pred, typename Proj1, typename Proj2>
    HPX_HOST_DEVICE HPX_FORCEINLINE void sequential_lexicographical_compare(
        std::size_t base_idx, ZipIterator it, std::size_t part_count,
        Token& tok, Pred&& pred, Proj1&& proj1, Proj2&& proj2)
    {
        return sequential_lexicographical_compare_t<ExPolicy>{}(base_idx, it,
            part_count, tok, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj1, proj1),
            HPX_FORWARD(Proj2, proj2));
    }
#endif

}    // namespace hpx::parallel::detail
