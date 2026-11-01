//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/datastructures/tuple.hpp>
#include <hpx/executors/execution_policy.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_unique_t final
      : hpx::detail::tag_dispatch<sequential_unique_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename FwdIter, typename Sent, typename Pred, typename Proj>
        static constexpr FwdIter invoke_default(
            FwdIter first, Sent last, Pred&& pred, Proj&& proj)
        {
            if (first == last)
                return first;

            using projected_type = std::decay_t<std::invoke_result_t<Proj,
                typename std::iterator_traits<FwdIter>::reference>>;

            FwdIter result = first;
            projected_type result_projected = HPX_INVOKE(proj, *result);
            while (++first != last)
            {
                if (!HPX_INVOKE(
                        pred, result_projected, HPX_INVOKE(proj, *first)))
                {
                    if (++result != first)
                    {
                        *result = std::ranges::iter_move(first);
                    }
                    result_projected = HPX_INVOKE(proj, *result);
                }
            }
            return ++result;
        }

        template <typename ZipIter, typename Pred, typename Proj>
        static constexpr void invoke_default(
            ZipIter part_begin, std::size_t part_size, Pred&& pred, Proj&& proj)
        {
            using FwdIter = typename hpx::tuple_element<0,
                typename ZipIter::iterator_tuple_type>::type;

            FwdIter base = hpx::get<0>(part_begin.get_iterator_tuple());

            util::const_loop_n<std::decay_t<ExPolicy>>(++part_begin, part_size,
                [base, pred = HPX_FORWARD(Pred, pred),
                    proj = HPX_FORWARD(Proj, proj)](
                    ZipIter it) mutable -> void {
                    bool r = hpx::invoke(pred, hpx::invoke(proj, *base),
                        hpx::invoke(proj, hpx::get<0>(*it)));

                    if (!((hpx::get<1>(*it) = r)))
                        base = hpx::get<0>(it.get_iterator_tuple());
                });
        }
    };

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename FwdIter,
        typename Sent, typename Pred, typename Proj>
    HPX_HOST_DEVICE HPX_FORCEINLINE FwdIter sequential_unique(
        FwdIter first, Sent last, Pred&& pred, Proj&& proj)
    {
        return sequential_unique_t<ExPolicy>{}(
            first, last, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
    }

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename ZipIter,
        typename Pred, typename Proj>
    HPX_HOST_DEVICE HPX_FORCEINLINE void sequential_unique(
        ZipIter part_begin, std::size_t part_size, Pred&& pred, Proj&& proj)
    {
        return sequential_unique_t<ExPolicy>{}(part_begin, part_size,
            HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
    }

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_unique_copy_t final
      : hpx::detail::tag_dispatch<sequential_unique_copy_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename InIter, typename Sent, typename OutIter,
            typename Pred, typename Proj>
        static constexpr util::in_out_result<InIter, OutIter> invoke_default(
            InIter first, Sent last, OutIter dest, Pred&& pred, Proj&& proj,
            std::true_type)
        {
            if (first == last)
            {
                return util::in_out_result<InIter, OutIter>{
                    HPX_MOVE(first), HPX_MOVE(dest)};
            }

            using projected_type = std::decay_t<std::invoke_result_t<Proj,
                typename std::iterator_traits<InIter>::reference>>;

            InIter base = first;
            *dest++ = *first;
            projected_type base_projected = HPX_INVOKE(proj, *base);

            while (++first != last)
            {
                if (!HPX_INVOKE(pred, base_projected, HPX_INVOKE(proj, *first)))
                {
                    base = first;
                    *dest++ = *first;
                    base_projected = HPX_INVOKE(proj, *base);
                }
            }
            return util::in_out_result<InIter, OutIter>{
                HPX_MOVE(first), HPX_MOVE(dest)};
        }

        template <typename InIter, typename Sent, typename OutIter,
            typename Pred, typename Proj>
        static constexpr util::in_out_result<InIter, OutIter> invoke_default(
            InIter first, Sent last, OutIter dest, Pred&& pred, Proj&& proj,
            std::false_type)
        {
            if (first == last)
            {
                return util::in_out_result<InIter, OutIter>{
                    HPX_MOVE(first), HPX_MOVE(dest)};
            }

            using element_type =
                typename std::iterator_traits<InIter>::value_type;
            using projected_type = std::decay_t<std::invoke_result_t<Proj,
                typename std::iterator_traits<InIter>::reference>>;
            element_type base_val = *first;
            projected_type base_projected = HPX_INVOKE(proj, base_val);

            *dest++ = base_val;

            while (++first != last)
            {
                if (!HPX_INVOKE(pred, base_projected, HPX_INVOKE(proj, *first)))
                {
                    base_val = *first;
                    *dest++ = base_val;
                    base_projected = HPX_INVOKE(proj, base_val);
                }
            }
            return util::in_out_result<InIter, OutIter>{
                HPX_MOVE(first), HPX_MOVE(dest)};
        }

        template <typename ZipIter, typename Pred, typename Proj>
        static constexpr std::size_t invoke_default(
            ZipIter part_begin, std::size_t part_size, Pred&& pred, Proj&& proj)
        {
            using InIter = typename hpx::tuple_element<0,
                typename ZipIter::iterator_tuple_type>::type;

            InIter base = hpx::get<0>(part_begin.get_iterator_tuple());
            std::size_t curr = 0;

            util::const_loop_n<std::decay_t<ExPolicy>>(++part_begin, part_size,
                [&pred, proj, &base, &curr](ZipIter it) mutable -> void {
                    bool r = hpx::invoke(pred, hpx::invoke(proj, *base),
                        hpx::invoke(proj, hpx::get<0>(*it)));

                    if (!(hpx::get<1>(*it) = r))
                    {
                        base = hpx::get<0>(it.get_iterator_tuple());
                        ++curr;
                    }
                });

            return curr;
        }
    };

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename InIter,
        typename Sent, typename OutIter, typename Pred, typename Proj,
        typename IsForwardIter>
    HPX_HOST_DEVICE HPX_FORCEINLINE util::in_out_result<InIter, OutIter>
    sequential_unique_copy(InIter first, Sent last, OutIter dest, Pred&& pred,
        Proj&& proj, IsForwardIter is_fwd)
    {
        return sequential_unique_copy_t<ExPolicy>{}(first, last, dest,
            HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj), is_fwd);
    }

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename ZipIter,
        typename Pred, typename Proj>
    HPX_HOST_DEVICE HPX_FORCEINLINE std::size_t sequential_unique_copy(
        ZipIter part_begin, std::size_t part_size, Pred&& pred, Proj&& proj)
    {
        return sequential_unique_copy_t<ExPolicy>{}(part_begin, part_size,
            HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
    }

}    // namespace hpx::parallel::detail
