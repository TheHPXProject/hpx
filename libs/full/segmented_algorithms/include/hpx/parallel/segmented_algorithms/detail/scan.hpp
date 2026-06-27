//  Copyright (c) 2016 Minh-Khanh Do
//  Copyright (c) 2020-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/segmented_algorithms/detail/scan.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented scan

    /// \cond NOINTERNAL

    // returns the last value of the scan used to compute the next init value
    template <typename T, typename InIter, typename Op, typename Conv>
    T sequential_segmented_scan_t(
        InIter first, InIter last, Op&& op, Conv&& conv)
    {
        T ret = T();
        if (first != last)
        {
            ret = HPX_INVOKE(conv, *first);
            for (++first; first != last; ++first)
            {
                ret = hpx::invoke(op, ret, hpx::invoke(conv, *first));
            }
        }
        return ret;
    }

    // does a scan and returns last value of the scan
    template <typename Value>
    struct segmented_scan_t : algorithm<segmented_scan_t<Value>, Value>
    {
        using T = Value;

        constexpr segmented_scan_t() noexcept
          : algorithm<segmented_scan_t<Value>, Value>("segmented_scan_t")
        {
        }

        template <typename ExPolicy, typename InIter, typename Op,
            typename Conv>
        static T sequential(
            ExPolicy&&, InIter first, InIter last, Op&& op, Conv&& conv)
        {
            return sequential_segmented_scan_t<T>(
                first, last, HPX_FORWARD(Op, op), HPX_FORWARD(Conv, conv));
        }

        template <typename ExPolicy, typename FwdIter, typename Op,
            typename Conv>
        static util::detail::algorithm_result_t<ExPolicy, T> parallel(
            ExPolicy&& policy, FwdIter first, FwdIter last, Op&& op,
            Conv&& conv)
        {
            using execution_policy_type = std::decay_t<ExPolicy>;
            return util::partitioner<ExPolicy, T>::call(
                HPX_FORWARD(ExPolicy, policy), first,
                std::distance(first, last),
                [op, conv](
                    FwdIter part_begin, std::size_t part_size) mutable -> T {
                    T ret = hpx::invoke(conv, *part_begin);
                    if (part_size > 1)
                    {
                        // MSVC complains if 'op' is captured by reference
                        util::const_loop_n<execution_policy_type>(
                            part_begin + 1, part_size - 1,
                            [&ret, op, conv](
                                FwdIter const& curr) mutable -> void {
                                ret = hpx::invoke(
                                    op, ret, hpx::invoke(conv, *curr));
                            });
                    }
                    return ret;
                },
                hpx::unwrapping([op](auto&& results) mutable -> T {
                    T ret = *results.begin();
                    if (results.size() > 1)
                    {
                        // MSVC complains if 'op' is captured by reference
                        util::const_loop_n<execution_policy_type>(
                            results.begin() + 1, results.size() - 1,
                            [&ret, op](typename std::vector<T>::iterator const&
                                    curr) mutable {
                                ret = hpx::invoke(op, ret, *curr);
                            });
                    }
                    return ret;
                }));
        }
    };

    // do the scan (exclusive/inclusive)
    // does not return anything
    template <typename Algo>
    struct segmented_scan_void : algorithm<segmented_scan_void<Algo>>
    {
        constexpr segmented_scan_void() noexcept
          : algorithm<segmented_scan_void<Algo>>("segmented_scan_void")
        {
        }

        template <typename ExPolicy, typename InIter, typename OutIter,
            typename T, typename Op, typename Conv>
        static hpx::util::unused_type sequential(ExPolicy&& policy,
            InIter first, InIter last, OutIter dest, T&& init, Op&& op,
            Conv&& conv)
        {
            Algo().sequential(HPX_FORWARD(ExPolicy, policy), first, last, dest,
                HPX_FORWARD(T, init), HPX_FORWARD(Op, op),
                HPX_FORWARD(Conv, conv));

            return hpx::util::unused;
        }

        template <typename ExPolicy, typename InIter, typename OutIter,
            typename T, typename Op, typename Conv>
        static util::detail::algorithm_result_t<ExPolicy> parallel(
            ExPolicy&& policy, InIter first, InIter last, OutIter dest,
            T&& init, Op&& op, Conv&& conv)
        {
            using result_type = util::detail::algorithm_result_t<ExPolicy>;

            if (first == last)
                return util::detail::algorithm_result<ExPolicy>::get();

            return hpx::util::void_guard<result_type>(),
                   Algo().parallel(HPX_FORWARD(ExPolicy, policy), first, last,
                       dest, HPX_FORWARD(T, init), HPX_FORWARD(Op, op),
                       HPX_FORWARD(Conv, conv));
        }
    };

    template <typename SegIter, typename OutIter>
    static bool is_segmented_the_same(
        SegIter, SegIter, OutIter, std::false_type)
    {
        return false;
    }

    // check if two segmented iterators are partitioned the same
    // partition size and id should be the same
    template <typename SegIter, typename OutIter>
    static bool is_segmented_the_same(
        SegIter first, SegIter last, OutIter dest, std::true_type)
    {
        using traits_in = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator_in = typename traits_in::segment_iterator;
        using local_iterator_type_in = typename traits_in::local_iterator;

        using traits_out = hpx::traits::segmented_iterator_traits<OutIter>;
        using segment_iterator_out = typename traits_out::segment_iterator;
        using local_iterator_type_out = typename traits_out::local_iterator;

        segment_iterator_in sit_in = traits_in::segment(first);
        segment_iterator_in send_in = traits_in::segment(last);

        segment_iterator_out sit_out = traits_out::segment(dest);

        using hpx::naming::get_locality_from_id;

        if (sit_in == send_in)
        {
            // all elements on the same partition
            local_iterator_type_in beg_in = traits_in::local(first);
            local_iterator_type_in end_in = traits_in::local(last);

            local_iterator_type_out beg_out = traits_out::local(dest);
            local_iterator_type_out end_out = traits_out::end(sit_out);

            if (beg_in != end_in)
            {
                id_type const in_id =
                    get_locality_from_id(traits_in::get_id(sit_in));
                id_type const out_id =
                    get_locality_from_id(traits_out::get_id(sit_out));

                if (in_id != out_id)
                    return false;

                std::size_t const in_dist = std::distance(beg_in, end_in);
                std::size_t const out_dist = std::distance(beg_out, end_out);

                if (out_dist < in_dist)
                    return false;
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type_in beg_in = traits_in::local(first);
            local_iterator_type_in end_in = traits_in::end(sit_in);

            local_iterator_type_out beg_out = traits_out::local(dest);
            local_iterator_type_out end_out = traits_out::end(sit_out);

            if (beg_in != end_in)
            {
                id_type const in_id =
                    get_locality_from_id(traits_in::get_id(sit_in));
                id_type const out_id =
                    get_locality_from_id(traits_out::get_id(sit_out));

                if (in_id != out_id)
                    return false;

                std::size_t const in_dist = std::distance(beg_in, end_in);
                std::size_t const out_dist = std::distance(beg_out, end_out);

                if (in_dist != out_dist)
                    return false;
            }

            // handle all partitions
            for (++sit_in, ++sit_out; sit_in != send_in; ++sit_in, ++sit_out)
            {
                beg_in = traits_in::begin(sit_in);
                end_in = traits_in::end(sit_in);

                beg_out = traits_out::begin(sit_out);
                end_out = traits_out::end(sit_out);

                if (beg_in != end_in)
                {
                    id_type in_id =
                        get_locality_from_id(traits_in::get_id(sit_in));
                    id_type out_id =
                        get_locality_from_id(traits_out::get_id(sit_out));

                    if (in_id != out_id)
                        return false;

                    std::size_t const in_dist = std::distance(beg_in, end_in);
                    std::size_t const out_dist =
                        std::distance(beg_out, end_out);

                    if (in_dist != out_dist)
                        return false;
                }
            }

            // handle the last partition
            beg_in = traits_in::begin(sit_in);
            end_in = traits_in::end(sit_in);

            beg_out = traits_out::begin(sit_out);
            end_out = traits_out::end(sit_out);

            if (beg_in != end_in)
            {
                id_type const in_id =
                    get_locality_from_id(traits_in::get_id(sit_in));
                id_type const out_id =
                    get_locality_from_id(traits_out::get_id(sit_out));

                if (in_id != out_id)
                    return false;

                std::size_t const in_dist = std::distance(beg_in, end_in);
                std::size_t const out_dist = std::distance(beg_out, end_out);

                if (in_dist != out_dist)
                    return false;
            }
        }
        return true;
    }

    ///////////////////////////////////////////////////////////////////////
    // sequential implementation

    // sequential segmented OutIter implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename OutIter, typename T, typename Op, typename Conv>
    static util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_scan_seq(ExPolicy const& policy, SegIter first, SegIter last,
        OutIter dest, Conv&& conv, T const& init, Op&& op, std::true_type)
    {
        using result = util::detail::algorithm_result<ExPolicy, OutIter>;

        if (first == last)
            return result::get(HPX_MOVE(dest));

        using traits_in = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator_in = typename traits_in::segment_iterator;
        using local_iterator_type_in = typename traits_in::local_iterator;

        using traits_out = hpx::traits::segmented_iterator_traits<OutIter>;
        using segment_iterator_out = typename traits_out::segment_iterator;
        using local_iterator_type_out = typename traits_out::local_iterator;

        using local_iterator_in_tuple =
            hpx::tuple<local_iterator_type_in, local_iterator_type_in>;

        segment_iterator_in sit_in = traits_in::segment(first);
        segment_iterator_in send_in = traits_in::segment(last);

        segment_iterator_out sit_out = traits_out::segment(dest);

        std::vector<T> results;
        std::vector<local_iterator_in_tuple> in_iters;
        std::vector<segment_iterator_out> out_segs;
        std::vector<local_iterator_type_out> out_iters;

        // 1. Step: scan on each partition, push last T of scan into results
        if (sit_in == send_in)
        {
            // all elements on the same partition
            local_iterator_type_in beg = traits_in::local(first);
            local_iterator_type_in end = traits_in::local(last);
            if (beg != end)
            {
                results.push_back(
                    dispatch(traits_in::get_id(sit_in), segmented_scan_t<T>(),
                        policy, std::true_type(), beg, end, op, conv));
                in_iters.push_back(hpx::make_tuple(beg, end));
                out_segs.push_back(sit_out);
                out_iters.push_back(traits_out::local(dest));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type_in beg = traits_in::local(first);
            local_iterator_type_in end = traits_in::end(sit_in);

            if (beg != end)
            {
                results.push_back(
                    dispatch(traits_in::get_id(sit_in), segmented_scan_t<T>(),
                        policy, std::true_type(), beg, end, op, conv));
                in_iters.push_back(hpx::make_tuple(beg, end));
                out_segs.push_back(sit_out);
                out_iters.push_back(traits_out::local(dest));
            }

            // handle all partitions
            for (++sit_in, ++sit_out; sit_in != send_in; ++sit_in, ++sit_out)
            {
                beg = traits_in::begin(sit_in);
                end = traits_in::end(sit_in);
                if (beg != end)
                {
                    results.push_back(dispatch(traits_in::get_id(sit_in),
                        segmented_scan_t<T>(), policy, std::true_type(), beg,
                        end, op, conv));
                    in_iters.push_back(hpx::make_tuple(beg, end));
                    out_segs.push_back(sit_out);
                    out_iters.push_back(traits_out::begin(sit_out));
                }
            }

            // handle the beginning of the last partition
            beg = traits_in::begin(sit_in);
            end = traits_in::local(last);
            if (beg != end)
            {
                results.push_back(
                    dispatch(traits_in::get_id(sit_in), segmented_scan_t<T>(),
                        policy, std::true_type(), beg, end, op, conv));
                in_iters.push_back(hpx::make_tuple(beg, end));
                out_segs.push_back(sit_out);
                out_iters.push_back(traits_out::begin(sit_out));
            }
        }

        // first init value is the given init value
        T last_value = init;
        for (std::size_t i = 0; i < results.size(); ++i)
        {
            using hpx::get;
            local_iterator_type_out out = out_iters[i];

            // 2. Step: use the init values to dispatch final scan for each
            // segment
            dispatch(traits_out::get_id(out_segs[i]),
                segmented_scan_void<Algo>(), policy, std::true_type(),
                get<0>(in_iters[i]), get<1>(in_iters[i]), out, conv, last_value,
                op);

            // 3. Step: compute new init values for the next segment
            last_value = op(last_value, results[i]);
        }

        OutIter final_dest = dest;
        std::advance(final_dest, std::distance(first, last));

        return result::get(HPX_MOVE(final_dest));
    }

    // sequential non segmented OutIter implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename OutIter, typename T, typename Op, typename F1, typename F2>
    static util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_scan_seq_non(ExPolicy const& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, F1&& f1, F2&& f2)
    {
        using result = util::detail::algorithm_result<ExPolicy, OutIter>;

        if (first == last)
            return result::get(HPX_MOVE(dest));

        using traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        using vector_type = std::vector<T>;

        std::vector<vector_type> results;

        // scan on each partition, push whole result vector into results
        if (sit == send)
        {
            // all elements on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                results.push_back(dispatch(traits::get_id(sit), Algo(), policy,
                    std::true_type(), beg, end, op));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);

            if (beg != end)
            {
                results.push_back(dispatch(traits::get_id(sit), Algo(), policy,
                    std::true_type(), beg, end, op));
            }

            // handle all partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    results.push_back(dispatch(traits::get_id(sit), Algo(),
                        policy, std::true_type(), beg, end, op));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                results.push_back(dispatch(traits::get_id(sit), Algo(), policy,
                    std::true_type(), beg, end, op));
            }
        }

        // merge results with given merge algorithm f1
        // update init value with function f2
        T last_value = init;
        for (auto res : results)
        {
            dest = f1(res.begin(), res.end(), dest, last_value, op);
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 100000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
            last_value = f2(res, last_value);
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 100000
#pragma GCC diagnostic pop
#endif
        }
        return result::get(HPX_MOVE(dest));
    }

    ///////////////////////////////////////////////////////////////////////
    // parallel implementation

    // parallel segmented OutIter implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename OutIter, typename T, typename Op, typename Conv>
    static util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_scan_par(ExPolicy const& policy, SegIter first, SegIter last,
        OutIter dest, Conv&& conv, T const& init, Op&& op, std::true_type)
    {
        using result = util::detail::algorithm_result<ExPolicy, OutIter>;

        if (first == last)
            return result::get(HPX_MOVE(dest));

        using traits_in = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator_in = typename traits_in::segment_iterator;
        using local_iterator_type_in = typename traits_in::local_iterator;

        using traits_out = hpx::traits::segmented_iterator_traits<OutIter>;
        using segment_iterator_out = typename traits_out::segment_iterator;
        using local_iterator_type_out = typename traits_out::local_iterator;

        using difference_type =
            typename std::iterator_traits<segment_iterator_in>::difference_type;

        using forced_seq =
            std::integral_constant<bool, !std::forward_iterator<SegIter>>;

        using local_iterator_in_tuple =
            hpx::tuple<local_iterator_type_in, local_iterator_type_in>;

        segment_iterator_in sit_in = traits_in::segment(first);
        segment_iterator_in send_in = traits_in::segment(last);

        segment_iterator_out sit_out = traits_out::segment(dest);

        difference_type count = std::distance(sit_in, send_in);

        std::vector<hpx::shared_future<T>> results;
        std::vector<local_iterator_in_tuple> in_iters;
        std::vector<segment_iterator_out> out_segs;
        std::vector<local_iterator_type_out> out_iters;

        results.reserve(count);
        in_iters.reserve(count);
        out_iters.reserve(count);

        // 1. Step: scan on each partition, push last T of scan into results
        if (sit_in == send_in)
        {
            // all elements on the same partition
            local_iterator_type_in beg = traits_in::local(first);
            local_iterator_type_in end = traits_in::local(last);
            if (beg != end)
            {
                in_iters.push_back(hpx::make_tuple(beg, end));
                out_segs.push_back(sit_out);
                out_iters.push_back(traits_out::local(dest));
                results.push_back(dispatch_async(traits_in::get_id(sit_in),
                    segmented_scan_t<T>(), policy, forced_seq(), beg, end, op,
                    conv));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type_in beg = traits_in::local(first);
            local_iterator_type_in end = traits_in::end(sit_in);

            if (beg != end)
            {
                in_iters.push_back(hpx::make_tuple(beg, end));
                out_segs.push_back(sit_out);
                out_iters.push_back(traits_out::local(dest));
                results.push_back(dispatch_async(traits_in::get_id(sit_in),
                    segmented_scan_t<T>(), policy, forced_seq(), beg, end, op,
                    conv));
            }

            // handle all partitions
            for (++sit_in, ++sit_out; sit_in != send_in; ++sit_in, ++sit_out)
            {
                beg = traits_in::begin(sit_in);
                end = traits_in::end(sit_in);
                if (beg != end)
                {
                    in_iters.push_back(hpx::make_tuple(beg, end));
                    out_segs.push_back(sit_out);
                    out_iters.push_back(traits_out::begin(sit_out));
                    results.push_back(dispatch_async(traits_in::get_id(sit_in),
                        segmented_scan_t<T>(), policy, forced_seq(), beg, end,
                        op, conv));
                }
            }

            // handle the beginning of the last partition
            beg = traits_in::begin(sit_in);
            end = traits_in::local(last);
            if (beg != end)
            {
                in_iters.push_back(hpx::make_tuple(beg, end));
                out_segs.push_back(sit_out);
                out_iters.push_back(traits_out::begin(sit_out));
                results.push_back(dispatch_async(traits_in::get_id(sit_in),
                    segmented_scan_t<T>(), policy, forced_seq(), beg, end, op,
                    conv));
            }
        }

        std::vector<hpx::shared_future<T>> workitems;
        workitems.reserve(results.size() + 1);

        std::vector<hpx::future<void>> finalitems;
        finalitems.reserve(results.size());

        // first init value is the given init value
        workitems.push_back(make_ready_future(init));

        using op_type = std::decay_t<Op>;
        using conv_type = std::decay_t<Conv>;
        auto op_ptr = std::make_shared<op_type>(HPX_FORWARD(Op, op));
        auto conv_ptr = std::make_shared<conv_type>(HPX_FORWARD(Conv, conv));

        std::size_t i = 0;

        for (auto const& res : results)
        {
            using hpx::get;
            segment_iterator_out out_it = out_segs[i];
            local_iterator_type_out out = out_iters[i];
            local_iterator_in_tuple in_tuple = in_iters[i];

            // 2. Step: use the init values to dispatch final scan for each
            // segment performed as soon as the init values are ready wait for
            // 1. step of current partition to prevent race condition when used
            // in place
            finalitems.push_back(hpx::dataflow(policy.executor(),
                hpx::unwrapping([=](T last_value, T) mutable -> void {
                    dispatch(traits_out::get_id(out_it),
                        segmented_scan_void<Algo>(), hpx::execution::seq,
                        std::true_type(), get<0>(in_tuple), get<1>(in_tuple),
                        out, *conv_ptr, last_value, *op_ptr);
                }),
                workitems.back(), res));

            // 3. Step: compute new init value for the next segment
            // performed as soon as the needed results are ready
            workitems.push_back(hpx::dataflow(policy.executor(),
                hpx::unwrapping([=](T lhs, T rhs) {
                    return hpx::invoke(*op_ptr, lhs, rhs);
                }),
                workitems.back(), res));
            ++i;
        }

        OutIter final_dest = dest;
        std::advance(final_dest, std::distance(first, last));

        // wait for all tasks to finish
        return result::get(hpx::dataflow(
            [final_dest](std::vector<hpx::shared_future<T>>&&,
                std::vector<hpx::shared_future<T>>&&,
                std::vector<hpx::future<void>>&&) mutable -> OutIter {
                return final_dest;
            },
            HPX_MOVE(results), HPX_MOVE(workitems), HPX_MOVE(finalitems)));
    }

    // parallel non-segmented OutIter implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename OutIter, typename T, typename Op, typename F1, typename F2>
    static typename util::detail::algorithm_result<ExPolicy, OutIter>::type
    segmented_scan_par_non(ExPolicy const& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, F1&& f1, F2&& f2)
    {
        using result = util::detail::algorithm_result<ExPolicy, OutIter>;

        if (first == last)
            return result::get(HPX_MOVE(dest));

        using traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;
        using difference_type =
            typename std::iterator_traits<segment_iterator>::difference_type;

        using forced_seq =
            std::integral_constant<bool, !std::forward_iterator<SegIter>>;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        difference_type count = std::distance(sit, send);

        using vector_type = std::vector<T>;
        std::vector<hpx::shared_future<vector_type>> results;
        results.reserve(count);

        std::vector<std::size_t> segment_sizes;
        segment_sizes.reserve(count);

        OutIter final_dest = dest;
        std::advance(final_dest, std::distance(first, last));

        // scan on each partition, push whole result vector into results
        if (sit == send)
        {
            // all elements on the same partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::local(last);
            if (beg != end)
            {
                results.push_back(dispatch_async(traits::get_id(sit), Algo(),
                    policy, forced_seq(), beg, end, op));
                segment_sizes.push_back(std::distance(beg, end));
            }
        }
        else
        {
            // handle the remaining part of the first partition
            local_iterator_type beg = traits::local(first);
            local_iterator_type end = traits::end(sit);

            if (beg != end)
            {
                results.push_back(dispatch_async(traits::get_id(sit), Algo(),
                    policy, forced_seq(), beg, end, op));
                segment_sizes.push_back(std::distance(beg, end));
            }

            // handle all partitions
            for (++sit; sit != send; ++sit)
            {
                beg = traits::begin(sit);
                end = traits::end(sit);
                if (beg != end)
                {
                    results.push_back(dispatch_async(traits::get_id(sit),
                        Algo(), policy, forced_seq(), beg, end, op));
                    segment_sizes.push_back(std::distance(beg, end));
                }
            }

            // handle the beginning of the last partition
            beg = traits::begin(sit);
            end = traits::local(last);
            if (beg != end)
            {
                results.push_back(dispatch_async(traits::get_id(sit), Algo(),
                    policy, forced_seq(), beg, end, op));
                segment_sizes.push_back(std::distance(beg, end));
            }
        }

        std::vector<hpx::shared_future<T>> workitems;
        workitems.reserve(results.size() + 1);

        std::vector<hpx::future<void>> finalitems;
        finalitems.reserve(results.size());

        workitems.push_back(make_ready_future(init));

        using op_type = std::decay_t<Op>;
        using f1_type = std::decay_t<F1>;
        using f2_type = std::decay_t<F2>;
        auto op_ptr = std::make_shared<op_type>(HPX_FORWARD(Op, op));
        auto f1_ptr = std::make_shared<f1_type>(HPX_FORWARD(F1, f1));
        auto f2_ptr = std::make_shared<f2_type>(HPX_FORWARD(F2, f2));

        std::size_t segment_index = 0;

        for (auto const& res : results)
        {
            // collect all results with updated init values
            finalitems.push_back(hpx::dataflow(policy.executor(),
                hpx::unwrapping([=](T last_value, vector_type r) mutable {
                    // merge function
                    hpx::invoke(
                        *f1_ptr, r.begin(), r.end(), dest, last_value, *op_ptr);
                }),
                workitems.back(), res));

            std::advance(dest, segment_sizes[segment_index++]);

            // propagate results from left to right
            // new init value is most right value combined with old init
            workitems.push_back(hpx::dataflow(policy.executor(),
                hpx::unwrapping([=](T last_value, vector_type v) {
                    return hpx::invoke(
                        *op_ptr, last_value, hpx::invoke(*f2_ptr, HPX_MOVE(v)));
                }),
                workitems.back(), res));
        }

        // wait for all tasks to finish
        return result::get(hpx::dataflow(
            [final_dest](std::vector<hpx::shared_future<vector_type>>&&,
                std::vector<hpx::shared_future<T>>&&,
                std::vector<hpx::future<void>>&&) mutable -> OutIter {
                return final_dest;
            },
            HPX_MOVE(results), HPX_MOVE(workitems), HPX_MOVE(finalitems)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail
