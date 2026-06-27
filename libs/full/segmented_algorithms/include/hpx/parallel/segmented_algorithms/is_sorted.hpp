//  Copyright (c) 2026 Mo'men Samir
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
    // segmented_is_sorted_until

    ///////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter, typename Pred,
        typename Proj>
    util::detail::algorithm_result_t<ExPolicy, SegIter>
    segmented_is_sorted_until(Algo&& algo, ExPolicy const& policy,
        SegIter first, SegIter last, Pred&& pred, Proj&& proj, std::true_type)
    {
        using traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;
        using result = util::detail::algorithm_result<ExPolicy, SegIter>;

        util::invoke_projected<Pred, Proj> pred_projected{pred, proj};

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
                    policy, std::true_type(), beg, end, pred, proj);
                last = traits::compose(sit, out);
            }
        }
        else
        {
            bool last_found = false;

            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);

            if (beg != end)
            {
                local_iterator_type out = dispatch(traits::get_id(sit), algo,
                    policy, std::true_type(), beg, end, pred, proj);

                if (out != end)
                {
                    last_found = true;
                    last = traits::compose(sit, out);
                }

                if (!last_found)
                {
                    SegIter boundary = traits::compose(sit, std::prev(end));
                    SegIter next_elem = std::next(boundary);
                    if (next_elem != last &&
                        HPX_INVOKE(pred_projected, *next_elem, *boundary))
                    {
                        last_found = true;
                        last = next_elem;
                    }
                }
            }

            // handle all the full partitions
            for (++sit; sit != send && !last_found; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);

                if (beg != end)
                {
                    local_iterator_type out = dispatch(traits::get_id(sit),
                        algo, policy, std::true_type(), beg, end, pred, proj);

                    if (out != end)
                    {
                        last_found = true;
                        last = traits::compose(sit, out);
                        break;
                    }

                    SegIter boundary = traits::compose(sit, std::prev(end));
                    SegIter next_elem = std::next(boundary);
                    if (next_elem != last &&
                        HPX_INVOKE(pred_projected, *next_elem, *boundary))
                    {
                        last_found = true;
                        last = next_elem;
                        break;
                    }
                }
            }
            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end && !last_found)
            {
                local_iterator_type out = dispatch(traits::get_id(sit), algo,
                    policy, std::true_type(), beg, end, pred, proj);
                if (out != end)
                {
                    last = traits::compose(sit, out);
                }
            }
        }

        return result::get(HPX_MOVE(last));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter, typename Pred,
        typename Proj>
    util::detail::algorithm_result_t<ExPolicy, SegIter>
    segmented_is_sorted_until(Algo&& algo, ExPolicy const& policy,
        SegIter first, SegIter last, Pred&& pred, Proj&& proj, std::false_type)
    {
        using traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;
        using result = util::detail::algorithm_result<ExPolicy, SegIter>;

        using forced_seq =
            std::integral_constant<bool, !std::forward_iterator<SegIter>>;

        using segment_type = std::vector<hpx::future<SegIter>>;

        util::invoke_projected<Pred, Proj> pred_projected{pred, proj};

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        auto const size = std::distance(sit, send);

        segment_type segments;
        segments.reserve(size);

        std::vector<SegIter> between_segments;
        between_segments.reserve(size);

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(hpx::make_future<SegIter>(
                    dispatch_async(traits::get_id(sit), algo, policy,
                        forced_seq(), beg, end, pred, proj),
                    [sit, end, last](
                        local_iterator_type const& out) -> SegIter {
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
                segments.push_back(hpx::make_future<SegIter>(
                    dispatch_async(traits::get_id(sit), algo, policy,
                        forced_seq(), beg, end, pred, proj),
                    [sit, end, last](
                        local_iterator_type const& out) -> SegIter {
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
                    segments.push_back(hpx::make_future<SegIter>(
                        dispatch_async(traits::get_id(sit), algo, policy,
                            forced_seq(), beg, end, pred, proj),
                        [sit, end, last](
                            local_iterator_type const& out) -> SegIter {
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
                segments.push_back(hpx::make_future<SegIter>(
                    dispatch_async(traits::get_id(sit), algo, policy,
                        forced_seq(), beg, end, HPX_FORWARD(Pred, pred),
                        HPX_FORWARD(Proj, proj)),
                    [sit, end, last](
                        local_iterator_type const& out) -> SegIter {
                        if (out != end)
                            return traits::compose(sit, out);
                        return last;
                    }));
            }
        }

        return result::get(dataflow(
            [=](segment_type&& r) mutable -> SegIter {
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);
                std::vector<SegIter> res = hpx::unwrap(HPX_MOVE(r));

                auto it = res.begin();
                std::size_t i = 0;
                while (it != res.end())
                {
                    if (*it != last)
                        return *it;

                    if (i < between_segments.size())
                    {
                        if (HPX_INVOKE(pred_projected, *between_segments[i],
                                *std::prev(between_segments[i])))
                        {
                            return between_segments[i];
                        }
                    }
                    ++it;
                    ++i;
                }
                return last;
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename InIter,
        typename Pred = hpx::parallel::detail::less>
        requires(hpx::traits::is_iterator_v<InIter> &&
            hpx::traits::is_segmented_iterator_v<InIter>)
    InIter tag_invoke(
        hpx::is_sorted_until_t, InIter first, InIter last, Pred&& pred = Pred())
    {
        static_assert(std::forward_iterator<InIter>,
            "Requires at least forward iterator.");

        if (first == last)
            return first;

        using iterator_traits = hpx::traits::segmented_iterator_traits<InIter>;

        return hpx::parallel::detail::segmented_is_sorted_until(
            hpx::parallel::detail::is_sorted_until<
                typename iterator_traits::local_iterator,
                typename iterator_traits::local_iterator>(),
            hpx::execution::seq, first, last, HPX_FORWARD(Pred, pred),
            hpx::identity_v, std::true_type());
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter,
        typename Pred = hpx::parallel::detail::less>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, SegIter>
    tag_invoke(hpx::is_sorted_until_t, ExPolicy&& policy, SegIter first,
        SegIter last, Pred&& pred = Pred())
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        if (first == last)
        {
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    SegIter>;
            return result::get(HPX_MOVE(first));
        }

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
        using iterator_traits = hpx::traits::segmented_iterator_traits<SegIter>;

        return hpx::parallel::detail::segmented_is_sorted_until(
            hpx::parallel::detail::is_sorted_until<
                typename iterator_traits::local_iterator,
                typename iterator_traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(Pred, pred),
            hpx::identity_v, is_seq());
    }

    HPX_CXX_EXPORT template <typename InIter,
        typename Pred = hpx::parallel::detail::less>
        requires(hpx::traits::is_iterator_v<InIter> &&
            hpx::traits::is_segmented_iterator_v<InIter>)
    bool tag_invoke(
        hpx::is_sorted_t, InIter first, InIter last, Pred&& pred = Pred())
    {
        static_assert(std::forward_iterator<InIter>,
            "Requires at least forward iterator.");

        if (first == last)
            return true;

        using iterator_traits = hpx::traits::segmented_iterator_traits<InIter>;

        return hpx::parallel::detail::segmented_is_sorted_until(
                   hpx::parallel::detail::is_sorted_until<
                       typename iterator_traits::local_iterator,
                       typename iterator_traits::local_iterator>(),
                   hpx::execution::seq, first, last, HPX_FORWARD(Pred, pred),
                   hpx::identity_v, std::true_type()) == last;
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter,
        typename Pred = hpx::parallel::detail::less>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, bool> tag_invoke(
        hpx::is_sorted_t, ExPolicy&& policy, SegIter first, SegIter last,
        Pred&& pred = Pred())
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        if (first == last)
        {
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy, bool>;
            return result::get(true);
        }

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
        using iterator_traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using local_iter = typename iterator_traits::local_iterator;

        auto until_result = hpx::parallel::detail::segmented_is_sorted_until(
            hpx::parallel::detail::is_sorted_until<local_iter, local_iter>(),
            HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(Pred, pred),
            hpx::identity_v, is_seq());

        if constexpr (hpx::traits::is_future_v<decltype(until_result)>)
        {
            return until_result.then(
                [last](auto&& f) -> bool { return f.get() == last; });
        }
        else
        {
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy, bool>;
            return result::get(until_result == last);
        }
    }
}    // namespace hpx::segmented
