//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <cstddef>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_copy_if_t final
      : hpx::detail::tag_dispatch<sequential_copy_if_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename InIter1, typename InIter2, typename OutIter,
            typename Pred, typename Proj>
        static constexpr util::in_out_result<InIter1, OutIter> invoke_default(
            InIter1 first, InIter2 last, OutIter dest, Pred&& pred, Proj&& proj)
        {
            while (first != last)
            {
                if (HPX_INVOKE(pred, HPX_INVOKE(proj, *first)))
                    *dest++ = *first;
                ++first;
            }
            return util::in_out_result<InIter1, OutIter>{
                HPX_MOVE(first), HPX_MOVE(dest)};
        }

        template <typename ZipIter, typename Pred, typename Proj>
        static constexpr std::size_t invoke_default(
            ZipIter part_begin, std::size_t part_size, Pred&& pred, Proj&& proj)
        {
            using hpx::get;
            std::size_t curr = 0;

            // Note: replacing invoke() with HPX_INVOKE() below makes gcc generate errors
            util::const_loop_n<ExPolicy>(part_begin, part_size,
                [&pred, &proj, &curr](ZipIter it) mutable -> void {
                    bool f = static_cast<bool>(
                        hpx::invoke(pred, hpx::invoke(proj, get<0>(*it))));

                    // NOLINTNEXTLINE(bugprone-assignment-in-if-condition)
                    if ((get<1>(*it) = f))
                        ++curr;
                });

            return curr;
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_copy_if_t<ExPolicy> sequential_copy_if =
        sequential_copy_if_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename Pred, typename Proj>
    HPX_HOST_DEVICE HPX_FORCEINLINE
        util::in_out_result<InIter1, OutIter> sequential_copy_if(
            InIter1 first, InIter2 last, OutIter dest, Pred&& pred, Proj&& proj)
    {
        return sequential_copy_if_t<ExPolicy>{}(first, last, dest,
            HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename ZipIter,
        typename Pred, typename Proj>
    HPX_HOST_DEVICE HPX_FORCEINLINE std::size_t sequential_copy_if(
        ZipIter part_begin, std::size_t part_size, Pred&& pred, Proj&& proj)
    {
        return sequential_copy_if_t<ExPolicy>{}(part_begin, part_size,
            HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
    }
#endif

}    // namespace hpx::parallel::detail
