//  Copyright (c) 2026 Abir Roy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>

#include <algorithm>
#include <cstddef>
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
    // mismatch

    /// \cond NOINTERNAL

    template <typename Pred>
    struct mismatch_distance_algo
      : public algorithm<mismatch_distance_algo<Pred>,
            std::pair<std::size_t, std::size_t>>
    {
        constexpr mismatch_distance_algo() noexcept
          : algorithm<mismatch_distance_algo,
                std::pair<std::size_t, std::size_t>>("mismatch_distance_algo")
        {
        }

        template <typename ExPolicy, typename Iter1, typename Sent1,
            typename Iter2>
        static std::pair<std::size_t, std::size_t> sequential(
            ExPolicy, Iter1 first1, Sent1 last1, Iter2 first2, Pred pred)
        {
            auto res =
                hpx::parallel::detail::mismatch<std::pair<Iter1, Iter2>>()
                    .sequential(
                        hpx::execution::seq, first1, last1, first2, pred);
            return std::make_pair(std::distance(first1, res.first),
                std::distance(first2, res.second));
        }

        template <typename ExPolicy, typename Iter1, typename Sent1,
            typename Iter2>
        static auto parallel(ExPolicy&& policy, Iter1 first1, Sent1 last1,
            Iter2 first2, Pred pred)
        {
            auto res =
                hpx::parallel::detail::mismatch<std::pair<Iter1, Iter2>>()
                    .parallel(HPX_FORWARD(ExPolicy, policy), first1, last1,
                        first2, pred);

            using res_t = std::decay_t<decltype(res)>;
            if constexpr (hpx::traits::is_future_v<res_t>)
            {
                return hpx::dataflow(
                    [first1, first2](
                        auto&& f) -> std::pair<std::size_t, std::size_t> {
                        auto p = f.get();
                        return std::make_pair(std::distance(first1, p.first),
                            std::distance(first2, p.second));
                    },
                    HPX_MOVE(res));
            }
            else
            {
                return std::make_pair(std::distance(first1, res.first),
                    std::distance(first2, res.second));
            }
        }
    };

    // sequential remote implementation
    template <typename ExPolicy, typename SegIter1, typename SegIter2,
        typename Pred>
    util::detail::algorithm_result_t<ExPolicy, std::pair<SegIter1, SegIter2>>
    segmented_mismatch(ExPolicy const& policy, SegIter1 first1, SegIter1 last1,
        SegIter2 first2, Pred&& pred, std::true_type)
    {
        typedef hpx::traits::segmented_iterator_traits<SegIter1> traits1;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;

        typedef hpx::traits::segmented_iterator_traits<SegIter2> traits2;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;

        segment_iterator1 sit1 = traits1::segment(first1);
        segment_iterator1 send1 = traits1::segment(last1);
        segment_iterator2 sit2 = traits2::segment(first2);

        using result = util::detail::algorithm_result<ExPolicy,
            std::pair<SegIter1, SegIter2>>;

        mismatch_distance_algo<std::decay_t<Pred>> algo;

        if (sit1 == send1)
        {
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::local(last1);
            local_iterator_type2 beg2 = traits2::local(first2);

            if (beg1 != end1)
            {
                std::pair<std::size_t, std::size_t> local_dist =
                    dispatch(traits1::get_id(sit1), algo, policy,
                        std::true_type(), beg1, end1, beg2, pred);

                local_iterator_type1 res1 = beg1;
                std::advance(res1, local_dist.first);
                local_iterator_type2 res2 = beg2;
                std::advance(res2, local_dist.second);

                return result::get(std::make_pair(traits1::compose(sit1, res1),
                    traits2::compose(sit2, res2)));
            }
        }
        else
        {
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::end(sit1);
            local_iterator_type2 beg2 = traits2::local(first2);

            if (beg1 != end1)
            {
                std::pair<std::size_t, std::size_t> local_dist =
                    dispatch(traits1::get_id(sit1), algo, policy,
                        std::true_type(), beg1, end1, beg2, pred);

                local_iterator_type1 res1 = beg1;
                std::advance(res1, local_dist.first);
                local_iterator_type2 res2 = beg2;
                std::advance(res2, local_dist.second);

                if (res1 != end1)
                {
                    return result::get(
                        std::make_pair(traits1::compose(sit1, res1),
                            traits2::compose(sit2, res2)));
                }
            }

            for (++sit1, ++sit2; sit1 != send1; ++sit1, ++sit2)
            {
                beg1 = traits1::begin(sit1);
                end1 = traits1::end(sit1);
                beg2 = traits2::begin(sit2);

                if (beg1 != end1)
                {
                    std::pair<std::size_t, std::size_t> local_dist =
                        dispatch(traits1::get_id(sit1), algo, policy,
                            std::true_type(), beg1, end1, beg2, pred);

                    local_iterator_type1 res1 = beg1;
                    std::advance(res1, local_dist.first);
                    local_iterator_type2 res2 = beg2;
                    std::advance(res2, local_dist.second);

                    if (res1 != end1)
                    {
                        return result::get(
                            std::make_pair(traits1::compose(sit1, res1),
                                traits2::compose(sit2, res2)));
                    }
                }
            }

            beg1 = traits1::begin(sit1);
            end1 = traits1::local(last1);
            beg2 = traits2::begin(sit2);

            if (beg1 != end1)
            {
                std::pair<std::size_t, std::size_t> local_dist =
                    dispatch(traits1::get_id(sit1), algo, policy,
                        std::true_type(), beg1, end1, beg2, pred);

                local_iterator_type1 res1 = beg1;
                std::advance(res1, local_dist.first);
                local_iterator_type2 res2 = beg2;
                std::advance(res2, local_dist.second);

                if (res1 != end1)
                {
                    return result::get(
                        std::make_pair(traits1::compose(sit1, res1),
                            traits2::compose(sit2, res2)));
                }
            }
        }

        auto dist = detail::distance(first1, last1);
        SegIter2 expected_last2 = first2;
        detail::advance(expected_last2, dist);
        return result::get(std::make_pair(last1, expected_last2));
    }

    // parallel remote implementation
    template <typename ExPolicy, typename SegIter1, typename SegIter2,
        typename Pred>
    util::detail::algorithm_result_t<ExPolicy, std::pair<SegIter1, SegIter2>>
    segmented_mismatch(ExPolicy const& policy, SegIter1 first1, SegIter1 last1,
        SegIter2 first2, Pred&& pred, std::false_type)
    {
        typedef hpx::traits::segmented_iterator_traits<SegIter1> traits1;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;

        typedef hpx::traits::segmented_iterator_traits<SegIter2> traits2;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;

        typedef std::integral_constant<bool, !std::forward_iterator<SegIter1>>
            forced_seq;

        using result = util::detail::algorithm_result<ExPolicy,
            std::pair<SegIter1, SegIter2>>;

        segment_iterator1 sit1 = traits1::segment(first1);
        segment_iterator1 send1 = traits1::segment(last1);
        segment_iterator2 sit2 = traits2::segment(first2);

        std::vector<hpx::future<std::pair<std::size_t, std::size_t>>> segments;
        segments.reserve(detail::distance(sit1, send1));

        std::vector<std::tuple<segment_iterator1, segment_iterator2,
            local_iterator_type1, local_iterator_type2, local_iterator_type1>>
            seg_data;
        seg_data.reserve(detail::distance(sit1, send1));

        mismatch_distance_algo<std::decay_t<Pred>> algo;

        if (sit1 == send1)
        {
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::local(last1);
            local_iterator_type2 beg2 = traits2::local(first2);

            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, pred));
                seg_data.emplace_back(sit1, sit2, beg1, beg2, end1);
            }
        }
        else
        {
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::end(sit1);
            local_iterator_type2 beg2 = traits2::local(first2);

            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, pred));
                seg_data.emplace_back(sit1, sit2, beg1, beg2, end1);
            }

            for (++sit1, ++sit2; sit1 != send1; ++sit1, ++sit2)
            {
                beg1 = traits1::begin(sit1);
                end1 = traits1::end(sit1);
                beg2 = traits2::begin(sit2);

                if (beg1 != end1)
                {
                    segments.push_back(dispatch_async(traits1::get_id(sit1),
                        algo, policy, forced_seq(), beg1, end1, beg2, pred));
                    seg_data.emplace_back(sit1, sit2, beg1, beg2, end1);
                }
            }

            beg1 = traits1::begin(sit1);
            end1 = traits1::local(last1);
            beg2 = traits2::begin(sit2);

            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, pred));
                seg_data.emplace_back(sit1, sit2, beg1, beg2, end1);
            }
        }

        auto dist = detail::distance(first1, last1);
        SegIter2 expected_last2 = first2;
        detail::advance(expected_last2, dist);

        return result::get(dataflow(
            [seg_data = HPX_MOVE(seg_data), last1, expected_last2](
                std::vector<hpx::future<std::pair<std::size_t, std::size_t>>>&&
                    r_futures) mutable -> std::pair<SegIter1, SegIter2> {
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r_futures, errors);

                std::vector<std::pair<std::size_t, std::size_t>> r =
                    hpx::unwrap(HPX_MOVE(r_futures));

                for (std::size_t i = 0; i < r.size(); ++i)
                {
                    auto& data = seg_data[i];
                    local_iterator_type1 res1 = std::get<2>(data);
                    std::advance(res1, r[i].first);
                    local_iterator_type2 res2 = std::get<3>(data);
                    std::advance(res2, r[i].second);
                    local_iterator_type1 end1 = std::get<4>(data);

                    if (res1 != end1)
                    {
                        return std::make_pair(
                            traits1::compose(std::get<0>(data), res1),
                            traits2::compose(std::get<1>(data), res2));
                    }
                }
                return std::make_pair(last1, expected_last2);
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename SegIter1, typename SegIter2,
        typename Pred = hpx::parallel::detail::equal_to>
        requires(hpx::traits::is_iterator_v<SegIter1> &&
            hpx::traits::is_segmented_iterator_v<SegIter1> &&
            hpx::traits::is_iterator_v<SegIter2> &&
            hpx::traits::is_segmented_iterator_v<SegIter2>)
    std::pair<SegIter1, SegIter2> tag_invoke(hpx::mismatch_t, SegIter1 first1,
        SegIter1 last1, SegIter2 first2, Pred&& pred = Pred())
    {
        static_assert(std::forward_iterator<SegIter1>,
            "Requires at least forward iterator to First Vector");

        static_assert(std::forward_iterator<SegIter2>,
            "Requires at least forward iterator to Second Vector");

        if (first1 == last1)
            return std::make_pair(first1, first2);

        return hpx::parallel::detail::segmented_mismatch(hpx::execution::seq,
            first1, last1, first2, HPX_FORWARD(Pred, pred), std::true_type());
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter1,
        typename SegIter2, typename Pred = hpx::parallel::detail::equal_to>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter1> &&
            hpx::traits::is_segmented_iterator_v<SegIter1> &&
            hpx::traits::is_iterator_v<SegIter2> &&
            hpx::traits::is_segmented_iterator_v<SegIter2>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        std::pair<SegIter1, SegIter2>>
    tag_invoke(hpx::mismatch_t, ExPolicy&& policy, SegIter1 first1,
        SegIter1 last1, SegIter2 first2, Pred&& pred = Pred())
    {
        static_assert(std::forward_iterator<SegIter1>,
            "Requires at least forward iterator to First Vector");

        static_assert(std::forward_iterator<SegIter2>,
            "Requires at least forward iterator to Second Vector");

        using result = hpx::parallel::util::detail::algorithm_result<ExPolicy,
            std::pair<SegIter1, SegIter2>>;

        if (first1 == last1)
        {
            return result::get(std::make_pair(first1, first2));
        }

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        return hpx::parallel::detail::segmented_mismatch(
            HPX_FORWARD(ExPolicy, policy), first1, last1, first2,
            HPX_FORWARD(Pred, pred), is_seq());
    }

    HPX_CXX_EXPORT template <typename SegIter1, typename SegIter2,
        typename Pred = hpx::parallel::detail::equal_to>
        requires(hpx::traits::is_iterator_v<SegIter1> &&
            hpx::traits::is_segmented_iterator_v<SegIter1> &&
            hpx::traits::is_iterator_v<SegIter2> &&
            hpx::traits::is_segmented_iterator_v<SegIter2>)
    std::pair<SegIter1, SegIter2> tag_invoke(hpx::mismatch_t, SegIter1 first1,
        SegIter1 last1, SegIter2 first2, SegIter2 last2, Pred&& pred = Pred())
    {
        static_assert(std::forward_iterator<SegIter1>,
            "Requires at least forward iterator to First Vector");

        static_assert(std::forward_iterator<SegIter2>,
            "Requires at least forward iterator to Second Vector");

        auto dist1 = hpx::parallel::detail::distance(first1, last1);
        auto dist2 = hpx::parallel::detail::distance(first2, last2);

        if (dist1 > dist2)
        {
            last1 = first1;
            hpx::parallel::detail::advance(last1, dist2);
        }

        if (first1 == last1)
            return std::make_pair(first1, first2);

        return hpx::parallel::detail::segmented_mismatch(hpx::execution::seq,
            first1, last1, first2, HPX_FORWARD(Pred, pred), std::true_type());
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter1,
        typename SegIter2, typename Pred = hpx::parallel::detail::equal_to>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter1> &&
            hpx::traits::is_segmented_iterator_v<SegIter1> &&
            hpx::traits::is_iterator_v<SegIter2> &&
            hpx::traits::is_segmented_iterator_v<SegIter2>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        std::pair<SegIter1, SegIter2>>
    tag_invoke(hpx::mismatch_t, ExPolicy&& policy, SegIter1 first1,
        SegIter1 last1, SegIter2 first2, SegIter2 last2, Pred&& pred = Pred())
    {
        static_assert(std::forward_iterator<SegIter1>,
            "Requires at least forward iterator to First Vector");

        static_assert(std::forward_iterator<SegIter2>,
            "Requires at least forward iterator to Second Vector");

        auto dist1 = hpx::parallel::detail::distance(first1, last1);
        auto dist2 = hpx::parallel::detail::distance(first2, last2);

        if (dist1 > dist2)
        {
            last1 = first1;
            hpx::parallel::detail::advance(last1, dist2);
        }

        using result = hpx::parallel::util::detail::algorithm_result<ExPolicy,
            std::pair<SegIter1, SegIter2>>;

        if (first1 == last1)
        {
            return result::get(std::make_pair(first1, first2));
        }

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        return hpx::parallel::detail::segmented_mismatch(
            HPX_FORWARD(ExPolicy, policy), first1, last1, first2,
            HPX_FORWARD(Pred, pred), is_seq());
    }
}    // namespace hpx::segmented
