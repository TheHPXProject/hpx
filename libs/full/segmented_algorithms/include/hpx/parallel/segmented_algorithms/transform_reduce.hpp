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

#include <hpx/parallel/segmented_algorithms/detail/capture_dispatch.hpp>
#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>
#include <hpx/parallel/segmented_algorithms/detail/reduce.hpp>

#include <algorithm>
#include <exception>
#include <iterator>
#include <list>
#include <numeric>
#include <tuple>
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

    template <typename Algo>
    struct captured_transform_reduce
      : algorithm<captured_transform_reduce<Algo>, typename Algo::result_type>
    {
        constexpr captured_transform_reduce() noexcept
          : algorithm<captured_transform_reduce, typename Algo::result_type>(
                "captured_transform_reduce")
        {
        }

        template <typename ExPolicy, typename Iterator, typename Values,
            typename Reduce, typename Convert>
        static typename Algo::result_type sequential(ExPolicy policy, Algo algo,
            Iterator first, Iterator last, Values values, Reduce red_op,
            Convert conv_op)
        {
            return algo.call2(policy(hpx::execution::non_task),
                std::true_type(), first, last, values.begin(), red_op, conv_op);
        }

        template <typename ExPolicy, typename Iterator, typename Values,
            typename Reduce, typename Convert>
        static typename Algo::result_type parallel(ExPolicy policy, Algo algo,
            Iterator first, Iterator last, Values values, Reduce red_op,
            Convert conv_op)
        {
            return algo.call2(policy(hpx::execution::non_task),
                std::false_type(), first, last, values.begin(), red_op,
                conv_op);
        }
    };

    template <typename Algo, typename ExPolicy, typename IsSeq,
        typename Iterator1, typename Iterator2, typename Reduce,
        typename Convert>
    hpx::future<typename Algo::result_type> dispatch_transform_reduce_async(
        hpx::id_type const& id1, hpx::id_type const& id2, Algo algo,
        ExPolicy policy, IsSeq is_seq, Iterator1 first1, Iterator1 last1,
        Iterator2 first2, Reduce red_op, Convert conv_op)
    {
        if (get_partition_locality(id1) == get_partition_locality(id2))
        {
            return dispatch_async(id1, HPX_MOVE(algo), HPX_MOVE(policy), is_seq,
                first1, last1, first2, HPX_MOVE(red_op), HPX_MOVE(conv_op));
        }

        using value_type = typename std::iterator_traits<Iterator2>::value_type;
        std::vector<indexed_partition_range<Iterator2>> ranges = {{0,
            {id2, first2, std::next(first2, std::distance(first1, last1))}}};
        return capture_async<value_type>(id2, HPX_MOVE(ranges))
            .then([id1, algo = HPX_MOVE(algo), policy = HPX_MOVE(policy),
                      is_seq, first1, last1, red_op = HPX_MOVE(red_op),
                      conv_op = HPX_MOVE(conv_op)](
                      hpx::future<collected_partition_values<value_type>>
                          ready) mutable {
                auto captured = get_capture_result<ExPolicy>(HPX_MOVE(ready));
                return dispatch_async(id1, captured_transform_reduce<Algo>(),
                    HPX_MOVE(policy), is_seq, HPX_MOVE(algo), first1, last1,
                    HPX_MOVE(captured.values), HPX_MOVE(red_op),
                    HPX_MOVE(conv_op));
            });
    }

    template <typename Algo, typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename T, typename Reduce, typename Convert,
        typename IsSeq>
    util::detail::algorithm_result_t<ExPolicy, T> segmented_transform_reduce(
        Algo&& algo, ExPolicy const& policy, FwdIter1 first1, FwdIter1 last1,
        FwdIter2 first2, T&& init, Reduce&& red_op, Convert&& conv_op, IsSeq)
    {
        using traits1 = hpx::traits::segmented_iterator_traits<FwdIter1>;
        using traits2 = hpx::traits::segmented_iterator_traits<FwdIter2>;
        using difference_type =
            typename std::iterator_traits<FwdIter1>::difference_type;
        using value_type1 = typename std::iterator_traits<FwdIter1>::value_type;
        using value_type2 = typename std::iterator_traits<FwdIter2>::value_type;
        using result = util::detail::algorithm_result<ExPolicy, T>;
        constexpr std::size_t max_chunk_size =
            max_capture_batch_elements<value_type1, value_type2>();

        auto sit1 = traits1::segment(first1);
        auto send1 = traits1::segment(last1);
        auto sit2 = traits2::segment(first2);
        auto beg1 = traits1::local(first1);
        auto beg2 = traits2::local(first2);
        difference_type remaining =
            static_cast<difference_type>(detail::distance(first1, last1));
        using segment_iterator1 = typename traits1::segment_iterator;
        using segment_iterator2 = typename traits2::segment_iterator;
        using local_iterator1 = typename traits1::local_iterator;
        using local_iterator2 = typename traits2::local_iterator;
        std::vector<std::tuple<segment_iterator1, segment_iterator2,
            local_iterator1, local_iterator1, local_iterator2>>
            chunks;

        while (remaining != 0)
        {
            auto end1 =
                sit1 == send1 ? traits1::local(last1) : traits1::end(sit1);
            auto end2 = traits2::end(sit2);
            difference_type count = (std::min) (static_cast<difference_type>(
                                                    std::distance(beg1, end1)),
                static_cast<difference_type>(std::distance(beg2, end2)));
            count = (std::min) (count,
                static_cast<difference_type>(max_chunk_size));
            auto chunk_end1 = std::next(beg1, count);
            if (count != 0)
            {
                chunks.emplace_back(sit1, sit2, beg1, chunk_end1, beg2);
            }
            beg1 = chunk_end1;
            std::advance(beg2, count);
            remaining -= count;
            if (remaining == 0)
                break;
            if (beg1 == end1)
            {
                ++sit1;
                beg1 = traits1::begin(sit1);
            }
            if (beg2 == end2)
            {
                ++sit2;
                beg2 = traits2::begin(sit2);
            }
        }

        return result::get(hpx::async(
            [chunks = HPX_MOVE(chunks), init = HPX_FORWARD(T, init), policy,
                algo = HPX_FORWARD(Algo, algo),
                red_op = HPX_FORWARD(Reduce, red_op),
                conv_op = HPX_FORWARD(Convert, conv_op)]() mutable -> T {
                std::size_t next = 0;
                while (next != chunks.size())
                {
                    std::vector<hpx::future<T>> values;
                    difference_type captured_elements = 0;
                    while (next != chunks.size())
                    {
                        auto const& chunk = chunks[next];
                        auto id1 = traits1::get_id(std::get<0>(chunk));
                        auto id2 = traits2::get_id(std::get<1>(chunk));
                        bool const remote = get_partition_locality(id1) !=
                            get_partition_locality(id2);
                        difference_type count = remote ?
                            static_cast<difference_type>(std::distance(
                                std::get<2>(chunk), std::get<3>(chunk))) :
                            0;
                        if (!values.empty() &&
                            (IsSeq::value ||
                                count > static_cast<difference_type>(
                                            max_chunk_size) -
                                        captured_elements))
                            break;
                        values.push_back(dispatch_transform_reduce_async(id1,
                            id2, algo, policy, IsSeq(), std::get<2>(chunk),
                            std::get<3>(chunk), std::get<4>(chunk), red_op,
                            conv_op));
                        captured_elements += count;
                        ++next;
                    }
                    (void) hpx::wait_all_nothrow(values);
                    for (auto& value : values)
                        init = HPX_INVOKE(red_op, HPX_MOVE(init),
                            get_capture_result<ExPolicy>(HPX_MOVE(value)));
                }
                return init;
            }));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename SegIter, typename T, typename Reduce,
        typename Convert>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    std::decay_t<T> hpx_invoke(hpx::transform_reduce_t, SegIter first,
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
    hpx_invoke(hpx::transform_reduce_t, ExPolicy&& policy, SegIter first,
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
    T hpx_invoke(hpx::transform_reduce_t, FwdIter1 first1, FwdIter1 last1,
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
    parallel::util::detail::algorithm_result_t<ExPolicy, T> hpx_invoke(
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
