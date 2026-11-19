//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/traits/is_execution_policy.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <concepts>
#include <cstddef>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_inclusive_scan_t final
      : hpx::detail::tag_dispatch<sequential_inclusive_scan_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename InIter, typename Sent, typename OutIter, typename T,
            typename Op>
            requires(!std::integral<Sent>)
        static constexpr util::in_out_result<InIter, OutIter> invoke_default(
            InIter first, Sent last, OutIter dest, T init, Op&& op)
        {
            for (/* */; first != last; (void) ++first, ++dest)
            {
                init = HPX_INVOKE(op, init, *first);
                *dest = init;
            }
            return util::in_out_result<InIter, OutIter>{first, dest};
        }

        template <typename InIter, typename Sent, typename OutIter, typename Op>
            requires(!std::integral<Sent>)
        static constexpr util::in_out_result<InIter, OutIter> invoke_default(
            InIter first, Sent last, OutIter dest, Op&& op)
        {
            if (first != last)
            {
                auto init = *first;
                *dest++ = init;
                return invoke_default(
                    ++first, last, dest, HPX_MOVE(init), HPX_FORWARD(Op, op));
            }
            return util::in_out_result<InIter, OutIter>{first, dest};
        }

        template <typename InIter, typename OutIter, typename T, typename Op>
        static constexpr T invoke_default(
            InIter first, std::size_t count, OutIter dest, T init, Op&& op)
        {
            for (/* */; count-- != 0; (void) ++first, ++dest)
            {
                init = HPX_INVOKE(op, init, *first);
                *dest = init;
            }
            return init;
        }
    };

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename InIter,
        typename Sent, typename OutIter, typename T, typename Op>
        requires(!std::integral<Sent>)
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<InIter, OutIter>
        sequential_inclusive_scan(
            InIter first, Sent last, OutIter dest, T init, Op&& op)
    {
        return sequential_inclusive_scan_t<ExPolicy>{}(
            first, last, dest, init, HPX_FORWARD(Op, op));
    }

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename InIter,
        typename Sent, typename OutIter, typename Op>
        requires(!std::integral<Sent>)
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<InIter, OutIter>
        sequential_inclusive_scan(
            InIter first, Sent last, OutIter dest, Op&& op)
    {
        return sequential_inclusive_scan_t<ExPolicy>{}(
            first, last, dest, HPX_FORWARD(Op, op));
    }

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename InIter,
        typename Sent, typename OutIter, typename Op>
        requires(!std::integral<Sent>)
    HPX_HOST_DEVICE
        HPX_FORCEINLINE constexpr util::in_out_result<InIter, OutIter>
        sequential_inclusive_scan_noinit(
            InIter first, Sent last, OutIter dest, Op&& op)
    {
        return sequential_inclusive_scan_t<ExPolicy>{}(
            first, last, dest, HPX_FORWARD(Op, op));
    }

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename InIter,
        typename OutIter, typename T, typename Op>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr T sequential_inclusive_scan_n(
        InIter first, std::size_t count, OutIter dest, T init, Op&& op)
    {
        return sequential_inclusive_scan_t<ExPolicy>{}(
            first, count, dest, init, HPX_FORWARD(Op, op));
    }

}    // namespace hpx::parallel::detail
