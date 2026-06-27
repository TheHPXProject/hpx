//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>
#include <hpx/parallel/segmented_algorithms/detail/reduce.hpp>

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
    // segmented_transform_reduce

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter, typename T,
        typename Reduce, typename Convert>
    util::detail::algorithm_result_t<ExPolicy, T> segmented_transform_reduce(
        Algo&& algo, ExPolicy const& policy, FwdIter first, FwdIter last,
        T&& init, Reduce&& red_op, Convert&& conv_op, std::true_type)
    {
        typedef hpx::traits::segmented_iterator_traits<FwdIter> traits;
        typedef typename traits::segment_iterator segment_iterator;
        typedef typename traits::local_iterator local_iterator_type;
        typedef util::detail::algorithm_result<ExPolicy, T> result;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        T overall_result = init;

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                overall_result = HPX_INVOKE(red_op, overall_result,
                    dispatch(traits::get_id(sit), algo, policy,
                        std::true_type(), beg, end, red_op, conv_op));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);
            if (beg != end)
            {
                overall_result = HPX_INVOKE(red_op, overall_result,
                    dispatch(traits::get_id(sit), algo, policy,
                        std::true_type(), beg, end, red_op, conv_op));
            }

            // handle all of the full partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    overall_result = HPX_INVOKE(red_op, overall_result,
                        dispatch(traits::get_id(sit), algo, policy,
                            std::true_type(), beg, end, red_op, conv_op));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                overall_result = HPX_INVOKE(red_op, overall_result,
                    dispatch(traits::get_id(sit), algo, policy,
                        std::true_type(), beg, end, red_op, conv_op));
            }
        }

        return result::get(HPX_MOVE(overall_result));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter, typename T,
        typename Reduce, typename Convert>
    util::detail::algorithm_result_t<ExPolicy, T> segmented_transform_reduce(
        Algo&& algo, ExPolicy const& policy, FwdIter first, FwdIter last,
        T&& init, Reduce&& red_op, Convert&& conv_op, std::false_type)
    {
        typedef hpx::traits::segmented_iterator_traits<FwdIter> traits;
        typedef typename traits::segment_iterator segment_iterator;
        typedef typename traits::local_iterator local_iterator_type;
        typedef util::detail::algorithm_result<ExPolicy, T> result;

        typedef std::integral_constant<bool, !std::forward_iterator<FwdIter>>
            forced_seq;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        std::vector<shared_future<T>> segments;
        segments.reserve(std::distance(sit, send));

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, red_op, conv_op));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, red_op, conv_op));
            }

            // handle all of the full partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    segments.push_back(dispatch_async(traits::get_id(sit), algo,
                        policy, forced_seq(), beg, end, red_op, conv_op));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, red_op, conv_op));
            }
        }

        return result::get(dataflow(
            [=](std::vector<shared_future<T>>&& r) -> T {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);

                // VS2015RC bails out if red_op is capture by ref
                return std::accumulate(r.begin(), r.end(), init,
                    [=](T const& val, shared_future<T>& curr) {
                        return red_op(val, curr.get());
                    });
            },
            HPX_MOVE(segments)));
    }

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename T, typename Reduce, typename Convert>
    util::detail::algorithm_result_t<ExPolicy, T> segmented_transform_reduce(
        Algo&& algo, ExPolicy const& policy, FwdIter1 first1, FwdIter1 last1,
        FwdIter2 first2, T&& init, Reduce&& red_op, Convert&& conv_op,
        std::true_type)
    {
        typedef hpx::traits::segmented_iterator_traits<FwdIter1> traits1;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;
        typedef hpx::traits::segmented_iterator_traits<FwdIter2> traits2;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;
        typedef util::detail::algorithm_result<ExPolicy, T> result;

        auto last2 = first2;
        detail::advance(last2, std::distance(first1, last1));

        segment_iterator1 sit1 = traits1::segment(first1);
        segment_iterator1 send1 = traits1::segment(last1);
        segment_iterator2 sit2 = traits1::segment(first2);

        T overall_result = init;

        if (sit1 == send1)
        {
            // all elements are on the same partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type2 beg2 = traits2::local(first2);
            local_iterator_type1 end1 = traits1::local(last1);
            if (beg1 != end1)
            {
                overall_result = HPX_INVOKE(red_op, overall_result,
                    dispatch(traits1::get_id(sit1), algo, policy,
                        std::true_type(), beg1, end1, beg2, red_op, conv_op));
            }
        }
        else
        {
            // handle the remaining part of the first1 partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type2 beg2 = traits2::local(first1);
            local_iterator_type1 end1 = traits1::end(sit1);
            if (beg1 != end1)
            {
                overall_result = HPX_INVOKE(red_op, overall_result,
                    dispatch(traits1::get_id(sit1), algo, policy,
                        std::true_type(), beg1, end1, beg2, red_op, conv_op));
            }

            // handle all of the full partitions
            for (++sit1, ++sit2; sit1 != send1; ++sit1, ++sit2)
            {
                beg1 = traits1::begin(sit1);
                beg2 = traits2::begin(sit2);
                end1 = traits1::end(sit1);
                if (beg1 != end1)
                {
                    overall_result = HPX_INVOKE(red_op, overall_result,
                        dispatch(traits1::get_id(sit1), algo, policy,
                            std::true_type(), beg1, end1, beg2, red_op,
                            conv_op));
                }
            }

            // handle the beginning of the last1 partition
            beg1 = traits1::begin(sit1);
            beg2 = traits2::begin(sit2);
            end1 = traits1::local(last1);
            if (beg1 != end1)
            {
                overall_result = HPX_INVOKE(red_op, overall_result,
                    dispatch(traits1::get_id(sit1), algo, policy,
                        std::true_type(), beg1, end1, beg2, red_op, conv_op));
            }
        }

        return result::get(HPX_MOVE(overall_result));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename T, typename Reduce, typename Convert>
    util::detail::algorithm_result_t<ExPolicy, T> segmented_transform_reduce(
        Algo&& algo, ExPolicy const& policy, FwdIter1 first1, FwdIter1 last1,
        FwdIter2 first2, T&& init, Reduce&& red_op, Convert&& conv_op,
        std::false_type)
    {
        typedef hpx::traits::segmented_iterator_traits<FwdIter1> traits1;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;
        typedef hpx::traits::segmented_iterator_traits<FwdIter2> traits2;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;
        typedef util::detail::algorithm_result<ExPolicy, T> result;

        auto last2 = first2;
        detail::advance(last2, std::distance(first1, last1));

        segment_iterator1 sit1 = traits1::segment(first1);
        segment_iterator1 send1 = traits1::segment(last1);
        segment_iterator2 sit2 = traits1::segment(first2);

        typedef std::integral_constant<bool, !std::forward_iterator<FwdIter1>>
            forced_seq;

        std::vector<shared_future<T>> segments;
        segments.reserve(std::distance(sit1, send1));

        if (sit1 == send1)
        {
            // all elements are on the same partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type2 beg2 = traits2::local(first2);
            local_iterator_type1 end1 = traits1::local(last1);
            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, red_op, conv_op));
            }
        }
        else
        {
            // handle the remaining part of the first1 partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type2 beg2 = traits2::local(first2);
            local_iterator_type1 end1 = traits1::end(sit1);
            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, red_op, conv_op));
            }

            // handle all of the full partitions
            for (++sit1, ++sit2; sit1 != send1; ++sit1, ++sit2)
            {
                beg1 = traits1::begin(sit1);
                beg2 = traits2::begin(sit2);
                end1 = traits1::end(sit1);
                if (beg1 != end1)
                {
                    segments.push_back(
                        dispatch_async(traits1::get_id(sit1), algo, policy,
                            forced_seq(), beg1, end1, beg2, red_op, conv_op));
                }
            }

            // handle the beginning of the last1 partition
            beg1 = traits1::begin(sit1);
            beg2 = traits2::begin(sit2);
            end1 = traits1::local(last1);
            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, red_op, conv_op));
            }
        }

        return result::get(dataflow(
            [=](std::vector<shared_future<T>>&& r) -> T {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);

                // VS2015RC bails out if red_op is capture by ref
                return std::accumulate(r.begin(), r.end(), init,
                    [=](T const& val, shared_future<T>& curr) {
                        return red_op(val, curr.get());
                    });
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename SegIter, typename T, typename Reduce,
        typename Convert>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    std::decay_t<T> tag_invoke(hpx::transform_reduce_t, SegIter first,
        SegIter last, T&& init, Reduce&& red_op, Convert&& conv_op)
    {
        static_assert(
            std::input_iterator<SegIter>, "Requires at least input iterator.");

        using init_type = typename std::decay<T>::type;

        if (first == last)
        {
            return HPX_FORWARD(T, init);
        }

        return hpx::parallel::detail::segmented_transform_reduce(
            hpx::parallel::detail::seg_transform_reduce<init_type>(),
            hpx::execution::seq, first, last, HPX_FORWARD(T, init),
            HPX_FORWARD(Reduce, red_op), HPX_FORWARD(Convert, conv_op),
            std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename T,
        typename Reduce, typename Convert>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    parallel::util::detail::algorithm_result_t<ExPolicy, std::decay_t<T>>
    tag_invoke(hpx::transform_reduce_t, ExPolicy&& policy, SegIter first,
        SegIter last, T&& init, Reduce&& red_op, Convert&& conv_op)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
        using init_type = typename std::decay<T>::type;

        if (first == last)
        {
            return parallel::util::detail::algorithm_result<ExPolicy,
                init_type>::get(HPX_FORWARD(T, init));
        }

        return hpx::parallel::detail::segmented_transform_reduce(
            hpx::parallel::detail::seg_transform_reduce<init_type>(),
            HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(T, init),
            HPX_FORWARD(Reduce, red_op), HPX_FORWARD(Convert, conv_op),
            is_seq());
    }

    HPX_CXX_EXPORT template <typename FwdIter1, typename FwdIter2, typename T,
        typename Reduce, typename Convert>
        requires(hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_segmented_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2> &&
            hpx::traits::is_segmented_iterator_v<FwdIter2>)
    T tag_invoke(hpx::transform_reduce_t, FwdIter1 first1, FwdIter1 last1,
        FwdIter2 first2, T init, Reduce&& red_op, Convert&& conv_op)
    {
        static_assert(
            std::input_iterator<FwdIter1> && std::input_iterator<FwdIter2>,
            "Requires at least input iterator.");

        if (first1 == last1)
        {
            return HPX_MOVE(init);
        }

        return hpx::parallel::detail::segmented_transform_reduce(
            hpx::parallel::detail::seg_transform_reduce_binary<T>(),
            hpx::execution::seq, first1, last1, first2, HPX_FORWARD(T, init),
            HPX_FORWARD(Reduce, red_op), HPX_FORWARD(Convert, conv_op),
            std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename T, typename Reduce, typename Convert>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_segmented_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2> &&
            hpx::traits::is_segmented_iterator_v<FwdIter2>)
    parallel::util::detail::algorithm_result_t<ExPolicy, T> tag_invoke(
        hpx::transform_reduce_t, ExPolicy&& policy, FwdIter1 first1,
        FwdIter1 last1, FwdIter2 first2, T init, Reduce&& red_op,
        Convert&& conv_op)
    {
        static_assert(
            std::forward_iterator<FwdIter1> && std::forward_iterator<FwdIter2>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        if (first1 == last1)
        {
            return parallel::util::detail::algorithm_result<ExPolicy, T>::get(
                HPX_FORWARD(T, init));
        }

        return hpx::parallel::detail::segmented_transform_reduce(
            hpx::parallel::detail::seg_transform_reduce_binary<T>(),
            HPX_FORWARD(ExPolicy, policy), first1, last1, first2,
            HPX_FORWARD(T, init), HPX_FORWARD(Reduce, red_op),
            HPX_FORWARD(Convert, conv_op), is_seq());
    }
}    // namespace hpx::segmented
