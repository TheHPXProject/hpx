//  Copyright (c) 2017 Ajai V George
//  Copyright (c) 2022-2026 Hartmut Kaiser
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
    // segmented_reduce

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename SegIterB,
        typename SegIterE, typename T, typename Reduce>
    util::detail::algorithm_result_t<ExPolicy, T> segmented_reduce(Algo&& algo,
        ExPolicy const& policy, SegIterB first, SegIterE last, T&& init,
        Reduce&& red_op, std::true_type)
    {
        typedef hpx::traits::segmented_iterator_traits<SegIterB> traits;
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
                        std::true_type(), beg, end, red_op));
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
                        std::true_type(), beg, end, red_op));
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
                            std::true_type(), beg, end, red_op));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                overall_result = HPX_INVOKE(red_op, overall_result,
                    dispatch(traits::get_id(sit), algo, policy,
                        std::true_type(), beg, end, red_op));
            }
        }

        return result::get(HPX_MOVE(overall_result));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename SegIterB,
        typename SegIterE, typename T, typename Reduce>
    util::detail::algorithm_result_t<ExPolicy, T> segmented_reduce(Algo&& algo,
        ExPolicy const& policy, SegIterB first, SegIterE last, T&& init,
        Reduce&& red_op, std::false_type)
    {
        typedef hpx::traits::segmented_iterator_traits<SegIterB> traits;
        typedef typename traits::segment_iterator segment_iterator;
        typedef typename traits::local_iterator local_iterator_type;
        typedef util::detail::algorithm_result<ExPolicy, T> result;

        typedef std::integral_constant<bool, !std::forward_iterator<SegIterB>>
            forced_seq;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        std::vector<shared_future<T>> segments;
        segments.reserve(detail::distance(sit, send));

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, red_op));
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
                    policy, forced_seq(), beg, end, red_op));
            }

            // handle all of the full partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    segments.push_back(dispatch_async(traits::get_id(sit), algo,
                        policy, forced_seq(), beg, end, red_op));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, red_op));
            }
        }

        return result::get(dataflow(
            [=](std::vector<shared_future<T>>&& r) mutable -> T {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);

                // VS2015RC bails out if red_op is capture by ref
                return detail::accumulate(r.begin(), r.end(), init,
                    [=](T val, shared_future<T>& curr) mutable {
                        return HPX_INVOKE(red_op, HPX_MOVE(val), curr.get());
                    });
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx ::segmented {

    HPX_CXX_EXPORT template <typename InIterB, typename InIterE, typename T,
        typename F>
        requires(hpx::traits::is_iterator_v<InIterB> &&
            hpx::traits::is_segmented_iterator_v<InIterB> &&
            hpx::traits::is_iterator_v<InIterE> &&
            hpx::traits::is_segmented_iterator_v<InIterE>)
    T tag_invoke(hpx::reduce_t, InIterB first, InIterE last, T init, F&& f)
    {
        static_assert(
            std::input_iterator<InIterB>, "Requires at least input iterator.");

        static_assert(
            std::input_iterator<InIterE>, "Requires at least input iterator.");

        if (first == last)
        {
            return init;
        }

        return hpx::parallel::detail::segmented_reduce(
            hpx::parallel::detail::seg_reduce<T>(), hpx::execution::seq, first,
            last, HPX_FORWARD(T, init), HPX_FORWARD(F, f), std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename InIterB,
        typename InIterE, typename T, typename F>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<InIterB> &&
            hpx::traits::is_segmented_iterator_v<InIterB> &&
            hpx::traits::is_iterator_v<InIterE> &&
            hpx::traits::is_segmented_iterator_v<InIterE>)
    parallel::util::detail::algorithm_result_t<ExPolicy, T> tag_invoke(
        hpx::reduce_t, ExPolicy&& policy, InIterB first, InIterE last, T init,
        F&& f)
    {
        static_assert(
            std::input_iterator<InIterB>, "Requires at least input iterator.");

        static_assert(
            std::input_iterator<InIterE>, "Requires at least input iterator.");

        if (first == last)
        {
            return parallel::util::detail::algorithm_result<ExPolicy, T>::get(
                HPX_FORWARD(T, init));
        }

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        return hpx::parallel::detail::segmented_reduce(
            hpx::parallel::detail::seg_reduce<T>(),
            HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(T, init),
            HPX_FORWARD(F, f), is_seq());
    }
}    // namespace hpx::segmented
