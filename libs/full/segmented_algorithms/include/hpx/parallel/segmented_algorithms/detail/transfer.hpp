//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/async_distributed.hpp>
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
    // segmented transfer

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename SegOutIter>
    static util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<SegIter, SegOutIter>>
    segmented_transfer(Algo&& algo, ExPolicy const& policy, std::true_type,
        SegIter first, SegIter last, SegOutIter dest)
    {
        using traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;

        using output_traits =
            hpx::traits::segmented_iterator_traits<SegOutIter>;
        using segment_output_iterator =
            typename output_traits::segment_iterator;
        using local_output_iterator_type =
            typename output_traits::local_iterator;

        using local_iterator_pair = util::in_out_result<local_iterator_type,
            local_output_iterator_type>;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        segment_output_iterator sdest = traits::segment(dest);

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);

            if (beg != end)
            {
                local_iterator_pair p = dispatch(traits::get_id(sit), algo,
                    policy, std::true_type(), beg, end, traits::local(dest));

                dest = output_traits::compose(sdest, p.out);
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);
            local_output_iterator_type out = traits::local(dest);

            if (beg != end)
            {
                local_iterator_pair p = dispatch(traits::get_id(sit), algo,
                    policy, std::true_type(), beg, end, out);
                out = p.out;
            }

            // handle all full partitions
            for ((void) ++sit, ++sdest; sit != send; (void) ++sit, ++sdest)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                out = traits::begin(sdest);

                if (beg != end)
                {
                    local_iterator_pair p = dispatch(traits::get_id(sit), algo,
                        policy, std::true_type(), beg, end, out);
                    out = p.out;
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                local_iterator_pair p = dispatch(traits::get_id(sit), algo,
                    policy, std::true_type(), beg, end, traits::begin(sdest));
                out = p.out;
            }

            dest = output_traits::compose(sdest, out);
        }

        using result_type = util::in_out_result<SegIter, SegOutIter>;

        return util::detail::algorithm_result<ExPolicy, result_type>::get(
            result_type{last, dest});
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename SegOutIter>
    static util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<SegIter, SegOutIter>>
    segmented_transfer(Algo&& algo, ExPolicy const& policy, std::false_type,
        SegIter first, SegIter last, SegOutIter dest)
    {
        using traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;

        using output_traits =
            hpx::traits::segmented_iterator_traits<SegOutIter>;
        using segment_output_iterator =
            typename output_traits::segment_iterator;
        using local_output_iterator_type =
            typename output_traits::local_iterator;

        using local_iterator_pair = util::in_out_result<local_iterator_type,
            local_output_iterator_type>;

        using forced_seq =
            std::integral_constant<bool, !std::forward_iterator<SegIter>>;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        segment_output_iterator sdest = traits::segment(dest);

        std::vector<shared_future<local_iterator_pair>> segments;
        segments.reserve(std::distance(sit, send));

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);

            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, traits::local(dest)));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);
            local_output_iterator_type out = traits::local(dest);

            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, out));
            }

            // handle all full partitions
            for ((void) ++sit, ++sdest; sit != send; (void) ++sit, ++sdest)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                out = traits::begin(sdest);

                if (beg != end)
                {
                    segments.push_back(dispatch_async(traits::get_id(sit), algo,
                        policy, forced_seq(), beg, end, out));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);

            if (beg != end)
            {
                segments.push_back(dispatch_async(traits::get_id(sit), algo,
                    policy, forced_seq(), beg, end, traits::begin(sdest)));
            }
        }

        // NOLINTNEXTLINE(bugprone-use-after-move)
        HPX_ASSERT(!segments.empty());

        return util::detail::algorithm_result<ExPolicy,
            util::in_out_result<SegIter, SegOutIter>>::
            get(hpx::dataflow(
                [=](std::vector<shared_future<local_iterator_pair>>&& r)
                    -> util::in_out_result<SegIter, SegOutIter> {
                    // handle any remote exceptions, will throw on error
                    std::list<std::exception_ptr> errors;
                    parallel::util::detail::handle_remote_exceptions<
                        ExPolicy>::call(r, errors);

                    local_iterator_pair p = r.back().get();
                    using result_type =
                        util::in_out_result<SegIter, SegOutIter>;

                    return result_type{output_traits::compose(sdest, p.in),
                        output_traits::compose(sdest, p.out)};
                },
                HPX_MOVE(segments)));
    }

    ///////////////////////////////////////////////////////////////////////
    // segmented implementation
    HPX_CXX_EXPORT template <typename Algo, typename ExPolicy,
        typename FwdIter1, typename Sent1, typename FwdIter2>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<FwdIter1, FwdIter2>>
    transfer_(ExPolicy&& policy, FwdIter1 first, Sent1 last, FwdIter2 dest,
        std::true_type)
    {
        if (first == last)
        {
            using result_type = util::in_out_result<FwdIter1, FwdIter2>;

            return util::detail::algorithm_result<ExPolicy, result_type>::get(
                result_type{last, dest});
        }

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
        return segmented_transfer(
            Algo(), HPX_FORWARD(ExPolicy, policy), is_seq(), first, last, dest);
    }

    // forward declare the non-segmented version of this algorithm
    template <typename Algo, typename ExPolicy, typename FwdIter1,
        typename Sent1, typename FwdIter2>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<FwdIter1, FwdIter2>>
    transfer_(ExPolicy&& policy, FwdIter1 first, Sent1 last, FwdIter2 dest,
        std::false_type);

    /// \endcond
}    // namespace hpx::parallel::detail
