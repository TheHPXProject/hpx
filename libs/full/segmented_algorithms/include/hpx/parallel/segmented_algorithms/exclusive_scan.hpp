//  Copyright (c) 2016 Minh-Khanh Do
//  Copyright (c) 2020-2026 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/segmented_algorithms/exclusive_scan.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>
#include <hpx/parallel/segmented_algorithms/detail/scan.hpp>

#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented exclusive_scan

    /// \cond NOINTERNAL

    struct merge_exclusive_scan
    {
        // adds init to each element except for the first one
        template <typename InIter, typename OutIter, typename T, typename Op>
        OutIter operator()(
            InIter first, InIter last, OutIter dest, T init, Op&& op)
        {
            *dest = init;
            for (++first, ++dest; first != last; (void) ++first, ++dest)
            {
                *dest = op(init, *first);
            }
            return dest;
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // do exclusive scan returns result as vector
    // first element of result vector is last T of scan
    // (can be used to transfer to the next partition)
    //
    // first element can be used because it will be
    // overwritten by the last T of the previous partition
    template <typename Value>
    struct segmented_exclusive_scan_vector
      : detail::algorithm<segmented_exclusive_scan_vector<Value>, Value>
    {
        typedef Value vector_type;

        constexpr segmented_exclusive_scan_vector() noexcept
          : segmented_exclusive_scan_vector::algorithm(
                "segmented_exclusive_scan_vector")
        {
        }

        template <typename ExPolicy, typename InIter, typename Op>
        static vector_type sequential(
            ExPolicy&& policy, InIter first, InIter last, Op&& op)
        {
            vector_type result(std::distance(first, last));

            // use first element to save the last T of scan
            if (result.size() != 0)
            {
                exclusive_scan<typename vector_type::iterator>().sequential(
                    HPX_FORWARD(ExPolicy, policy), first + 1, last,
                    result.begin() + 1, *first, op);
                result[0] = op(result.back(), *(last - 1));
            }
            return result;
        }

        template <typename ExPolicy, typename FwdIter, typename Op>
        static util::detail::algorithm_result_t<ExPolicy, vector_type> parallel(
            ExPolicy&& /* policy */, FwdIter first, FwdIter last, Op&& op)
        {
            using result =
                util::detail::algorithm_result<ExPolicy, vector_type>;

            vector_type res(std::distance(first, last));

            // use first element to save last T of scan
            return result::get(hpx::dataflow(
                [=](vector_type r) {
                    exclusive_scan<typename vector_type::iterator>().parallel(
                        hpx::execution::par, first + 1, last, r.begin() + 1,
                        *first, op);
                    r[0] = op(r.back(), *(last - 1));
                    return r;
                },
                HPX_MOVE(res)));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // sequential implementation

    // sequential segmented OutIter implementation
    template <typename ExPolicy, typename SegIter, typename OutIter, typename T,
        typename Op, typename Conv>
    util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_exclusive_scan_seq(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::true_type, Conv&& conv)
    {
        using traits_out = hpx::traits::segmented_iterator_traits<OutIter>;
        return segmented_scan_seq<
            transform_exclusive_scan<typename traits_out::local_raw_iterator>>(
            HPX_FORWARD(ExPolicy, policy), first, last, dest,
            HPX_FORWARD(Conv, conv), init, HPX_FORWARD(Op, op),
            std::true_type());
    }

    // sequential non segmented OutIter implementation
    template <typename ExPolicy, typename SegIter, typename OutIter, typename T,
        typename Op, typename Conv>
    util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_exclusive_scan_seq(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::false_type,
        Conv&& /* conv */)
    {
        using vector_type = std::vector<T>;
        return segmented_scan_seq_non<
            segmented_exclusive_scan_vector<vector_type>>(
            HPX_FORWARD(ExPolicy, policy), first, last, dest, init,
            HPX_FORWARD(Op, op), merge_exclusive_scan(),
            // new init value is first element from
            // segmented_exclusive_scan_vector + last init value
            [op](vector_type v, T val) { return op(v.front(), val); });
    }

    ///////////////////////////////////////////////////////////////////////
    // parallel implementation

    // parallel segmented OutIter implementation
    template <typename ExPolicy, typename SegIter, typename OutIter, typename T,
        typename Op, typename Conv>
    util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_exclusive_scan_par(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::true_type, Conv&& conv)
    {
        using traits_out = hpx::traits::segmented_iterator_traits<OutIter>;
        return segmented_scan_par<
            transform_exclusive_scan<typename traits_out::local_raw_iterator>>(
            HPX_FORWARD(ExPolicy, policy), first, last, dest,
            HPX_FORWARD(Conv, conv), init, HPX_FORWARD(Op, op),
            std::true_type());
    }

    // parallel non-segmented OutIter implementation
    template <typename ExPolicy, typename SegIter, typename OutIter, typename T,
        typename Op, typename Conv>
    util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_exclusive_scan_par(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::false_type,
        Conv&& /* conv */)
    {
        using vector_type = std::vector<T>;
        return segmented_scan_par_non<
            segmented_exclusive_scan_vector<vector_type>>(
            HPX_FORWARD(ExPolicy, policy), first, last, dest, init,
            HPX_FORWARD(Op, op), merge_exclusive_scan(),
            // last T of scan is on the front
            // see segmented_exclusive_scan_vector
            [](vector_type v) { return v.front(); });
    }

    ///////////////////////////////////////////////////////////////////////
    // sequential remote implementation
    template <typename ExPolicy, typename SegIter, typename OutIter, typename T,
        typename Op, typename Conv>
    util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_exclusive_scan(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::true_type, Conv&& conv)
    {
        using is_out_seg = typename hpx::traits::segmented_iterator_traits<
            OutIter>::is_segmented_iterator;

        // check if OutIter is segmented in the same way as SegIter
        if (is_segmented_the_same(first, last, dest, is_out_seg()))
        {
            return segmented_exclusive_scan_seq(HPX_FORWARD(ExPolicy, policy),
                first, last, dest, init, HPX_FORWARD(Op, op), is_out_seg(),
                HPX_FORWARD(Conv, conv));
        }

        return segmented_exclusive_scan_seq(HPX_FORWARD(ExPolicy, policy),
            first, last, dest, init, HPX_FORWARD(Op, op), std::false_type(),
            HPX_FORWARD(Conv, conv));
    }

    ///////////////////////////////////////////////////////////////////////
    // parallel remote implementation
    template <typename ExPolicy, typename SegIter, typename OutIter, typename T,
        typename Op, typename Conv>
    util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_exclusive_scan(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::false_type, Conv&& conv)
    {
        using is_out_seg = typename hpx::traits::segmented_iterator_traits<
            OutIter>::is_segmented_iterator;

        if (is_segmented_the_same(first, last, dest, is_out_seg()))
        {
            return segmented_exclusive_scan_par(HPX_FORWARD(ExPolicy, policy),
                first, last, dest, init, HPX_FORWARD(Op, op), is_out_seg(),
                HPX_FORWARD(Conv, conv));
        }

        return segmented_exclusive_scan_par(HPX_FORWARD(ExPolicy, policy),
            first, last, dest, init, HPX_FORWARD(Op, op), std::false_type(),
            HPX_FORWARD(Conv, conv));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename InIter, typename OutIter, typename T,
        typename Op = std::plus<T>>
        requires(hpx::traits::is_iterator_v<InIter> &&
            hpx::traits::is_segmented_iterator_v<InIter> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter> &&
            hpx::is_invocable_v<Op,
                typename std::iterator_traits<InIter>::value_type,
                typename std::iterator_traits<InIter>::value_type>)
    OutIter tag_invoke(hpx::exclusive_scan_t, InIter first, InIter last,
        OutIter dest, T init, Op&& op = Op())
    {
        static_assert(
            std::input_iterator<InIter>, "Requires at least input iterator.");

        static_assert(
            std::output_iterator<OutIter, hpx::traits::iter_value_t<InIter>>,
            "Requires at least output iterator.");

        if (first == last)
            return dest;

        return hpx::parallel::detail::segmented_exclusive_scan(
            hpx::execution::seq, first, last, dest, HPX_MOVE(init),
            HPX_FORWARD(Op, op), std::true_type{}, hpx::identity_v);
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename T, typename Op = std::plus<T>>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_segmented_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2> &&
            hpx::traits::is_segmented_iterator_v<FwdIter2> &&
            hpx::is_invocable_v<Op,
                typename std::iterator_traits<FwdIter1>::value_type,
                typename std::iterator_traits<FwdIter1>::value_type>)
    parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2> tag_invoke(
        hpx::exclusive_scan_t, ExPolicy&& policy, FwdIter1 first, FwdIter1 last,
        FwdIter2 dest, T init, Op&& op = Op())
    {
        static_assert(std::forward_iterator<FwdIter1>,
            "Requires at least forward iterator.");

        static_assert(std::forward_iterator<FwdIter2>,
            "Requires at least forward iterator.");

        if (first == last)
            return parallel::util::detail::algorithm_result<ExPolicy,
                FwdIter2>::get(HPX_MOVE(dest));

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        return hpx::parallel::detail::segmented_exclusive_scan(
            HPX_FORWARD(ExPolicy, policy), first, last, dest, HPX_MOVE(init),
            HPX_FORWARD(Op, op), is_seq(), hpx::identity_v);
    }
}    // namespace hpx::segmented
