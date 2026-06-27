//  Copyright (c) 2017 Ajai V George
//  Copyright (c) 2024-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/executors.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <list>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented_transform

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename OutIter, typename F, typename Proj>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<SegIter, OutIter>>
    segmented_transform(Algo&& algo, ExPolicy const& policy, SegIter first,
        SegIter last, OutIter dest, F&& f, Proj&& proj, std::true_type)
    {
        typedef hpx::traits::segmented_iterator_traits<SegIter> traits1;
        typedef hpx::traits::segmented_iterator_traits<OutIter> traits2;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;

        typedef util::detail::algorithm_result<ExPolicy,
            util::in_out_result<SegIter, OutIter>>
            result;

        segment_iterator1 sit = traits1::segment(first);
        segment_iterator1 send = traits1::segment(last);
        segment_iterator2 sdest = traits2::segment(dest);

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type1 beg = traits1::local(first);
            local_iterator_type1 end = traits1::local(last);
            local_iterator_type2 ldest = traits2::local(dest);
            if (beg != end)
            {
                util::in_out_result<local_iterator_type1, local_iterator_type2>
                    out = dispatch(traits2::get_id(sdest), algo, policy,
                        std::true_type(), beg, end, ldest, f, proj);
                last = traits1::compose(send, out.in);
                dest = traits2::compose(sdest, out.out);
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type1 beg = traits1::local(first);
            local_iterator_type1 end = traits1::end(sit);
            local_iterator_type2 ldest = traits2::local(dest);
            util::in_out_result<local_iterator_type1, local_iterator_type2> out;
            if (beg != end)
            {
                out = dispatch(traits2::get_id(sdest), algo, policy,
                    std::true_type(), beg, end, ldest, f, proj);
            }

            // handle all of the full partitions
            for (++sit, ++sdest; sit != send; ++sit, ++sdest)
            {
                beg = traits1::begin(sit);
                end = traits1::end(sit);
                ldest = traits2::begin(sdest);
                if (beg != end)
                {
                    out = dispatch(traits2::get_id(sdest), algo, policy,
                        std::true_type(), beg, end, ldest, f, proj);
                }
            }

            // handle the beginning of the last partition
            beg = traits1::begin(sit);
            end = traits1::local(last);
            ldest = traits2::begin(sdest);
            if (beg != end)
            {
                out = dispatch(traits2::get_id(sdest), algo, policy,
                    std::true_type(), beg, end, ldest, f, proj);
            }
            last = traits1::compose(send, out.in);
            dest = traits2::compose(sdest, out.out);
        }
        return result::get(util::in_out_result<SegIter, OutIter>{
            HPX_MOVE(last), HPX_MOVE(dest)});
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename OutIter, typename F, typename Proj>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<SegIter, OutIter>>
    segmented_transform(Algo&& algo, ExPolicy const& policy, SegIter first,
        SegIter last, OutIter dest, F&& f, Proj&& proj, std::false_type)
    {
        typedef hpx::traits::segmented_iterator_traits<SegIter> traits1;
        typedef hpx::traits::segmented_iterator_traits<OutIter> traits2;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;

        typedef util::detail::algorithm_result<ExPolicy,
            util::in_out_result<SegIter, OutIter>>
            result;

        typedef std::integral_constant<bool, !std::forward_iterator<SegIter>>
            forced_seq;

        segment_iterator1 sit = traits1::segment(first);
        segment_iterator1 send = traits1::segment(last);
        segment_iterator2 sdest = traits2::segment(dest);

        typedef std::vector<future<
            util::in_out_result<local_iterator_type1, local_iterator_type2>>>
            segment_type;
        segment_type segments;
        segments.reserve(std::distance(sit, send));

        if (sit == send)
        {
            // all elements are on the same partition
            local_iterator_type1 beg = traits1::local(first);
            local_iterator_type1 end = traits1::local(last);
            local_iterator_type2 ldest = traits2::local(dest);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits2::get_id(sdest), algo,
                    policy, forced_seq(), beg, end, ldest, f, proj));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type1 beg = traits1::local(first);
            local_iterator_type1 end = traits1::end(sit);
            local_iterator_type2 ldest = traits2::local(dest);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits2::get_id(sdest), algo,
                    policy, forced_seq(), beg, end, ldest, f, proj));
            }

            // handle all of the full partitions
            for (++sit, ++sdest; sit != send; ++sit, ++sdest)
            {
                beg = traits1::begin(sit);
                end = traits1::end(sit);
                ldest = traits2::begin(sdest);
                if (beg != end)
                {
                    segments.push_back(dispatch_async(traits2::get_id(sdest),
                        algo, policy, forced_seq(), beg, end, ldest, f, proj));
                }
            }

            // handle the beginning of the last partition
            beg = traits1::begin(sit);
            end = traits1::local(last);
            ldest = traits2::begin(sdest);
            if (beg != end)
            {
                segments.push_back(dispatch_async(traits2::get_id(sdest), algo,
                    policy, forced_seq(), beg, end, ldest, f, proj));
            }
        }

        return result::get(dataflow(
            [=](segment_type&& r) -> util::in_out_result<SegIter, OutIter> {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);
                auto ft = r.back().get();
                auto olast = traits1::compose(send, ft.in);
                auto odest = traits2::compose(sdest, ft.out);
                return util::in_out_result<SegIter, OutIter>{olast, odest};
            },
            HPX_MOVE(segments)));
    }

    // Binary transform

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F, typename Proj1,
        typename Proj2>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    segmented_transform(Algo&& algo, ExPolicy&& policy, InIter1 first1,
        InIter1 last1, InIter2 first2, OutIter dest, F&& f, Proj1&& proj1,
        Proj2&& proj2, std::true_type)
    {
        typedef hpx::traits::segmented_iterator_traits<InIter1> traits1;
        typedef hpx::traits::segmented_iterator_traits<InIter2> traits2;
        typedef hpx::traits::segmented_iterator_traits<OutIter> traits3;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;
        typedef typename traits3::segment_iterator segment_iterator3;
        typedef typename traits3::local_iterator local_iterator_type3;

        typedef util::detail::algorithm_result<ExPolicy,
            util::in_in_out_result<InIter1, InIter2, OutIter>>
            result;

        auto last2 = first2;
        detail::advance(last2, std::distance(first1, last1));

        segment_iterator1 sit1 = traits1::segment(first1);
        segment_iterator1 send1 = traits1::segment(last1);
        segment_iterator2 sit2 = traits2::segment(first2);
        segment_iterator2 send2 = traits2::segment(last2);
        segment_iterator3 sdest = traits3::segment(dest);
        if (sit1 == send1)
        {
            // all elements are on the same partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::local(last1);
            local_iterator_type2 beg2 = traits2::local(first2);
            local_iterator_type3 ldest = traits3::local(dest);
            if (beg1 != end1)
            {
                util::in_in_out_result<local_iterator_type1,
                    local_iterator_type2, local_iterator_type3>
                    out = dispatch(traits1::get_id(sit1), algo, policy,
                        std::true_type(), beg1, end1, beg2, ldest, f, proj1,
                        proj2);
                last1 = traits1::compose(send1, out.in1);
                last2 = traits2::compose(send2, out.in2);
                dest = traits3::compose(sdest, out.out);
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::end(sit1);
            local_iterator_type2 beg2 = traits2::local(first2);
            local_iterator_type3 ldest = traits3::local(dest);
            util::in_in_out_result<local_iterator_type1, local_iterator_type2,
                local_iterator_type3>
                out;
            if (beg1 != end1)
            {
                out = dispatch(traits1::get_id(sit1), algo, policy,
                    std::true_type(), beg1, end1, beg2, ldest, f, proj1, proj2);
            }

            // handle all of the full partitions
            for (++sit1, ++sit2, ++sdest; sit1 != send1;
                ++sit1, ++sit2, ++sdest)
            {
                beg1 = traits1::begin(sit1);
                end1 = traits1::end(sit1);
                beg2 = traits2::begin(sit2);
                ldest = traits3::begin(sdest);
                if (beg1 != end1)
                {
                    out = dispatch(traits1::get_id(sit1), algo, policy,
                        std::true_type(), beg1, end1, beg2, ldest, f, proj1,
                        proj2);
                }
            }

            // handle the beginning of the last partition
            beg1 = traits1::begin(sit1);
            end1 = traits1::local(last1);
            beg2 = traits2::begin(sit2);
            ldest = traits3::begin(sdest);
            if (beg1 != end1)
            {
                out = dispatch(traits1::get_id(sit1), algo, policy,
                    std::true_type(), beg1, end1, beg2, ldest, f, proj1, proj2);
            }
            last1 = traits1::compose(send1, out.in1);
            last2 = traits2::compose(send2, out.in2);
            dest = traits3::compose(sdest, out.out);
        }
        return result::get(util::in_in_out_result<InIter1, InIter2, OutIter>{
            HPX_MOVE(last1), HPX_MOVE(last2), HPX_MOVE(dest)});
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F, typename Proj1,
        typename Proj2>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    segmented_transform(Algo&& algo, ExPolicy&& policy, InIter1 first1,
        InIter1 last1, InIter2 first2, OutIter dest, F&& f, Proj1&& proj1,
        Proj2&& proj2, std::false_type)
    {
        typedef hpx::traits::segmented_iterator_traits<InIter1> traits1;
        typedef hpx::traits::segmented_iterator_traits<InIter2> traits2;
        typedef hpx::traits::segmented_iterator_traits<OutIter> traits3;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;
        typedef typename traits3::segment_iterator segment_iterator3;
        typedef typename traits3::local_iterator local_iterator_type3;

        typedef util::detail::algorithm_result<ExPolicy,
            util::in_in_out_result<InIter1, InIter2, OutIter>>
            result;

        typedef std::integral_constant<bool,
            !std::forward_iterator<InIter1> || !std::forward_iterator<InIter2>>
            forced_seq;

        auto last2 = first2;
        detail::advance(last2, std::distance(first1, last1));

        segment_iterator1 sit1 = traits1::segment(first1);
        segment_iterator1 send1 = traits1::segment(last1);
        segment_iterator2 sit2 = traits2::segment(first2);
        segment_iterator2 send2 = traits2::segment(last2);
        segment_iterator3 sdest = traits3::segment(dest);

        typedef std::vector<future<util::in_in_out_result<local_iterator_type1,
            local_iterator_type2, local_iterator_type3>>>
            segment_type;
        segment_type segments;
        segments.reserve(std::distance(sit1, send1));

        if (sit1 == send1)
        {
            // all elements are on the same partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::local(last1);
            local_iterator_type2 beg2 = traits2::local(first2);
            local_iterator_type3 ldest = traits3::local(dest);
            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, ldest, f, proj1,
                    proj2));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::end(sit1);
            local_iterator_type2 beg2 = traits2::local(first2);
            local_iterator_type3 ldest = traits3::local(dest);
            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, ldest, f, proj1,
                    proj2));
            }

            // handle all of the full partitions
            for (++sit1, ++sit2, ++sdest; sit1 != send1;
                ++sit1, ++sit2, ++sdest)
            {
                beg1 = traits1::begin(sit1);
                end1 = traits1::end(sit1);
                beg2 = traits2::begin(sit2);
                ldest = traits3::begin(sdest);
                if (beg1 != end1)
                {
                    segments.push_back(dispatch_async(traits1::get_id(sit1),
                        algo, policy, forced_seq(), beg1, end1, beg2, ldest, f,
                        proj1, proj2));
                }
            }

            // handle the beginning of the last partition
            beg1 = traits1::begin(sit1);
            end1 = traits1::local(last1);
            beg2 = traits2::begin(sit2);
            ldest = traits3::begin(sdest);
            if (beg1 != end1)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, ldest, f, proj1,
                    proj2));
            }
        }

        return result::get(dataflow(
            [=](segment_type&& r)
                -> util::in_in_out_result<InIter1, InIter2, OutIter> {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);
                auto rl = r.back().get();
                auto olast1 = traits1::compose(send1, rl.in1);
                auto olast2 = traits2::compose(send2, rl.in2);
                auto odest = traits3::compose(sdest, rl.out);
                return util::in_in_out_result<InIter1, InIter2, OutIter>{
                    olast1, olast2, odest};
            },
            HPX_MOVE(segments)));
    }

    // Binary transform1

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F, typename Proj1,
        typename Proj2>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    segmented_transform(Algo&& algo, ExPolicy&& policy, InIter1 first1,
        InIter1 last1, InIter2 first2, InIter2 last2, OutIter dest, F&& f,
        Proj1&& proj1, Proj2&& proj2, std::true_type)
    {
        typedef hpx::traits::segmented_iterator_traits<InIter1> traits1;
        typedef hpx::traits::segmented_iterator_traits<InIter2> traits2;
        typedef hpx::traits::segmented_iterator_traits<OutIter> traits3;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;
        typedef typename traits3::segment_iterator segment_iterator3;
        typedef typename traits3::local_iterator local_iterator_type3;

        typedef util::detail::algorithm_result<ExPolicy,
            util::in_in_out_result<InIter1, InIter2, OutIter>>
            result;

        segment_iterator1 sit1 = traits1::segment(first1);
        segment_iterator1 send1 = traits1::segment(last1);
        segment_iterator2 sit2 = traits2::segment(first2);
        segment_iterator2 send2 = traits2::segment(last2);
        segment_iterator3 sdest = traits3::segment(dest);

        if (sit1 == send1 && sit2 == send2)
        {
            // all elements are on the same partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::local(last1);
            local_iterator_type2 beg2 = traits2::local(first2);
            local_iterator_type2 end2 = traits2::local(last2);
            local_iterator_type3 ldest = traits3::local(dest);
            if (beg1 != end1 && beg2 != end2)
            {
                util::in_in_out_result<local_iterator_type1,
                    local_iterator_type2, local_iterator_type3>
                    out = dispatch(traits1::get_id(sit1), algo, policy,
                        std::true_type(), beg1, end1, beg2, end2, ldest, f,
                        proj1, proj2);
                last1 = traits1::compose(send1, out.in1);
                last2 = traits2::compose(send2, out.in2);
                dest = traits3::compose(sdest, out.out);
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::end(sit1);
            local_iterator_type2 beg2 = traits2::local(first2);
            local_iterator_type2 end2 = traits2::end(sit2);
            local_iterator_type3 ldest = traits3::local(dest);
            util::in_in_out_result<local_iterator_type1, local_iterator_type2,
                local_iterator_type3>
                out;
            if (beg1 != end1 && beg2 != end2)
            {
                out = dispatch(traits1::get_id(sit1), algo, policy,
                    std::true_type(), beg1, end1, beg2, end2, ldest, f, proj1,
                    proj2);
            }

            // handle all of the full partitions
            for (++sit1, ++sit2, ++sdest; sit1 != send1 && sit2 != send2;
                ++sit1, ++sit2, ++sdest)
            {
                beg1 = traits1::begin(sit1);
                end1 = traits1::end(sit1);
                beg2 = traits2::begin(sit2);
                end2 = traits2::end(sit2);
                ldest = traits3::begin(sdest);
                if (beg1 != end1 && beg2 != end2)
                {
                    out = dispatch(traits1::get_id(sit1), algo, policy,
                        std::true_type(), beg1, end1, beg2, end2, ldest, f,
                        proj1, proj2);
                }
            }

            // handle the beginning of the last partition
            beg1 = traits1::begin(sit1);
            end1 = traits1::end(sit1);
            beg2 = traits2::begin(sit2);
            end2 = traits2::end(sit2);
            ldest = traits3::begin(sdest);
            if (beg1 != end1 && beg2 != end2)
            {
                out = dispatch(traits1::get_id(sit1), algo, policy,
                    std::true_type(), beg1, end1, beg2, end2, ldest, f, proj1,
                    proj2);
            }
            last1 = traits1::compose(send1, out.in1);
            last2 = traits2::compose(send2, out.in2);
            dest = traits3::compose(sdest, out.out);
        }
        return result::get(util::in_in_out_result<InIter1, InIter2, OutIter>{
            HPX_MOVE(last1), HPX_MOVE(last2), HPX_MOVE(dest)});
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F, typename Proj1,
        typename Proj2>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_in_out_result<InIter1, InIter2, OutIter>>
    segmented_transform(Algo&& algo, ExPolicy&& policy, InIter1 first1,
        InIter1 last1, InIter2 first2, InIter2 last2, OutIter dest, F&& f,
        Proj1&& proj1, Proj2&& proj2, std::false_type)
    {
        typedef hpx::traits::segmented_iterator_traits<InIter1> traits1;
        typedef hpx::traits::segmented_iterator_traits<InIter2> traits2;
        typedef hpx::traits::segmented_iterator_traits<OutIter> traits3;
        typedef typename traits1::segment_iterator segment_iterator1;
        typedef typename traits1::local_iterator local_iterator_type1;
        typedef typename traits2::segment_iterator segment_iterator2;
        typedef typename traits2::local_iterator local_iterator_type2;
        typedef typename traits3::segment_iterator segment_iterator3;
        typedef typename traits3::local_iterator local_iterator_type3;

        typedef util::detail::algorithm_result<ExPolicy,
            util::in_in_out_result<InIter1, InIter2, OutIter>>
            result;

        typedef std::integral_constant<bool,
            !std::forward_iterator<InIter1> || !std::forward_iterator<InIter2>>
            forced_seq;

        segment_iterator1 sit1 = traits1::segment(first1);
        segment_iterator1 send1 = traits1::segment(last1);
        segment_iterator2 sit2 = traits2::segment(first2);
        segment_iterator2 send2 = traits2::segment(last2);
        segment_iterator3 sdest = traits3::segment(dest);

        typedef std::vector<future<util::in_in_out_result<local_iterator_type1,
            local_iterator_type2, local_iterator_type3>>>
            segment_type;
        segment_type segments;
        segments.reserve(std::distance(sit1, send1));

        if (sit1 == send1 && sit2 == send2)
        {
            // all elements are on the same partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::local(last1);
            local_iterator_type2 beg2 = traits2::local(first2);
            local_iterator_type2 end2 = traits2::local(last2);
            local_iterator_type3 ldest = traits3::local(dest);
            if (beg1 != end1 && beg2 != end2)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, end2, ldest, f,
                    proj1, proj2));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type1 beg1 = traits1::local(first1);
            local_iterator_type1 end1 = traits1::end(sit1);
            local_iterator_type2 beg2 = traits2::local(first2);
            local_iterator_type2 end2 = traits2::end(sit2);
            local_iterator_type3 ldest = traits3::local(dest);
            if (beg1 != end1 && beg2 != end2)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, end2, ldest, f,
                    proj1, proj2));
            }

            // handle all of the full partitions
            for (++sit1, ++sit2, ++sdest; sit1 != send1 && sit2 != send2;
                ++sit1, ++sit2, ++sdest)
            {
                beg1 = traits1::begin(sit1);
                beg2 = traits2::begin(sit2);
                end1 = traits1::end(sit1);
                end2 = traits2::end(sit2);
                ldest = traits3::begin(sdest);
                if (beg1 != end1 && beg2 != end2)
                {
                    segments.push_back(dispatch_async(traits1::get_id(sit1),
                        algo, policy, forced_seq(), beg1, end1, beg2, end2,
                        ldest, f, proj1, proj2));
                }
            }

            // handle the beginning of the last partition
            beg1 = traits1::begin(sit1);
            beg2 = traits2::begin(sit2);
            end1 = traits1::end(sit1);
            end2 = traits2::end(sit2);
            ldest = traits3::begin(sdest);
            if (beg1 != end1 && beg2 != end2)
            {
                segments.push_back(dispatch_async(traits1::get_id(sit1), algo,
                    policy, forced_seq(), beg1, end1, beg2, end2, ldest, f,
                    proj1, proj2));
            }
        }

        return result::get(dataflow(
            [=](segment_type&& r)
                -> util::in_in_out_result<InIter1, InIter2, OutIter> {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);
                auto rl = r.back().get();
                auto olast1 = traits1::compose(send1, rl.in1);
                auto olast2 = traits2::compose(send2, rl.in2);
                auto odest = traits3::compose(sdest, rl.out);
                return util::in_in_out_result<InIter1, InIter2, OutIter>{
                    olast1, olast2, odest};
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename SegIter, typename OutIter, typename F>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    hpx::parallel::util::in_out_result<SegIter, OutIter> tag_invoke(
        hpx::transform_t, SegIter first, SegIter last, OutIter dest, F&& f)
    {
        static_assert(
            std::input_iterator<SegIter>, "Requires at least input iterator.");

        if (first == last)
        {
            return hpx::parallel::util::in_out_result<SegIter, OutIter>{
                HPX_MOVE(first), HPX_MOVE(dest)};
        }

        using iterator_traits1 =
            hpx::traits::segmented_iterator_traits<SegIter>;
        using iterator_traits2 =
            hpx::traits::segmented_iterator_traits<OutIter>;

        return hpx::parallel::detail::segmented_transform(
            hpx::parallel::detail::transform<hpx::parallel::util::in_out_result<
                typename iterator_traits1::local_iterator,
                typename iterator_traits2::local_iterator>>(),
            hpx::execution::seq, first, last, dest, HPX_FORWARD(F, f),
            hpx::identity_v, std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter,
        typename OutIter, typename F>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        hpx::parallel::util::in_out_result<SegIter, OutIter>>
    tag_invoke(hpx::transform_t, ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, F&& f)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
        using result = hpx::parallel::util::detail::algorithm_result<ExPolicy,
            hpx::parallel::util::in_out_result<SegIter, OutIter>>;

        if (first == last)
        {
            return result::get(
                hpx::parallel::util::in_out_result<SegIter, OutIter>{
                    HPX_MOVE(first), HPX_MOVE(dest)});
        }

        using iterator_traits1 =
            hpx::traits::segmented_iterator_traits<SegIter>;
        using iterator_traits2 =
            hpx::traits::segmented_iterator_traits<OutIter>;

        return hpx::parallel::detail::segmented_transform(
            hpx::parallel::detail::transform<hpx::parallel::util::in_out_result<
                typename iterator_traits1::local_iterator,
                typename iterator_traits2::local_iterator>>(),
            HPX_FORWARD(ExPolicy, policy), first, last, dest, HPX_FORWARD(F, f),
            hpx::identity_v, is_seq());
    }

    HPX_CXX_EXPORT template <typename InIter1, typename InIter2,
        typename OutIter, typename F>
        requires(hpx::traits::is_iterator_v<InIter1> &&
            hpx::traits::is_segmented_iterator_v<InIter1> &&
            hpx::traits::is_iterator_v<InIter2> &&
            hpx::traits::is_segmented_iterator_v<InIter2> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    hpx::parallel::util::in_in_out_result<InIter1, InIter2, OutIter> tag_invoke(
        hpx::transform_t, InIter1 first1, InIter1 last1, InIter2 first2,
        OutIter dest, F&& f)
    {
        static_assert(
            std::input_iterator<InIter1> && std::input_iterator<InIter2>,
            "Requires at least input iterator.");

        auto last2 = first2;
        hpx::parallel::detail::advance(last2, std::distance(first1, last1));

        if (first1 == last1)
        {
            return hpx::parallel::util::in_in_out_result<InIter1, InIter2,
                OutIter>{HPX_MOVE(first1), HPX_MOVE(first2), HPX_MOVE(dest)};
        }

        using iterator1_traits =
            hpx::traits::segmented_iterator_traits<InIter1>;
        using iterator2_traits =
            hpx::traits::segmented_iterator_traits<InIter2>;
        using iterator3_traits =
            hpx::traits::segmented_iterator_traits<OutIter>;

        using proj_id = hpx::identity;

        return hpx::parallel::detail::segmented_transform(
            hpx::parallel::detail::transform_binary<hpx::parallel::util::
                    in_in_out_result<typename iterator1_traits::local_iterator,
                        typename iterator2_traits::local_iterator,
                        typename iterator3_traits::local_iterator>>(),
            hpx::execution::seq, first1, last1, first2, dest, HPX_FORWARD(F, f),
            proj_id{}, proj_id{}, std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<InIter1> &&
            hpx::traits::is_segmented_iterator_v<InIter1> &&
            hpx::traits::is_iterator_v<InIter2> &&
            hpx::traits::is_segmented_iterator_v<InIter2> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        hpx::parallel::util::in_in_out_result<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::transform_t, ExPolicy&& policy, InIter1 first1,
        InIter1 last1, InIter2 first2, OutIter dest, F&& f)
    {
        static_assert(
            std::forward_iterator<InIter1> && std::forward_iterator<InIter2>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
        using result = hpx::parallel::util::detail::algorithm_result<ExPolicy,
            hpx::parallel::util::in_in_out_result<InIter1, InIter2, OutIter>>;

        auto last2 = first2;
        hpx::parallel::detail::advance(last2, std::distance(first1, last1));

        if (first1 == last1)
        {
            return result::get(hpx::parallel::util::in_in_out_result<InIter1,
                InIter2, OutIter>{
                HPX_MOVE(first1), HPX_MOVE(first2), HPX_MOVE(dest)});
        }

        using iterator1_traits =
            hpx::traits::segmented_iterator_traits<InIter1>;
        using iterator2_traits =
            hpx::traits::segmented_iterator_traits<InIter2>;
        using iterator3_traits =
            hpx::traits::segmented_iterator_traits<OutIter>;

        using proj_id = hpx::identity;

        return hpx::parallel::detail::segmented_transform(
            hpx::parallel::detail::transform_binary<hpx::parallel::util::
                    in_in_out_result<typename iterator1_traits::local_iterator,
                        typename iterator2_traits::local_iterator,
                        typename iterator3_traits::local_iterator>>(),
            HPX_FORWARD(ExPolicy, policy), first1, last1, first2, dest,
            HPX_FORWARD(F, f), proj_id{}, proj_id{}, is_seq());
    }

    HPX_CXX_EXPORT template <typename InIter1, typename InIter2,
        typename OutIter, typename F>
        requires(hpx::traits::is_iterator_v<InIter1> &&
            hpx::traits::is_segmented_iterator_v<InIter1> &&
            hpx::traits::is_iterator_v<InIter2> &&
            hpx::traits::is_segmented_iterator_v<InIter2> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    hpx::parallel::util::in_in_out_result<InIter1, InIter2, OutIter> tag_invoke(
        hpx::transform_t, InIter1 first1, InIter1 last1, InIter2 first2,
        InIter2 last2, OutIter dest, F&& f)
    {
        static_assert(
            std::input_iterator<InIter1> && std::input_iterator<InIter2>,
            "Requires at least input iterator.");

        if (first1 == last1)
        {
            return hpx::parallel::util::in_in_out_result<InIter1, InIter2,
                OutIter>{HPX_MOVE(first1), HPX_MOVE(first2), HPX_MOVE(dest)};
        }

        using iterator1_traits =
            hpx::traits::segmented_iterator_traits<InIter1>;
        using iterator2_traits =
            hpx::traits::segmented_iterator_traits<InIter2>;
        using iterator3_traits =
            hpx::traits::segmented_iterator_traits<OutIter>;

        using proj_id = hpx::identity;

        return hpx::parallel::detail::segmented_transform(
            hpx::parallel::detail::transform_binary2<hpx::parallel::util::
                    in_in_out_result<typename iterator1_traits::local_iterator,
                        typename iterator2_traits::local_iterator,
                        typename iterator3_traits::local_iterator>>(),
            hpx::execution::seq, first1, last1, first2, last2, dest,
            HPX_FORWARD(F, f), proj_id{}, proj_id{}, std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename OutIter, typename F>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<InIter1> &&
            hpx::traits::is_segmented_iterator_v<InIter1> &&
            hpx::traits::is_iterator_v<InIter2> &&
            hpx::traits::is_segmented_iterator_v<InIter2> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        hpx::parallel::util::in_in_out_result<InIter1, InIter2, OutIter>>
    tag_invoke(hpx::transform_t, ExPolicy&& policy, InIter1 first1,
        InIter1 last1, InIter2 first2, InIter2 last2, OutIter dest, F&& f)
    {
        static_assert(
            std::forward_iterator<InIter1> && std::forward_iterator<InIter2>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
        using result = hpx::parallel::util::detail::algorithm_result<ExPolicy,
            hpx::parallel::util::in_in_out_result<InIter1, InIter2, OutIter>>;

        if (first1 == last1)
        {
            return result::get(hpx::parallel::util::in_in_out_result<InIter1,
                InIter2, OutIter>{
                HPX_MOVE(first1), HPX_MOVE(first2), HPX_MOVE(dest)});
        }

        using iterator1_traits =
            hpx::traits::segmented_iterator_traits<InIter1>;
        using iterator2_traits =
            hpx::traits::segmented_iterator_traits<InIter2>;
        using iterator3_traits =
            hpx::traits::segmented_iterator_traits<OutIter>;

        using proj_id = hpx::identity;

        return hpx::parallel::detail::segmented_transform(
            hpx::parallel::detail::transform_binary2<hpx::parallel::util::
                    in_in_out_result<typename iterator1_traits::local_iterator,
                        typename iterator2_traits::local_iterator,
                        typename iterator3_traits::local_iterator>>(),
            HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2, dest,
            HPX_FORWARD(F, f), proj_id{}, proj_id{}, is_seq());
    }
}    // namespace hpx::segmented
