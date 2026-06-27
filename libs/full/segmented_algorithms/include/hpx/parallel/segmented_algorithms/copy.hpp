//  Copyright (c) 2026 Arpit Khandelwal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/executors.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>
#include <hpx/parallel/segmented_algorithms/detail/transfer.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <list>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented_copy

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename OutIter>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<SegIter, OutIter>>
    segmented_copy(Algo&& algo, ExPolicy const& policy, SegIter first,
        SegIter last, OutIter dest, std::true_type)
    {
        using traits1 = hpx::traits::segmented_iterator_traits<SegIter>;
        using traits2 = hpx::traits::segmented_iterator_traits<OutIter>;
        using segment_iterator1 = typename traits1::segment_iterator;
        using local_iterator_type1 = typename traits1::local_iterator;
        using segment_iterator2 = typename traits2::segment_iterator;
        using local_iterator_type2 = typename traits2::local_iterator;

        using result = util::detail::algorithm_result<ExPolicy,
            util::in_out_result<SegIter, OutIter>>;

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
                util::in_out_result<local_iterator_type1, local_iterator_type2>
                    out = dispatch(traits2::get_id(sdest), algo, policy,
                        std::true_type(), beg, end, ldest);
                last = traits1::compose(send, out.in);
                dest = traits2::compose(sdest, out.out);
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type1 beg = traits1::local(first);
            local_iterator_type1 end = traits1::end(sit);
            local_iterator_type2 ldest = traits2::local(dest);
            util::in_out_result<local_iterator_type1, local_iterator_type2> out{
                beg, ldest};
            if (beg != end)
            {
                out = dispatch(traits2::get_id(sdest), algo, policy,
                    std::true_type(), beg, end, ldest);
            }

            // handle all of the full partitions
            for (++sit, ++sdest; sit != send; ++sit, ++sdest)
            {
                beg = traits1::begin(sit);
                end = traits1::end(sit);
                ldest = traits2::begin(sdest);
                out = util::in_out_result<local_iterator_type1,
                    local_iterator_type2>{beg, ldest};
                if (beg != end)
                {
                    out = dispatch(traits2::get_id(sdest), algo, policy,
                        std::true_type(), beg, end, ldest);
                }
            }

            // handle the beginning of the last partition
            beg = traits1::begin(sit);
            end = traits1::local(last);
            ldest = traits2::begin(sdest);
            out =
                util::in_out_result<local_iterator_type1, local_iterator_type2>{
                    beg, ldest};
            if (beg != end)
            {
                out = dispatch(traits2::get_id(sdest), algo, policy,
                    std::true_type(), beg, end, ldest);
            }
            last = traits1::compose(send, out.in);
            dest = traits2::compose(sdest, out.out);
        }
        return result::get(util::in_out_result<SegIter, OutIter>{
            HPX_MOVE(last), HPX_MOVE(dest)});
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename OutIter>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<SegIter, OutIter>>
    segmented_copy(Algo&& algo, ExPolicy const& policy, SegIter first,
        SegIter last, OutIter dest, std::false_type)
    {
        using traits1 = hpx::traits::segmented_iterator_traits<SegIter>;
        using traits2 = hpx::traits::segmented_iterator_traits<OutIter>;
        using segment_iterator1 = typename traits1::segment_iterator;
        using local_iterator_type1 = typename traits1::local_iterator;
        using segment_iterator2 = typename traits2::segment_iterator;
        using local_iterator_type2 = typename traits2::local_iterator;

        using result = util::detail::algorithm_result<ExPolicy,
            util::in_out_result<SegIter, OutIter>>;

        using forced_seq = std::integral_constant<bool,
            !hpx::traits::is_forward_iterator_v<SegIter>>;

        segment_iterator1 sit = traits1::segment(first);
        segment_iterator1 send = traits1::segment(last);
        segment_iterator2 sdest = traits2::segment(dest);

        using segment_type = std::vector<future<
            util::in_out_result<local_iterator_type1, local_iterator_type2>>>;
        segment_type segments;
        segments.reserve(std::distance(sit, send));

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type1 beg = traits1::local(first);
            local_iterator_type1 end = traits1::local(last);
            local_iterator_type2 ldest = traits2::local(dest);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits2::get_id(sdest), algo,
                    policy, forced_seq(), beg, end, ldest));
            }
            else
            {
                segments.push_back(hpx::make_ready_future(
                    util::in_out_result<local_iterator_type1,
                        local_iterator_type2>{beg, ldest}));
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
                    policy, forced_seq(), beg, end, ldest));
            }
            else
            {
                segments.push_back(hpx::make_ready_future(
                    util::in_out_result<local_iterator_type1,
                        local_iterator_type2>{beg, ldest}));
            }

            // handle all of the full partitions
            for (++sit, ++sdest; sit != send; ++sit, ++sdest)
            {
                beg = traits1::begin(sit);
                end = traits1::end(sit);
                ldest = traits2::begin(sdest);
                if (beg != end)
                {
                    segments.push_back(dispatch_async(traits2::get_id(sdest),
                        algo, policy, forced_seq(), beg, end, ldest));
                }
                else
                {
                    segments.push_back(hpx::make_ready_future(
                        util::in_out_result<local_iterator_type1,
                            local_iterator_type2>{beg, ldest}));
                }
            }

            // handle the beginning of the last partition
            beg = traits1::begin(sit);
            end = traits1::local(last);
            ldest = traits2::begin(sdest);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits2::get_id(sdest), algo,
                    policy, forced_seq(), beg, end, ldest));
            }
            else
            {
                segments.push_back(hpx::make_ready_future(
                    util::in_out_result<local_iterator_type1,
                        local_iterator_type2>{beg, ldest}));
            }
        }

        return result::get(dataflow(
            [=](segment_type&& r) -> util::in_out_result<SegIter, OutIter> {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);
                auto ft = r.back().get();
                auto olast = traits1::compose(send, ft.in);
                auto odest = traits2::compose(sdest, ft.out);
                return util::in_out_result<SegIter, OutIter>{olast, odest};
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    ///////////////////////////////////////////////////////////////////////////
    // segmented copy - tag_invoke overloads

    // no-policy (sequential) overload
    HPX_CXX_EXPORT template <typename SegIter, typename OutIter>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    OutIter tag_invoke(hpx::copy_t, SegIter first, SegIter last, OutIter dest)
    {
        static_assert(hpx::traits::is_input_iterator_v<SegIter>,
            "Requires at least input iterator.");

        if (first == last)
        {
            return HPX_MOVE(dest);
        }

        using iterator_traits1 =
            hpx::traits::segmented_iterator_traits<SegIter>;
        using iterator_traits2 =
            hpx::traits::segmented_iterator_traits<OutIter>;

        auto result = hpx::parallel::detail::segmented_copy(
            hpx::parallel::detail::copy<hpx::parallel::util::in_out_result<
                typename iterator_traits1::local_iterator,
                typename iterator_traits2::local_iterator>>(),
            hpx::execution::seq, first, last, dest, std::true_type{});

        return HPX_MOVE(result.out);
    }

    // execution-policy overload
    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter,
        typename OutIter>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, OutIter>
    tag_invoke(hpx::copy_t, ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest)
    {
        static_assert(hpx::traits::is_forward_iterator_v<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = typename hpx::is_sequenced_execution_policy<
            std::decay_t<ExPolicy>>::type;

        if (first == last)
        {
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    OutIter>;
            return result::get(HPX_MOVE(dest));
        }

        using iterator_traits1 =
            hpx::traits::segmented_iterator_traits<SegIter>;
        using iterator_traits2 =
            hpx::traits::segmented_iterator_traits<OutIter>;

        return hpx::parallel::util::get_second_element(
            hpx::parallel::detail::segmented_copy(
                hpx::parallel::detail::copy<hpx::parallel::util::in_out_result<
                    typename iterator_traits1::local_iterator,
                    typename iterator_traits2::local_iterator>>(),
                HPX_FORWARD(ExPolicy, policy), first, last, dest, is_seq{}));
    }
}    // namespace hpx::segmented
