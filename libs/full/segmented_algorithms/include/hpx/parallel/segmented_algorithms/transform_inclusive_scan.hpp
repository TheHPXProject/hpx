//  Copyright (c) 2017 Ajai V George
//  Copyright (c) 2021 Akhil J Nair
//  Copyright (c) 2024-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/segmented_algorithms/transform_inclusive_scan.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>
#include <hpx/parallel/segmented_algorithms/detail/scan.hpp>
#include <hpx/parallel/segmented_algorithms/inclusive_scan.hpp>

#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename InIter, typename OutIter, typename Op,
        typename Conv>
        requires(hpx::traits::is_iterator_v<InIter> &&
            hpx::traits::is_segmented_iterator_v<InIter> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    OutIter tag_invoke(hpx::transform_inclusive_scan_t, InIter first,
        InIter last, OutIter dest, Op&& op, Conv&& conv)
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
            HPX_FORWARD(Op, op), std::true_type{}, HPX_FORWARD(Conv, conv));
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename Op, typename Conv>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_segmented_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2> &&
            hpx::traits::is_segmented_iterator_v<FwdIter2>)
    parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2> tag_invoke(
        hpx::transform_inclusive_scan_t, ExPolicy&& policy, FwdIter1 first,
        FwdIter1 last, FwdIter2 dest, Op&& op, Conv&& conv)
    {
        static_assert(std::forward_iterator<FwdIter1>,
            "Requires at least forward iterator.");

        static_assert(std::forward_iterator<FwdIter2>,
            "Requires at least forward iterator.");

        if (first == last)
            return parallel::util::detail::algorithm_result<ExPolicy,
                FwdIter2>::get(HPX_MOVE(dest));

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
        using value_type = typename std::iterator_traits<FwdIter1>::value_type;

        return hpx::parallel::detail::segmented_inclusive_scan(
            HPX_FORWARD(ExPolicy, policy), first, last, dest, value_type{},
            HPX_FORWARD(Op, op), is_seq(), HPX_FORWARD(Conv, conv));
    }

    HPX_CXX_EXPORT template <typename InIter, typename OutIter, typename T,
        typename Op, typename Conv>
        requires(hpx::traits::is_iterator_v<InIter> &&
            hpx::traits::is_segmented_iterator_v<InIter> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    OutIter tag_invoke(hpx::transform_inclusive_scan_t, InIter first,
        InIter last, OutIter dest, Op&& op, Conv&& conv, T init)
    {
        static_assert(
            std::input_iterator<InIter>, "Requires at least input iterator.");

        static_assert(
            std::output_iterator<OutIter, hpx::traits::iter_value_t<InIter>>,
            "Requires at least output iterator.");

        if (first == last)
            return dest;

        return hpx::parallel::detail::segmented_inclusive_scan(
            hpx::execution::seq, first, last, dest, HPX_MOVE(init),
            HPX_FORWARD(Op, op), std::true_type{}, HPX_FORWARD(Conv, conv));
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename T, typename Op, typename Conv>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_segmented_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2> &&
            hpx::traits::is_segmented_iterator_v<FwdIter2>)
    parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2> tag_invoke(
        hpx::transform_inclusive_scan_t, ExPolicy&& policy, FwdIter1 first,
        FwdIter1 last, FwdIter2 dest, Op&& op, Conv&& conv, T init)
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
            HPX_FORWARD(ExPolicy, policy), first, last, dest, HPX_MOVE(init),
            HPX_FORWARD(Op, op), is_seq(), HPX_FORWARD(Conv, conv));
    }
}    // namespace hpx::segmented
