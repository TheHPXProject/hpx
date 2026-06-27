//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>

#include <exception>
#include <iterator>
#include <list>
#include <type_traits>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented_generate

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter, typename F>
    util::detail::algorithm_result_t<ExPolicy, SegIter> segmented_generate(
        Algo&& algo, ExPolicy const& policy, SegIter first, SegIter last, F&& f,
        std::true_type)
    {
        typedef hpx::traits::segmented_iterator_traits<SegIter> traits;
        typedef typename traits::segment_iterator segment_iterator;
        typedef typename traits::local_iterator local_iterator_type;
        typedef util::detail::algorithm_result<ExPolicy, SegIter> result;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                local_iterator_type out = dispatch(traits::get_id(sit), algo,
                    policy, std::true_type(), beg, end, f);
                last = traits::compose(send, out);
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);
            local_iterator_type out = traits::local(last);

            if (beg != end)
            {
                out = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f);
            }

            // handle all of the full partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    out = dispatch(traits::get_id(sit), algo, policy,
                        std::true_type(), beg, end, f);
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                out = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f);
            }

            last = traits::compose(send, out);
        }

        return result::get(HPX_MOVE(last));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter, typename F>
    util::detail::algorithm_result_t<ExPolicy, SegIter> segmented_generate(
        Algo&& algo, ExPolicy const& policy, SegIter first, SegIter last, F&& f,
        std::false_type)
    {
        typedef hpx::traits::segmented_iterator_traits<SegIter> traits;
        typedef typename traits::segment_iterator segment_iterator;
        typedef typename traits::local_iterator local_iterator_type;

        typedef std::integral_constant<bool, !std::forward_iterator<SegIter>>
            forced_seq;
        typedef util::detail::algorithm_result<ExPolicy, SegIter> result;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        std::vector<shared_future<local_iterator_type>> segments;
        segments.reserve(std::distance(sit, send));

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, f));
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
                    policy, forced_seq(), beg, end, f));
            }

            // handle all of the full partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    segments.push_back(dispatch_async(traits::get_id(sit), algo,
                        policy, forced_seq(), beg, end, f));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, f));
            }
        }

        return result::get(dataflow(
            [=](std::vector<hpx::shared_future<local_iterator_type>>&& r)
                -> SegIter {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);
                return traits::compose(send, r.back().get());
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename SegIter, typename F>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    SegIter tag_invoke(hpx::generate_t, SegIter first, SegIter last, F&& f)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        if (first == last)
        {
            return HPX_MOVE(first);
        }

        typedef hpx::traits::segmented_iterator_traits<SegIter> iterator_traits;

        return hpx::parallel::detail::segmented_generate(
            hpx::parallel::detail::generate<
                typename iterator_traits::local_iterator>(),
            hpx::execution::seq, first, last, HPX_FORWARD(F, f),
            std::true_type{});
    }

    HPX_CXX_EXPORT template <typename SegIter, typename Size, typename F>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter> &&
            std::is_integral_v<Size>)
    SegIter tag_invoke(hpx::generate_n_t, SegIter first, Size count, F&& f)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        if (hpx::parallel::detail::is_negative(count) || count == 0)
        {
            return HPX_MOVE(first);
        }

        typedef hpx::traits::segmented_iterator_traits<SegIter> iterator_traits;

        SegIter last = first;
        hpx::parallel::detail::advance(last, count);
        return hpx::parallel::detail::segmented_generate(
            hpx::parallel::detail::generate<
                typename iterator_traits::local_iterator>(),
            hpx::execution::seq, first, last, HPX_FORWARD(F, f),
            std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename F>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    parallel::util::detail::algorithm_result_t<ExPolicy, SegIter> tag_invoke(
        hpx::generate_t, ExPolicy&& policy, SegIter first, SegIter last, F&& f)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        if (first == last)
        {
            return parallel::util::detail::algorithm_result<ExPolicy,
                SegIter>::get(HPX_MOVE(first));
        }

        typedef hpx::traits::segmented_iterator_traits<SegIter> iterator_traits;

        return hpx::parallel::detail::segmented_generate(
            hpx::parallel::detail::generate<
                typename iterator_traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(F, f),
            is_seq());
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename Size,
        typename F>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter> &&
            std::is_integral_v<Size>)
    parallel::util::detail::algorithm_result_t<ExPolicy, SegIter> tag_invoke(
        hpx::generate_n_t, ExPolicy&& policy, SegIter first, Size count, F&& f)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        if (hpx::parallel::detail::is_negative(count) || count == 0)
        {
            return parallel::util::detail::algorithm_result<ExPolicy,
                SegIter>::get(HPX_MOVE(first));
        }

        typedef hpx::traits::segmented_iterator_traits<SegIter> iterator_traits;

        SegIter last = first;
        hpx::parallel::detail::advance(last, count);
        return hpx::parallel::detail::segmented_generate(
            hpx::parallel::detail::generate<
                typename iterator_traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(F, f),
            is_seq());
    }
}    // namespace hpx::segmented
