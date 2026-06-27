//  Copyright (c) 2016 Minh-Khanh Do
//  Copyright (c) 2020-2026 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/segmented_algorithms/inclusive_scan.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>
#include <hpx/parallel/segmented_algorithms/detail/scan.hpp>

#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented inclusive_scan

    /// \cond NOINTERNAL

    // adds init to each element from first to dest
    struct merge_inclusive_scan
    {
        template <typename InIter, typename OutIter, typename T, typename Op>
        OutIter operator()(
            InIter first, InIter last, OutIter dest, T init, Op&& op)
        {
            for (/* */; first != last; (void) ++first, ++dest)
            {
                *dest = op(init, *first);
            }
            return dest;
        }
    };

    ///////////////////////////////////////////////////////////////////////

    // do inclusive scan returns result as vector
    template <typename Value>
    struct segmented_inclusive_scan_vector
      : public detail::algorithm<segmented_inclusive_scan_vector<Value>, Value>
    {
        using vector_type = Value;

        constexpr segmented_inclusive_scan_vector() noexcept
          : segmented_inclusive_scan_vector::algorithm(
                "segmented_inclusive_scan_vector")
        {
        }

        template <typename ExPolicy, typename InIter, typename Op>
        static vector_type sequential(
            ExPolicy&& policy, InIter first, InIter last, Op&& op)
        {
            vector_type result(std::distance(first, last));

            // use first element as init value for inclusive_scan
            if (result.size() != 0)
            {
                result[0] = *first;
                inclusive_scan<typename vector_type::iterator>().sequential(
                    HPX_FORWARD(ExPolicy, policy), first + 1, last,
                    result.begin() + 1, *first, HPX_FORWARD(Op, op));
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

            // use first element as the init value for inclusive_scan
            if (res.size() != 0)
            {
                res[0] = *first;
            }

            return result::get(dataflow(
                [=](vector_type r) {
                    inclusive_scan<typename vector_type::iterator>().parallel(
                        hpx::execution::par, first + 1, last, r.begin() + 1,
                        *first, op);
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
    segmented_inclusive_scan_seq(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::true_type, Conv&& conv)
    {
        using traits_out = hpx::traits::segmented_iterator_traits<OutIter>;

        return segmented_scan_seq<
            transform_inclusive_scan<typename traits_out::local_raw_iterator>>(
            HPX_FORWARD(ExPolicy, policy), first, last, dest,
            HPX_FORWARD(Conv, conv), init, HPX_FORWARD(Op, op),
            std::true_type());
    }

    // sequential non-segmented OutIter implementation
    template <typename ExPolicy, typename SegIter, typename OutIter, typename T,
        typename Op, typename Conv>
    util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_inclusive_scan_seq(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::false_type,
        Conv&& /* conv */)
    {
        using vector_type = std::vector<T>;

        return segmented_scan_seq_non<
            segmented_inclusive_scan_vector<vector_type>>(
            HPX_FORWARD(ExPolicy, policy), first, last, dest, init,
            HPX_FORWARD(Op, op), merge_inclusive_scan(),
            // new init value is last element from
            // segmented_inclusive_scan_vector + last init value
            [op](vector_type v, T val) { return op(v.back(), val); });
    }

    ///////////////////////////////////////////////////////////////////////
    // parallel implementation

    // parallel segmented OutIter implementation
    template <typename ExPolicy, typename SegIter, typename OutIter, typename T,
        typename Op, typename Conv>
    util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_inclusive_scan_par(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::true_type, Conv&& conv)
    {
        using traits_out = hpx::traits::segmented_iterator_traits<OutIter>;

        return segmented_scan_par<
            transform_inclusive_scan<typename traits_out::local_raw_iterator>>(
            HPX_FORWARD(ExPolicy, policy), first, last, dest,
            HPX_FORWARD(Conv, conv), init, HPX_FORWARD(Op, op),
            std::true_type());
    }

    // parallel non-segmented OutIter implementation
    template <typename ExPolicy, typename SegIter, typename OutIter, typename T,
        typename Op, typename Conv>
    util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_inclusive_scan_par(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::false_type, Conv&&)
    {
        using vector_type = std::vector<T>;

        return segmented_scan_par_non<
            segmented_inclusive_scan_vector<vector_type>>(
            HPX_FORWARD(ExPolicy, policy), first, last, dest, init,
            HPX_FORWARD(Op, op), merge_inclusive_scan(),
            // last T of scan is in the back
            [](vector_type v) { return v.back(); });
    }

    ///////////////////////////////////////////////////////////////////////
    // sequential remote implementation
    template <typename ExPolicy, typename SegIter, typename OutIter, typename T,
        typename Op, typename Conv>
    util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_inclusive_scan(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::true_type, Conv&& conv)
    {
        using is_out_seg = typename hpx::traits::segmented_iterator_traits<
            OutIter>::is_segmented_iterator;

        // check if OutIter is segmented in the same way as SegIter
        // NOLINTNEXTLINE(bugprone-branch-clone)
        if (is_segmented_the_same(first, last, dest, is_out_seg()))
        {
            return segmented_inclusive_scan_seq(HPX_FORWARD(ExPolicy, policy),
                first, last, dest, init, HPX_FORWARD(Op, op), is_out_seg(),
                HPX_FORWARD(Conv, conv));
        }
        else
        {
            return segmented_inclusive_scan_seq(HPX_FORWARD(ExPolicy, policy),
                first, last, dest, init, HPX_FORWARD(Op, op), std::false_type(),
                HPX_FORWARD(Conv, conv));
        }
    }

    ///////////////////////////////////////////////////////////////////////
    // parallel remote implementation
    template <typename ExPolicy, typename SegIter, typename OutIter, typename T,
        typename Op, typename Conv>
    util::detail::algorithm_result_t<ExPolicy, OutIter>
    segmented_inclusive_scan(ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest, T const& init, Op&& op, std::false_type, Conv&& conv)
    {
        using is_out_seg = typename hpx::traits::segmented_iterator_traits<
            OutIter>::is_segmented_iterator;

        // check if OutIter is segmented in the same way as SegIter
        // NOLINTNEXTLINE(bugprone-branch-clone)
        if (is_segmented_the_same(first, last, dest, is_out_seg()))
        {
            return segmented_inclusive_scan_par(HPX_FORWARD(ExPolicy, policy),
                first, last, dest, init, HPX_FORWARD(Op, op), is_out_seg(),
                HPX_FORWARD(Conv, conv));
        }
        else
        {
            return segmented_inclusive_scan_par(HPX_FORWARD(ExPolicy, policy),
                first, last, dest, init, HPX_FORWARD(Op, op), std::false_type(),
                HPX_FORWARD(Conv, conv));
        }
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename InIter, typename OutIter,
        typename Op = std::plus<>>
        requires(hpx::traits::is_iterator_v<InIter> &&
            hpx::traits::is_segmented_iterator_v<InIter> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    OutIter tag_invoke(hpx::inclusive_scan_t, InIter first, InIter last,
        OutIter dest, Op&& op = Op())
    {
        static_assert(
            std::input_iterator<InIter>, "Requires at least input iterator.");

        static_assert(
            std::output_iterator<OutIter, hpx::traits::iter_value_t<InIter>>,
            "Requires at least output iterator.");

        if (first == last)
            return dest;

        using value_type = typename std::iterator_traits<InIter>::value_type;

        return hpx::parallel::detail::segmented_inclusive_scan(
            hpx::execution::seq, first, last, dest, value_type{},
            HPX_FORWARD(Op, op), std::true_type{}, hpx::identity_v);
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename Op = std::plus<>>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_segmented_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2> &&
            hpx::traits::is_segmented_iterator_v<FwdIter2>)
    parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2> tag_invoke(
        hpx::inclusive_scan_t, ExPolicy&& policy, FwdIter1 first, FwdIter1 last,
        FwdIter2 dest, Op&& op = Op())
    {
        static_assert(std::forward_iterator<FwdIter1>,
            "Requires at least forward iterator.");

        static_assert(std::forward_iterator<FwdIter2>,
            "Requires at least forward iterator.");

        if (first == last)
            return parallel::util::detail::algorithm_result<ExPolicy,
                FwdIter2>::get(HPX_MOVE(dest));

        using value_type = typename std::iterator_traits<FwdIter1>::value_type;
        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        return hpx::parallel::detail::segmented_inclusive_scan(
            HPX_FORWARD(ExPolicy, policy), first, last, dest, value_type{},
            HPX_FORWARD(Op, op), is_seq(), hpx::identity_v);
    }

    HPX_CXX_EXPORT template <typename InIter, typename OutIter, typename Op,
        typename T>
        requires(hpx::traits::is_iterator_v<InIter> &&
            hpx::traits::is_segmented_iterator_v<InIter> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    OutIter tag_invoke(hpx::inclusive_scan_t, InIter first, InIter last,
        OutIter dest, Op&& op, T&& init)
    {
        static_assert(
            std::input_iterator<InIter>, "Requires at least input iterator.");

        static_assert(
            std::output_iterator<OutIter, hpx::traits::iter_value_t<InIter>>,
            "Requires at least output iterator.");

        if (first == last)
            return dest;

        return hpx::parallel::detail::segmented_inclusive_scan(
            hpx::execution::seq, first, last, dest, HPX_FORWARD(T, init),
            HPX_FORWARD(Op, op), std::true_type{}, hpx::identity_v);
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename Op, typename T>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_segmented_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2> &&
            hpx::traits::is_segmented_iterator_v<FwdIter2>)
    parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2> tag_invoke(
        hpx::inclusive_scan_t, ExPolicy&& policy, FwdIter1 first, FwdIter1 last,
        FwdIter2 dest, Op&& op, T&& init)
    {
        static_assert(std::forward_iterator<FwdIter1>,
            "Requires at least forward iterator.");

        static_assert(std::forward_iterator<FwdIter2>,
            "Requires at least forward iterator.");

        if (first == last)
            return parallel::util::detail::algorithm_result<ExPolicy,
                FwdIter2>::get(HPX_MOVE(dest));

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        return hpx::parallel::detail::segmented_inclusive_scan(
            HPX_FORWARD(ExPolicy, policy), first, last, dest,
            HPX_FORWARD(T, init), HPX_FORWARD(Op, op), is_seq(),
            hpx::identity_v);
    }
}    // namespace hpx::segmented
