//  Copyright (c) 2021 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/parallel/algorithms/detail/adjacent_find.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/datapar/loop.hpp>
#include <hpx/parallel/datapar/zip_iterator.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_adjacent_find
    {
        template <typename InIter, typename Sent_, typename PredProj>
        static InIter call(InIter first, Sent_ last, PredProj&& pred_projected)
        {
            if (first == last)
                return last;

            InIter next = first;
            ++next;

            auto zip_iter = hpx::util::zip_iterator(first, next);
            std::size_t const count =
                hpx::parallel::detail::distance(first, last);
            util::cancellation_token<std::size_t> tok(count);

            call(0, zip_iter, count - 1, tok,
                std::forward<PredProj>(pred_projected));

            std::size_t adj_find_res = tok.get_data();
            if (adj_find_res != count)
                std::advance(first, adj_find_res);
            else
                first = last;

            return first;
        }

        template <typename ZipIter, typename Token, typename PredProj>
        static constexpr void call(std::size_t base_idx, ZipIter part_begin,
            std::size_t part_count, Token& tok, PredProj&& pred_projected)
        {
            bool cancelled = false;
            std::size_t cancel_pos = 0;
            util::loop_idx_n<ExPolicy>(base_idx, part_begin, part_count, tok,
                [&pred_projected, &cancelled, &cancel_pos](
                    auto&& t, std::size_t i) {
                    if (!cancelled)
                    {
                        using hpx::get;
                        auto msk = pred_projected(get<0>(t), get<1>(t));
                        int const offset =
                            hpx::parallel::traits::find_first_of(msk);
                        if (offset != -1)
                        {
                            cancelled = true;
                            cancel_pos = i + offset;
                        }
                    }
                });
            if (cancelled)
                tok.cancel(cancel_pos);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename Sent_, typename PredProj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr InIter tag_invoke(sequential_adjacent_find_t<ExPolicy>,
        InIter first, Sent_ last, PredProj&& pred_projected)
    {
        constexpr bool datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<InIter>;
        if constexpr (datapar_compatible)
        {
            return datapar_adjacent_find<ExPolicy>::call(
                first, last, std::forward<PredProj>(pred_projected));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));

            return sequential_adjacent_find<base_policy_type>(
                first, last, std::forward<PredProj>(pred_projected));
        }
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename ZipIter,
        typename Token, typename PredProj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr void tag_invoke(sequential_adjacent_find_t<ExPolicy>,
        std::size_t base_idx, ZipIter part_begin, std::size_t part_count,
        Token& tok, PredProj&& pred_projected)
    {
        constexpr bool datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<ZipIter>;
        if constexpr (datapar_compatible)
        {
            return datapar_adjacent_find<ExPolicy>::call(base_idx, part_begin,
                part_count, tok, std::forward<PredProj>(pred_projected));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));

            return sequential_adjacent_find<base_policy_type>(base_idx,
                part_begin, part_count, tok,
                std::forward<PredProj>(pred_projected));
        }
    }
}    // namespace hpx::parallel::detail

#endif
