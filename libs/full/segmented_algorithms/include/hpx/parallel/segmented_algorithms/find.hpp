//  Copyright (c) 2017 Ajai V George
//  Copyright (c) 2024-2026 Hartmut Kaiser
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
    // segmented_find
    template <typename Algo, typename ExPolicy, typename FwdIter, typename U>
    util::detail::algorithm_result_t<ExPolicy, FwdIter> segmented_find(
        Algo&& algo, ExPolicy&& policy, FwdIter first, FwdIter last,
        U&& f_or_val, std::true_type)
    {
        typedef hpx::traits::segmented_iterator_traits<FwdIter> traits;
        typedef typename traits::segment_iterator segment_iterator;
        typedef typename traits::local_iterator local_iterator_type;
        typedef util::detail::algorithm_result<ExPolicy, FwdIter> result;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);
        FwdIter output = last;
        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                local_iterator_type out = dispatch(traits::get_id(sit), algo,
                    policy, std::true_type(), beg, end, f_or_val);
                output = traits::compose(send, out);
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);
            local_iterator_type out = traits::local(last);
            bool found = false;
            if (beg != end)
            {
                out = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f_or_val);
                if (out != end)
                    found = true;
            }
            if (!found)
            {
                // handle all of the full partitions
                for (++sit; sit != send; ++sit)
                {
                    beg = traits::begin(sit);
                    end = traits::end(sit);
                    out = traits::begin(send);
                    if (beg != end)
                    {
                        out = dispatch(traits::get_id(sit), algo, policy,
                            std::true_type(), beg, end, f_or_val);
                        if (out != end)
                        {
                            found = true;
                            break;
                        }
                    }
                }
            }
            if (!found)
            {
                // handle the beginning of the last partition
                beg = traits::begin(sit);
                end = traits::local(last);
                if (beg != end)
                {
                    out = dispatch(traits::get_id(sit), algo, policy,
                        std::true_type(), beg, end, f_or_val);
                    if (out != end)
                        found = true;
                }
            }
            if (found)
                output = traits::compose(sit, out);
        }
        return result::get(HPX_MOVE(output));
    }

    template <typename Algo, typename ExPolicy, typename FwdIter, typename U>
    util::detail::algorithm_result_t<ExPolicy, FwdIter> segmented_find(
        Algo&& algo, ExPolicy&& policy, FwdIter first, FwdIter last,
        U&& f_or_val, std::false_type)
    {
        typedef hpx::traits::segmented_iterator_traits<FwdIter> traits;
        typedef typename traits::segment_iterator segment_iterator;
        typedef typename traits::local_iterator local_iterator_type;
        typedef util::detail::algorithm_result<ExPolicy, FwdIter> result;

        typedef std::integral_constant<bool, !std::forward_iterator<FwdIter>>
            forced_seq;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        std::vector<future<FwdIter>> segments;
        segments.reserve(std::distance(sit, send));

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(hpx::make_future<FwdIter>(
                    dispatch_async(traits::get_id(sit), algo, policy,
                        forced_seq(), beg, end, f_or_val),
                    [send, end, last](
                        local_iterator_type const& out) -> FwdIter {
                        if (out != end)
                            return traits::compose(send, out);
                        else
                            return last;
                    }));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);
            if (beg != end)
            {
                segments.push_back(hpx::make_future<FwdIter>(
                    dispatch_async(traits::get_id(sit), algo, policy,
                        forced_seq(), beg, end, f_or_val),
                    [sit, end, last](
                        local_iterator_type const& out) -> FwdIter {
                        if (out != end)
                            return traits::compose(sit, out);
                        else
                            return last;
                    }));
            }

            // handle all of the full partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    segments.push_back(hpx::make_future<FwdIter>(
                        dispatch_async(traits::get_id(sit), algo, policy,
                            forced_seq(), beg, end, f_or_val),
                        [sit, end, last](
                            local_iterator_type const& out) -> FwdIter {
                            if (out != end)
                                return traits::compose(sit, out);
                            else
                                return last;
                        }));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(hpx::make_future<FwdIter>(
                    dispatch_async(traits::get_id(sit), algo, policy,
                        forced_seq(), beg, end, f_or_val),
                    [sit, end, last](
                        local_iterator_type const& out) -> FwdIter {
                        if (out != end)
                            return traits::compose(sit, out);
                        else
                            return last;
                    }));
            }
        }
        return result::get(dataflow(
            [=](std::vector<hpx::future<FwdIter>>&& r) -> FwdIter {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);

                std::vector<FwdIter> res = hpx::unwrap(HPX_MOVE(r));
                auto it = res.begin();
                while (it != res.end())
                {
                    if (*it != last)
                        return *it;
                    it++;
                }
                return res.back();
            },
            HPX_MOVE(segments)));
    }
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename SegIter, typename T>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    SegIter tag_invoke(hpx::find_t, SegIter first, SegIter last, T const& val)
    {
        static_assert(
            std::input_iterator<SegIter>, "Requires at least input iterator.");

        if (first == last)
        {
            return first;
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<SegIter>;

        return hpx::parallel::detail::segmented_find(
            hpx::parallel::detail::find<
                typename iterator_traits::local_iterator>(),
            hpx::execution::seq, first, last, val, std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename T>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    parallel::util::detail::algorithm_result_t<ExPolicy, SegIter> tag_invoke(
        hpx::find_t, ExPolicy&& policy, SegIter first, SegIter last,
        T const& val)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        if (first == last)
        {
            return parallel::util::detail::algorithm_result<ExPolicy,
                SegIter>::get(HPX_FORWARD(SegIter, first));
        }

        typedef hpx::traits::segmented_iterator_traits<SegIter> iterator_traits;

        return hpx::parallel::detail::segmented_find(
            hpx::parallel::detail::find<
                typename iterator_traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last, val, is_seq());
    }

    HPX_CXX_EXPORT template <typename FwdIter, typename F>
        requires(hpx::traits::is_iterator_v<FwdIter> &&
            hpx::traits::is_segmented_iterator_v<FwdIter>)
    FwdIter tag_invoke(hpx::find_if_t, FwdIter first, FwdIter last, F&& f)
    {
        static_assert(
            std::input_iterator<FwdIter>, "Requires at least input iterator.");

        if (first == last)
        {
            return first;
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<FwdIter>;

        return hpx::parallel::detail::segmented_find(
            hpx::parallel::detail::find_if<
                typename iterator_traits::local_iterator>(),
            hpx::execution::seq, first, last, HPX_FORWARD(F, f),
            std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename FwdIter, typename F>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter> &&
            hpx::traits::is_segmented_iterator_v<FwdIter>)
    parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter> tag_invoke(
        hpx::find_if_t, ExPolicy&& policy, FwdIter first, FwdIter last, F&& f)
    {
        static_assert(std::forward_iterator<FwdIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        if (first == last)
        {
            return parallel::util::detail::algorithm_result<ExPolicy,
                FwdIter>::get(HPX_FORWARD(FwdIter, first));
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<FwdIter>;

        return hpx::parallel::detail::segmented_find(
            hpx::parallel::detail::find_if<
                typename iterator_traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(F, f),
            is_seq());
    }

    HPX_CXX_EXPORT template <typename FwdIter, typename F>
        requires(hpx::traits::is_iterator_v<FwdIter> &&
            hpx::traits::is_segmented_iterator_v<FwdIter>)
    FwdIter tag_invoke(hpx::find_if_not_t, FwdIter first, FwdIter last, F&& f)
    {
        static_assert(
            std::input_iterator<FwdIter>, "Requires at least input iterator.");

        if (first == last)
        {
            return first;
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<FwdIter>;

        return hpx::parallel::detail::segmented_find(
            hpx::parallel::detail::find_if_not<
                typename iterator_traits::local_iterator>(),
            hpx::execution::seq, first, last, HPX_FORWARD(F, f),
            std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename FwdIter, typename F>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter> &&
            hpx::traits::is_segmented_iterator_v<FwdIter>)
    parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter> tag_invoke(
        hpx::find_if_not_t, ExPolicy&& policy, FwdIter first, FwdIter last,
        F&& f)
    {
        static_assert(std::forward_iterator<FwdIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        if (first == last)
        {
            return parallel::util::detail::algorithm_result<ExPolicy,
                FwdIter>::get(HPX_FORWARD(FwdIter, first));
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<FwdIter>;

        return hpx::parallel::detail::segmented_find(
            hpx::parallel::detail::find_if_not<
                typename iterator_traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(F, f),
            is_seq());
    }
}    // namespace hpx::segmented
