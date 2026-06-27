//  Copyright (c) 2026 Abir Roy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>

#include <algorithm>
#include <exception>
#include <iterator>
#include <list>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented_equal

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter1,
        typename SegIter2, typename Pred>
    util::detail::algorithm_result_t<ExPolicy, bool> segmented_equal(
        Algo&& algo, ExPolicy const& policy, SegIter1 first1, SegIter1 last1,
        SegIter2 first2, Pred&& pred, std::true_type)
    {
        // Traits for Vector A
        typedef hpx::traits::segmented_iterator_traits<SegIter1> traits1;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;

        // Traits for Vector B
        typedef hpx::traits::segmented_iterator_traits<SegIter2> traits2;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;

        segment_iterator1 sit1 = traits1::segment(first1);
        segment_iterator1 send1 = traits1::segment(last1);
        segment_iterator2 sit2 = traits2::segment(first2);

        using result = util::detail::algorithm_result<ExPolicy, bool>;

        bool overall_result = true;

        if (sit1 == send1)
        {
            // all elements are on the same partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::local(last1);
            local_iterator_type2 beg2 = traits2::local(first2);
            if (beg1 != end1)
            {
                overall_result &= dispatch(traits1::get_id(sit1), algo, policy,
                    std::true_type(), beg1, end1, beg2,
                    HPX_FORWARD(Pred, pred));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::end(sit1);
            local_iterator_type2 beg2 = traits2::local(first2);
            if (beg1 != end1)
            {
                overall_result = dispatch(traits1::get_id(sit1), algo, policy,
                    std::true_type(), beg1, end1, beg2, pred);
            }

            // handle all of the full partitions
            for (++sit1, ++sit2; sit1 != send1; ++sit1, ++sit2)
            {
                if (!overall_result)
                    break;

                beg1 = traits1::begin(sit1);
                end1 = traits1::end(sit1);
                beg2 = traits2::begin(sit2);

                if (beg1 != end1)
                {
                    overall_result &= dispatch(traits1::get_id(sit1), algo,
                        policy, std::true_type(), beg1, end1, beg2, pred);
                }
            }

            // handle the beginning of the last partition
            beg1 = traits1::begin(sit1);
            end1 = traits1::local(last1);
            beg2 = traits2::begin(sit2);
            if (beg1 != end1)
            {
                overall_result &= dispatch(traits1::get_id(sit1), algo, policy,
                    std::true_type(), beg1, end1, beg2,
                    HPX_FORWARD(Pred, pred));
            }
        }

        return result::get(HPX_MOVE(overall_result));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter1,
        typename SegIter2, typename Pred>
    util::detail::algorithm_result_t<ExPolicy, bool> segmented_equal(
        Algo&& algo, ExPolicy const& policy, SegIter1 first1, SegIter1 last1,
        SegIter2 first2, Pred&& pred, std::false_type)
    {
        // Traits for Vector A
        typedef hpx::traits::segmented_iterator_traits<SegIter1> traits1;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;

        // Traits for Vector B
        typedef hpx::traits::segmented_iterator_traits<SegIter2> traits2;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;

        typedef std::integral_constant<bool, !std::forward_iterator<SegIter1>>
            forced_seq;

        using result = util::detail::algorithm_result<ExPolicy, bool>;

        segment_iterator1 sit1 = traits1::segment(first1);
        segment_iterator1 send1 = traits1::segment(last1);
        segment_iterator2 sit2 = traits2::segment(first2);

        std::vector<hpx::shared_future<bool>> segments;
        segments.reserve(detail::distance(sit1, send1));

        if (sit1 == send1)
        {
            // all elements are on the same partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::local(last1);
            local_iterator_type2 beg2 = traits2::local(first2);
            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2,
                    HPX_FORWARD(Pred, pred)));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::end(sit1);
            local_iterator_type2 beg2 = traits2::local(first2);
            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, pred));
            }

            // handle all of the full partitions
            for (++sit1, ++sit2; sit1 != send1; ++sit1, ++sit2)
            {
                beg1 = traits1::begin(sit1);
                end1 = traits1::end(sit1);
                beg2 = traits2::begin(sit2);
                if (beg1 != end1)
                {
                    segments.push_back(dispatch_async(traits1::get_id(sit1),
                        algo, policy, forced_seq(), beg1, end1, beg2, pred));
                }
            }

            // handle the beginning of the last partition
            beg1 = traits1::begin(sit1);
            end1 = traits1::local(last1);
            beg2 = traits2::begin(sit2);
            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2,
                    HPX_FORWARD(Pred, pred)));
            }
        }

        return result::get(dataflow(hpx::unwrapping([=](std::vector<bool>&& r) {
            return std::all_of(r.begin(), r.end(), [](bool v) { return v; });
        }),
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename SegIter1, typename SegIter2,
        typename Pred = hpx::parallel::detail::equal_to>
        requires(hpx::traits::is_iterator_v<SegIter1> &&
            hpx::traits::is_segmented_iterator_v<SegIter1> &&
            hpx::traits::is_iterator_v<SegIter2> &&
            hpx::traits::is_segmented_iterator_v<SegIter2>)
    bool tag_invoke(hpx::equal_t, SegIter1 first1, SegIter1 last1,
        SegIter2 first2, Pred&& pred = Pred())
    {
        static_assert(std::forward_iterator<SegIter1>,
            "Requires at least forward iterator to First Vector");

        static_assert(std::forward_iterator<SegIter2>,
            "Requires at least forward iterator to Second Vector");

        if (first1 == last1)
            return true;

        return hpx::parallel::detail::segmented_equal(
            hpx::parallel::detail::equal(), hpx::execution::seq, first1, last1,
            first2, HPX_FORWARD(Pred, pred), std::true_type());
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter1,
        typename SegIter2, typename Pred = hpx::parallel::detail::equal_to>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter1> &&
            hpx::traits::is_segmented_iterator_v<SegIter1> &&
            hpx::traits::is_iterator_v<SegIter2> &&
            hpx::traits::is_segmented_iterator_v<SegIter2>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, bool> tag_invoke(
        hpx::equal_t, ExPolicy&& policy, SegIter1 first1, SegIter1 last1,
        SegIter2 first2, Pred&& pred = Pred())
    {
        static_assert(std::forward_iterator<SegIter1>,
            "Requires at least forward iterator to First Vector");

        static_assert(std::forward_iterator<SegIter2>,
            "Requires at least forward iterator to Second Vector");

        if (first1 == last1)
        {
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy, bool>;
            return result::get(true);
        }

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        return hpx::parallel::detail::segmented_equal(
            hpx::parallel::detail::equal(), HPX_FORWARD(ExPolicy, policy),
            first1, last1, first2, HPX_FORWARD(Pred, pred), is_seq());
    }
}    // namespace hpx::segmented
