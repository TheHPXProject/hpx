//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/search.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/datapar/loop.hpp>
#include <hpx/parallel/datapar/zip_iterator.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter1,
        typename Iter2, typename Token, typename Pred, typename Proj1,
        typename Proj2>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE void tag_invoke(
        sequential_search_t<ExPolicy>, Iter1 it, Iter2 s_first,
        std::size_t base_idx, std::size_t part_size, std::size_t diff,
        std::size_t count, Token& tok, Pred&& op, Proj1&& proj1, Proj2&& proj2)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          Iter1>::value &&
            hpx::parallel::util::detail::iterator_datapar_compatible<
                Iter2>::value)
        {
            using value_type = typename std::iterator_traits<Iter1>::value_type;
            using pack_type =
                hpx::parallel::traits::vector_pack_type_t<value_type>;
            constexpr std::size_t pack_size =
                hpx::parallel::traits::vector_pack_size_v<pack_type>;

            // First-element broadcast: SIMD-scan haystack for matches of
            // needle[0]; only candidates pay the scalar full-needle verify.
            auto const needle0 = HPX_INVOKE(proj2, *s_first);

            auto verify = [&](std::size_t off) -> bool {
                Iter1 hay = it;
                std::advance(hay, off);
                Iter2 nee = s_first;
                for (std::size_t k = 0; k < diff; ++k, ++hay, ++nee)
                {
                    if (!HPX_INVOKE(op, HPX_INVOKE(proj1, *hay),
                            HPX_INVOKE(proj2, *nee)))
                        return false;
                }
                return true;
            };

            std::size_t i = 0;
            Iter1 curr = it;

            // SIMD bulk pass over starting positions.
            for (; i + pack_size <= part_size; i += pack_size)
            {
                if (tok.was_cancelled(base_idx + i))
                    return;

                pack_type v(hpx::parallel::traits::vector_pack_load<pack_type,
                    value_type>::unaligned(curr));

                auto mask = HPX_INVOKE(op, HPX_INVOKE(proj1, v), needle0);

                if (!hpx::parallel::traits::none_of(mask))
                {
                    for (std::size_t j = 0; j < pack_size; ++j)
                    {
                        if (mask[j] && verify(i + j))
                        {
                            tok.cancel(base_idx + i + j);
                            return;
                        }
                    }
                }

                std::advance(curr, pack_size);
            }

            // Scalar tail
            for (; i < part_size; ++i, ++curr)
            {
                if (tok.was_cancelled(base_idx + i))
                    return;

                if (HPX_INVOKE(op, HPX_INVOKE(proj1, *curr), needle0) &&
                    verify(i))
                {
                    tok.cancel(base_idx + i);
                    return;
                }
            }
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_search_t<base_policy_type>{}(it, s_first,
                base_idx, part_size, diff, count, tok, HPX_FORWARD(Pred, op),
                HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Size, typename V, typename Token, typename Pred, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE void tag_invoke(
        sequential_search_n_t<ExPolicy>, Iter it, std::size_t base_idx,
        std::size_t part_size, std::ptrdiff_t max_start, Size count,
        V const& value_proj, Token& tok, Pred&& pred, Proj&& proj)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          Iter>::value)
        {
            using value_type = typename std::iterator_traits<Iter>::value_type;
            using pack_type =
                hpx::parallel::traits::vector_pack_type_t<value_type>;
            constexpr std::size_t pack_size =
                hpx::parallel::traits::vector_pack_size_v<pack_type>;

            // Sliding-window scan: load pack_size elements at a time and track
            // the carry (consecutive matches before the current position).
            // none_of fast-path skips packs with no match in O(1).
            std::ptrdiff_t carry = 0;
            Iter curr = it;

            // Elements to scan: all starting positions in this chunk plus the
            // tail needed to verify the last one.
            std::size_t const scan_count =
                part_size + static_cast<std::size_t>(count) - 1;
            std::size_t i = 0;

            // SIMD bulk pass
            for (; i + pack_size <= scan_count; i += pack_size)
            {
                if (tok.was_cancelled(base_idx + i))
                    return;

                pack_type v(hpx::parallel::traits::vector_pack_load<pack_type,
                    value_type>::unaligned(curr));

                auto mask = HPX_INVOKE(pred, HPX_INVOKE(proj, v), value_proj);

                if (hpx::parallel::traits::none_of(mask))
                {
                    // Fast path: no element in this pack matches; wipe carry.
                    carry = 0;
                    std::advance(curr, pack_size);
                    continue;
                }

                if (hpx::parallel::traits::all_of(mask))
                {
                    // Entire pack matches; extend the run.
                    std::ptrdiff_t const carry_before = carry;
                    carry += static_cast<std::ptrdiff_t>(pack_size);
                    if (carry >= static_cast<std::ptrdiff_t>(count))
                    {
                        std::ptrdiff_t const start =
                            static_cast<std::ptrdiff_t>(base_idx + i) -
                            carry_before;
                        if (start < max_start)
                            tok.cancel(start);
                        return;
                    }
                    std::advance(curr, pack_size);
                    continue;
                }

                // Mixed: process lane by lane to maintain exact carry.
                for (std::size_t j = 0; j < pack_size; ++j, ++curr)
                {
                    if (mask[j])
                    {
                        if (++carry >= static_cast<std::ptrdiff_t>(count))
                        {
                            std::ptrdiff_t const start =
                                static_cast<std::ptrdiff_t>(base_idx + i + j) -
                                (static_cast<std::ptrdiff_t>(count) - 1);
                            if (start < max_start)
                                tok.cancel(start);
                            return;
                        }
                    }
                    else
                    {
                        carry = 0;
                    }
                }
            }

            // Scalar tail (< pack_size elements remaining)
            for (; i < scan_count; ++i, ++curr)
            {
                if (tok.was_cancelled(base_idx + i))
                    return;

                if (HPX_INVOKE(pred, HPX_INVOKE(proj, *curr), value_proj))
                {
                    if (++carry >= static_cast<std::ptrdiff_t>(count))
                    {
                        std::ptrdiff_t const start =
                            static_cast<std::ptrdiff_t>(base_idx + i) -
                            (static_cast<std::ptrdiff_t>(count) - 1);
                        if (start < max_start)
                            tok.cancel(start);
                        return;
                    }
                }
                else
                {
                    carry = 0;
                }
            }
        }
        else
        {
            using base_policy_type =
                decltype((hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>())));
            return sequential_search_n_t<base_policy_type>{}(it, base_idx,
                part_size, max_start, count, value_proj, tok,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
    }

}    // namespace hpx::parallel::detail

#endif
