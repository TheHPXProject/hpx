//  Copyright (c) 2017 Ajai V George
//  Copyright (c) 2025-2026 Hartmut Kaiser
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
#include <hpx/modules/type_support.hpp>

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
    // segmented_adjacent_find

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter, typename Pred,
        typename Proj>
    util::detail::algorithm_result_t<ExPolicy, FwdIter> segmented_adjacent_find(
        Algo&& algo, ExPolicy const& policy, FwdIter first, FwdIter last,
        Pred&& pred, Proj&& proj, std::true_type)
    {
        using traits = hpx::traits::segmented_iterator_traits<FwdIter>;
        using segment_iterator1 = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;

        using result = util::detail::algorithm_result<ExPolicy, FwdIter>;

        segment_iterator1 sit = traits::segment(first);
        segment_iterator1 send = traits::segment(last);

        FwdIter output = last;

        util::invoke_projected<Pred, Proj> pred_projected{pred, proj};

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                local_iterator_type out = dispatch(traits::get_id(sit), algo,
                    policy, std::true_type(), beg, end, pred, proj);
                output = traits::compose(sit, out);
            }
        }
        else
        {
            bool found = false;
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);
            local_iterator_type out = traits::local(last);
            if (beg != end)
            {
                out = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, pred, proj);
                if (out != end)
                {
                    found = true;
                    output = traits::compose(sit, out);
                }
            }
            FwdIter ending = traits::compose(sit, std::prev(end));
            if (!found &&
                HPX_INVOKE(pred_projected, *ending, *std::next(ending)))
            {
                found = true;
                output = traits::compose(sit, std::prev(end));
            }

            // handle all the full partitions
            if (!found)
            {
                for (++sit; sit != send; ++sit)
                {
                    beg = traits::begin(sit);
                    end = traits::end(sit);
                    if (beg != end)
                    {
                        out = dispatch(traits::get_id(sit), algo, policy,
                            std::true_type(), beg, end, pred, proj);
                        if (out != end)
                        {
                            found = true;
                            output = traits::compose(sit, out);
                            break;
                        }
                    }
                    ending = traits::compose(sit, std::prev(end));
                    if (HPX_INVOKE(
                            pred_projected, *ending, *std::next(ending)) &&
                        !found)
                    {
                        found = true;
                        output = traits::compose(sit, std::prev(end));
                        break;
                    }
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end && !found)
            {
                out = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, pred, proj);
                if (out != end)
                {
                    output = traits::compose(sit, out);
                }
            }
        }
        return result::get(HPX_MOVE(output));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter, typename Pred,
        typename Proj>
    util::detail::algorithm_result_t<ExPolicy, FwdIter> segmented_adjacent_find(
        Algo&& algo, ExPolicy const& policy, FwdIter first, FwdIter last,
        Pred&& pred, Proj&& proj, std::false_type)
    {
        using traits = hpx::traits::segmented_iterator_traits<FwdIter>;
        using segment_iterator1 = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;

        using result = util::detail::algorithm_result<ExPolicy, FwdIter>;

        using forced_seq =
            std::integral_constant<bool, !std::forward_iterator<FwdIter>>;

        segment_iterator1 sit = traits::segment(first);
        segment_iterator1 send = traits::segment(last);

        using segment_type = std::vector<future<FwdIter>>;
        segment_type segments;
        segments.reserve(std::distance(sit, send));

        std::vector<FwdIter> between_segments;
        between_segments.reserve(std::distance(sit, send));

        util::invoke_projected<Pred, Proj> pred_projected{pred, proj};

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(hpx::make_future<FwdIter>(
                    dispatch_async(traits::get_id(sit), algo, policy,
                        forced_seq(), beg, end, HPX_FORWARD(Pred, pred),
                        HPX_FORWARD(Proj, proj)),
                    [sit, end, last](
                        local_iterator_type const& out) -> FwdIter {
                        if (out != end)
                            return traits::compose(sit, out);
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
                        forced_seq(), beg, end, pred, proj),
                    [sit, end, last](
                        local_iterator_type const& out) -> FwdIter {
                        if (out != end)
                            return traits::compose(sit, out);
                        return last;
                    }));
            }

            // handle all the full partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    between_segments.push_back(traits::compose(sit, beg));
                    segments.push_back(hpx::make_future<FwdIter>(
                        dispatch_async(traits::get_id(sit), algo, policy,
                            forced_seq(), beg, end, pred, proj),
                        [sit, end, last](
                            local_iterator_type const& out) -> FwdIter {
                            if (out != end)
                                return traits::compose(sit, out);
                            return last;
                        }));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                between_segments.push_back(traits::compose(sit, beg));
                segments.push_back(hpx::make_future<FwdIter>(
                    dispatch_async(traits::get_id(sit), algo, policy,
                        forced_seq(), beg, end, HPX_FORWARD(Pred, pred),
                        HPX_FORWARD(Proj, proj)),
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
            [=](segment_type&& r) mutable -> FwdIter {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);
                std::vector<FwdIter> res = hpx::unwrap(HPX_MOVE(r));
                auto it = res.begin();
                int i = 0;
                while (it != res.end())
                {
                    if (*it != last)
                        return *it;
                    if (HPX_INVOKE(pred_projected,
                            *std::prev(between_segments[i]),
                            *(between_segments[i])))
                    {
                        return std::prev(between_segments[i]);
                    }
                    ++it;
                    i += 1;
                }
                return res.back();
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename InIter, typename Pred>
        requires(hpx::traits::is_iterator_v<InIter> &&
            hpx::traits::is_segmented_iterator_v<InIter>)
    InIter tag_invoke(
        hpx::adjacent_find_t, InIter first, InIter last, Pred&& pred = Pred())
    {
        static_assert(
            (std::input_iterator<InIter>), "Requires at least input iterator.");

        if (first == last)
        {
            return first;
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<InIter>;

        return hpx::parallel::detail::segmented_adjacent_find(
            hpx::parallel::detail::adjacent_find<
                typename iterator_traits::local_iterator,
                typename iterator_traits::local_iterator>(),
            hpx::execution::seq, first, last, HPX_FORWARD(Pred, pred),
            hpx::identity_v, std::true_type());
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename Pred>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, SegIter>
    tag_invoke(hpx::adjacent_find_t, ExPolicy&& policy, SegIter first,
        SegIter last, Pred&& pred)
    {
        static_assert((std::forward_iterator<SegIter>),
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        if (first == last)
        {
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    SegIter>;
            return result::get(HPX_MOVE(first));
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<SegIter>;

        return hpx::parallel::detail::segmented_adjacent_find(
            hpx::parallel::detail::adjacent_find<
                typename iterator_traits::local_iterator,
                typename iterator_traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(Pred, pred),
            hpx::identity_v, is_seq());
    }
}    // namespace hpx::segmented
