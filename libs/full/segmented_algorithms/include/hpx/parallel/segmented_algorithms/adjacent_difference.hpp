//  Copyright (c) 2017 Ajai V George
//  Copyright (c) 2021 Karame M.Shokooh
//  Copyright (c) 2024-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <list>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented_adjacent_difference

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename Op>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    segmented_adjacent_difference(Algo&& algo, ExPolicy const& policy,
        FwdIter1 first, FwdIter1 last, FwdIter2 dest, Op&& op, std::true_type)
    {
        using traits1 = hpx::traits::segmented_iterator_traits<FwdIter1>;
        using traits2 = hpx::traits::segmented_iterator_traits<FwdIter2>;
        using segment_iterator1 = typename traits1::segment_iterator;
        using local_iterator_type1 = typename traits1::local_iterator;
        using segment_iterator2 = typename traits2::segment_iterator;
        using local_iterator_type2 = typename traits2::local_iterator;

        using result =
            hpx::parallel::util::detail::algorithm_result<ExPolicy, FwdIter2>;

        FwdIter2 end_dest = dest, curr;
        std::advance(end_dest, std::distance(first, last));

        segment_iterator1 sit = traits1::segment(first);
        segment_iterator1 send = traits1::segment(last);
        segment_iterator2 sdest = traits2::segment(dest);

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type1 beg = traits1::local(first);
            local_iterator_type1 end = traits1::local(last);
            local_iterator_type2 ldest = traits2::local(dest);
            if (beg != end)
            {
                local_iterator_type2 out = dispatch(traits2::get_id(sdest),
                    algo, policy, std::true_type(), beg, end, ldest, op);
                dest = traits2::compose(sdest, out);
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type1 beg = traits1::local(first);
            local_iterator_type1 end = traits1::end(sit);
            local_iterator_type2 ldest = traits2::local(dest);
            local_iterator_type2 out = traits2::local(dest);
            if (beg != end)
            {
                out = dispatch(traits2::get_id(sdest), algo, policy,
                    std::true_type(), beg, end, ldest, op);
            }

            // handle all full partitions
            for (++sit, ++sdest; sit != send; ++sit, ++sdest)
            {
                beg = traits1::begin(sit);
                end = traits1::end(sit);
                ldest = traits2::begin(sdest);
                curr = traits2::compose(sdest, ldest);
                if (beg != end)
                {
                    out = dispatch(traits2::get_id(sdest), algo, policy,
                        std::true_type(), beg, end, ldest, op);

                    FwdIter1 beginning = traits1::compose(sit, beg);
                    if (beginning != last)
                    {
                        if (curr != end_dest)
                        {
                            *curr = HPX_INVOKE(
                                op, *beginning, *std::prev(beginning));
                        }
                    }
                }
            }

            // handle the beginning of the last partition
            beg = traits1::begin(sit);
            end = traits1::local(last);
            ldest = traits2::begin(sdest);
            curr = traits2::compose(sdest, ldest);
            if (beg != end)
            {
                out = dispatch(traits2::get_id(sdest), algo, policy,
                    std::true_type(), beg, end, ldest, op);

                FwdIter1 beginning = traits1::compose(sit, beg);
                if (beginning != last)
                {
                    if (curr != end_dest)
                    {
                        *curr =
                            HPX_INVOKE(op, *beginning, *std::prev(beginning));
                    }
                }
            }
            dest = traits2::compose(sdest, out);
        }
        return result::get(HPX_MOVE(dest));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename Op>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    segmented_adjacent_difference(Algo&& algo, ExPolicy const& policy,
        FwdIter1 first, FwdIter1 last, FwdIter2 dest, Op&& op, std::false_type)
    {
        using traits1 = hpx::traits::segmented_iterator_traits<FwdIter1>;
        using traits2 = hpx::traits::segmented_iterator_traits<FwdIter2>;
        using segment_iterator1 = typename traits1::segment_iterator;
        using local_iterator_type1 = typename traits1::local_iterator;
        using segment_iterator2 = typename traits2::segment_iterator;
        using local_iterator_type2 = typename traits2::local_iterator;

        using result =
            hpx::parallel::util::detail::algorithm_result<ExPolicy, FwdIter2>;

        using forced_seq =
            std::integral_constant<bool, !std::forward_iterator<FwdIter1>>;

        segment_iterator1 sit = traits1::segment(first);
        segment_iterator1 send = traits1::segment(last);
        segment_iterator2 sdest = traits2::segment(dest);

        using segment_type = std::vector<future<local_iterator_type2>>;
        segment_type segments;

        auto size = std::distance(sit, send);
        segments.reserve(size);

        std::vector<FwdIter1> between_segments;
        between_segments.reserve(size);

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type1 beg = traits1::local(first);
            local_iterator_type1 end = traits1::local(last);
            local_iterator_type2 ldest = traits2::local(dest);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits2::get_id(sdest), algo,
                    policy, forced_seq(), beg, end, ldest, op));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type1 beg = traits1::local(first);
            local_iterator_type1 end = traits1::end(sit);
            local_iterator_type2 ldest = traits2::local(dest);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits2::get_id(sdest), algo,
                    policy, forced_seq(), beg, end, ldest, op));
            }

            // handle all full partitions
            for (++sit, ++sdest; sit != send; ++sit, ++sdest)
            {
                beg = traits1::begin(sit);
                end = traits1::end(sit);
                ldest = traits2::begin(sdest);
                if (beg != end)
                {
                    between_segments.push_back(traits1::compose(sit, beg));
                    segments.push_back(dispatch_async(traits2::get_id(sdest),
                        algo, policy, forced_seq(), beg, end, ldest, op));
                }
            }

            // handle the beginning of the last partition
            beg = traits1::begin(sit);
            end = traits1::local(last);
            ldest = traits2::begin(sdest);
            if (beg != end)
            {
                between_segments.push_back(traits1::compose(sit, beg));
                segments.push_back(dispatch_async(traits2::get_id(sdest), algo,
                    policy, forced_seq(), beg, end, ldest, op));
            }
        }

        return result::get(dataflow(
            [=](segment_type&& r) -> FwdIter2 {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);

                auto ft = r.back().get();
                auto odest = traits2::compose(sdest, ft);
                auto start = between_segments.begin();
                while (start != between_segments.end())
                {
                    FwdIter2 curr = dest;
                    std::advance(curr, std::distance(first, *start));
                    *curr = HPX_INVOKE(op, *(*start), *std::prev(*start));
                    start = std::next(start);
                }
                return odest;
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename Op>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_segmented_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2> &&
            hpx::traits::is_segmented_iterator_v<FwdIter2>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    tag_invoke(hpx::adjacent_difference_t, ExPolicy&& policy, FwdIter1 first,
        FwdIter1 last, FwdIter2 dest, Op&& op)
    {
        static_assert(std::forward_iterator<FwdIter1>,
            "Requires at least forward iterator.");
        static_assert(std::forward_iterator<FwdIter2>,
            "Requires at least forward iterator.");

        if (first == last)
        {
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    FwdIter2>;
            return result::get(HPX_MOVE(dest));
        }

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
        using traits = hpx::traits::segmented_iterator_traits<FwdIter2>;

        return hpx::parallel::detail::segmented_adjacent_difference(
            hpx::parallel::detail::adjacent_difference<
                typename traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last, dest,
            HPX_FORWARD(Op, op), is_seq());
    }

    HPX_CXX_EXPORT template <typename InIter1, typename InIter2, typename Op>
        requires(hpx::traits::is_iterator_v<InIter1> &&
            hpx::traits::is_segmented_iterator_v<InIter1> &&
            hpx::traits::is_iterator_v<InIter2> &&
            hpx::traits::is_segmented_iterator_v<InIter2>)
    InIter2 tag_invoke(hpx::adjacent_difference_t, InIter1 first, InIter1 last,
        InIter2 dest, Op&& op)
    {
        static_assert(
            std::input_iterator<InIter1>, "Requires at least input iterator.");
        static_assert(
            std::input_iterator<InIter2>, "Requires at least input iterator.");

        if (first == last)
        {
            return dest;
        }

        using traits = hpx::traits::segmented_iterator_traits<InIter2>;

        return hpx::parallel::detail::segmented_adjacent_difference(
            hpx::parallel::detail::adjacent_difference<
                typename traits::local_iterator>(),
            hpx::execution::seq, first, last, dest, HPX_FORWARD(Op, op),
            std::true_type{});
    }
}    // namespace hpx::segmented
