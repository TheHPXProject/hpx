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
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented_all_any_none

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter, typename F,
        typename Proj>
    util::detail::algorithm_result_t<ExPolicy, bool> segmented_none_of(
        Algo&& algo, ExPolicy const& policy, FwdIter first, FwdIter last, F&& f,
        Proj&& proj, std::true_type)
    {
        using traits = hpx::traits::segmented_iterator_traits<FwdIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;
        using result = util::detail::algorithm_result<ExPolicy, bool>;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);
        bool output = false;
        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                output = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f, proj);
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);

            if (beg != end)
            {
                output = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f, proj);
            }

            // handle all of the full partitions
            for (++sit; sit != send && output; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);

                if (beg != end)
                {
                    output = dispatch(traits::get_id(sit), algo, policy,
                        std::true_type(), beg, end, f, proj);
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end && output)
            {
                output = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f, proj);
            }
        }

        return result::get(HPX_MOVE(output));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter, typename F,
        typename Proj>
    util::detail::algorithm_result_t<ExPolicy, bool> segmented_none_of(
        Algo&& algo, ExPolicy const& policy, FwdIter first, FwdIter last, F&& f,
        Proj&& proj, std::false_type)
    {
        using traits = hpx::traits::segmented_iterator_traits<FwdIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;
        using result = util::detail::algorithm_result<ExPolicy, bool>;

        using forced_seq =
            std::integral_constant<bool, !std::forward_iterator<FwdIter>>;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        std::vector<shared_future<bool>> segments;
        segments.reserve(std::distance(sit, send));

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, f, proj));
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
                    policy, forced_seq(), beg, end, f, proj));
            }

            // handle all of the full partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    segments.push_back(dispatch_async(traits::get_id(sit), algo,
                        policy, forced_seq(), beg, end, f, proj));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, f, proj));
            }
        }

        return result::get(dataflow(
            [=](std::vector<shared_future<bool>>&& r) -> bool {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);
                std::vector<bool> res = hpx::unwrap(HPX_MOVE(r));
                auto it = res.begin();
                while (it != res.end())
                {
                    if (*it == false)
                        return false;
                    it++;
                }
                return true;
            },
            HPX_MOVE(segments)));
    }

    ///////////////////////////////////////////////////////////////////////
    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter, typename F,
        typename Proj>
    util::detail::algorithm_result_t<ExPolicy, bool> segmented_any_of(
        Algo&& algo, ExPolicy const& policy, FwdIter first, FwdIter last, F&& f,
        Proj&& proj, std::true_type)
    {
        using traits = hpx::traits::segmented_iterator_traits<FwdIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;
        using result = util::detail::algorithm_result<ExPolicy, bool>;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);
        bool output = false;
        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                output = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f, proj);
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);

            if (beg != end)
            {
                output = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f, proj);
            }

            // handle all of the full partitions
            for (++sit; sit != send && !output; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);

                if (beg != end)
                {
                    output = dispatch(traits::get_id(sit), algo, policy,
                        std::true_type(), beg, end, f, proj);
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end && !output)
            {
                output = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f, proj);
            }
        }

        return result::get(HPX_MOVE(output));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter, typename F,
        typename Proj>
    util::detail::algorithm_result_t<ExPolicy, bool> segmented_any_of(
        Algo&& algo, ExPolicy const& policy, FwdIter first, FwdIter last, F&& f,
        Proj&& proj, std::false_type)
    {
        using traits = hpx::traits::segmented_iterator_traits<FwdIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;
        using result = util::detail::algorithm_result<ExPolicy, bool>;

        using forced_seq =
            std::integral_constant<bool, !std::forward_iterator<FwdIter>>;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        std::vector<shared_future<bool>> segments;
        segments.reserve(std::distance(sit, send));

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, f, proj));
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
                    policy, forced_seq(), beg, end, f, proj));
            }

            // handle all of the full partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    segments.push_back(dispatch_async(traits::get_id(sit), algo,
                        policy, forced_seq(), beg, end, f, proj));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, f, proj));
            }
        }

        return result::get(dataflow(
            [=](std::vector<shared_future<bool>>&& r) -> bool {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);
                std::vector<bool> res = hpx::unwrap(HPX_MOVE(r));
                auto it = res.begin();
                while (it != res.end())
                {
                    if (*it == true)
                        return true;
                    it++;
                }
                return false;
            },
            HPX_MOVE(segments)));
    }

    ///////////////////////////////////////////////////////////////////////
    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter, typename F,
        typename Proj>
    util::detail::algorithm_result_t<ExPolicy, bool> segmented_all_of(
        Algo&& algo, ExPolicy const& policy, FwdIter first, FwdIter last, F&& f,
        Proj&& proj, std::true_type)
    {
        using traits = hpx::traits::segmented_iterator_traits<FwdIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;
        using result = util::detail::algorithm_result<ExPolicy, bool>;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);
        bool output = false;
        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                output = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f, proj);
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);

            if (beg != end)
            {
                output = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f, proj);
            }

            // handle all of the full partitions
            for (++sit; sit != send && output; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);

                if (beg != end)
                {
                    output = dispatch(traits::get_id(sit), algo, policy,
                        std::true_type(), beg, end, f, proj);
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end && output)
            {
                output = dispatch(traits::get_id(sit), algo, policy,
                    std::true_type(), beg, end, f, proj);
            }
        }

        return result::get(HPX_MOVE(output));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter, typename F,
        typename Proj>
    util::detail::algorithm_result_t<ExPolicy, bool> segmented_all_of(
        Algo&& algo, ExPolicy const& policy, FwdIter first, FwdIter last, F&& f,
        Proj&& proj, std::false_type)
    {
        using traits = hpx::traits::segmented_iterator_traits<FwdIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;
        using result = util::detail::algorithm_result<ExPolicy, bool>;

        using forced_seq =
            std::integral_constant<bool, !std::forward_iterator<FwdIter>>;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        std::vector<shared_future<bool>> segments;
        segments.reserve(std::distance(sit, send));

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, f, proj));
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
                    policy, forced_seq(), beg, end, f, proj));
            }

            // handle all of the full partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    segments.push_back(dispatch_async(traits::get_id(sit), algo,
                        policy, forced_seq(), beg, end, f, proj));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, f, proj));
            }
        }

        return result::get(dataflow(
            [=](std::vector<shared_future<bool>>&& r) -> bool {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);
                std::vector<bool> res = hpx::unwrap(HPX_MOVE(r));
                auto it = res.begin();
                while (it != res.end())
                {
                    if (*it == false)
                        return false;
                    it++;
                }
                return true;
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename InIter, typename F>
        requires(hpx::traits::is_iterator_v<InIter> &&
            hpx::traits::is_segmented_iterator_v<InIter>)
    bool tag_invoke(hpx::none_of_t, InIter first, InIter last, F&& f)
    {
        static_assert(
            std::input_iterator<InIter>, "Requires at least input iterator.");

        return hpx::parallel::detail::segmented_none_of(
            hpx::parallel::detail::none_of(), hpx::execution::seq, first, last,
            HPX_FORWARD(F, f), hpx::identity_v, std::true_type());
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename F>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, bool> tag_invoke(
        hpx::none_of_t, ExPolicy&& policy, SegIter first, SegIter last, F&& f)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        return hpx::parallel::detail::segmented_none_of(
            hpx::parallel::detail::none_of(), HPX_FORWARD(ExPolicy, policy),
            first, last, HPX_FORWARD(F, f), hpx::identity_v, is_seq());
    }

    HPX_CXX_EXPORT template <typename InIter, typename F>
        requires(hpx::traits::is_iterator_v<InIter> &&
            hpx::traits::is_segmented_iterator_v<InIter>)
    bool tag_invoke(hpx::any_of_t, InIter first, InIter last, F&& f)
    {
        static_assert(
            std::input_iterator<InIter>, "Requires at least input iterator.");

        return hpx::parallel::detail::segmented_any_of(
            hpx::parallel::detail::any_of(), hpx::execution::seq, first, last,
            HPX_FORWARD(F, f), hpx::identity_v, std::true_type());
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename F>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, bool> tag_invoke(
        hpx::any_of_t, ExPolicy&& policy, SegIter first, SegIter last, F&& f)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        return hpx::parallel::detail::segmented_any_of(
            hpx::parallel::detail::any_of(), HPX_FORWARD(ExPolicy, policy),
            first, last, HPX_FORWARD(F, f), hpx::identity_v, is_seq());
    }

    HPX_CXX_EXPORT template <typename InIter, typename F>
        requires(hpx::traits::is_iterator_v<InIter> &&
            hpx::traits::is_segmented_iterator_v<InIter>)
    bool tag_invoke(hpx::all_of_t, InIter first, InIter last, F&& f)
    {
        static_assert(
            std::input_iterator<InIter>, "Requires at least input iterator.");

        return hpx::parallel::detail::segmented_all_of(
            hpx::parallel::detail::all_of(), hpx::execution::seq, first, last,
            HPX_FORWARD(F, f), hpx::identity_v, std::true_type());
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename F>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, bool> tag_invoke(
        hpx::all_of_t, ExPolicy&& policy, SegIter first, SegIter last, F&& f)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        return hpx::parallel::detail::segmented_all_of(
            hpx::parallel::detail::all_of(), HPX_FORWARD(ExPolicy, policy),
            first, last, HPX_FORWARD(F, f), hpx::identity_v, is_seq());
    }
}    // namespace hpx::segmented
