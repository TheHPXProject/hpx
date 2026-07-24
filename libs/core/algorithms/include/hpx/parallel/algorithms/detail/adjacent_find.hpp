//  Copyright (c) 2021 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <cstddef>
#include <functional>
#include <type_traits>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_adjacent_find_t final
      : hpx::detail::tag_dispatch<sequential_adjacent_find_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename InIter, typename Sent_, typename PredProj>
        static inline InIter invoke_default(
            InIter first, Sent_ last, PredProj&& pred_projected)
        {
            return std::adjacent_find(
                first, last, HPX_FORWARD(PredProj, pred_projected));
        }

        template <typename ZipIter, typename Token, typename PredProj>
        static inline void invoke_default(std::size_t base_idx,
            ZipIter part_begin, std::size_t part_count, Token& tok,
            PredProj&& pred_projected)
        {
            util::loop_idx_n<ExPolicy>(base_idx, part_begin, part_count, tok,
                [&pred_projected, &tok](auto t, std::size_t i) {
                    using hpx::get;
                    if (pred_projected(get<0>(t), get<1>(t)))
                        tok.cancel(i);
                });
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_adjacent_find_t<ExPolicy>
        sequential_adjacent_find = sequential_adjacent_find_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename Sent_, typename PredProj>
    HPX_HOST_DEVICE HPX_FORCEINLINE InIter sequential_adjacent_find(
        InIter first, Sent_ last, PredProj&& pred_projected)
    {
        return sequential_adjacent_find_t<ExPolicy>{}(
            first, last, HPX_FORWARD(PredProj, pred_projected));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename ZipIter,
        typename Token, typename PredProj>
    HPX_HOST_DEVICE HPX_FORCEINLINE void sequential_adjacent_find(
        std::size_t base_idx, ZipIter part_begin, std::size_t part_count,
        Token& tok, PredProj&& pred_projected)
    {
        return sequential_adjacent_find_t<ExPolicy>{}(base_idx, part_begin,
            part_count, tok, HPX_FORWARD(PredProj, pred_projected));
    }
#endif

}    // namespace hpx::parallel::detail
