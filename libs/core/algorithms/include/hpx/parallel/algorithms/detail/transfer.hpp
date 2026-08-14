//  Copyright (c) 2016 Minh-Khanh Do
//  Copyright (c) 2016-2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/iterator_support.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/algorithms/traits/segmented_iterator_traits.hpp>
#endif

#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // transfer
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename FwdIter, typename OutIter>
        struct iterators_are_segmented
          : std::integral_constant<bool,
                hpx::traits::segmented_iterator_traits<
                    FwdIter>::is_segmented_iterator::value &&
                    hpx::traits::segmented_iterator_traits<
                        OutIter>::is_segmented_iterator::value>
        {
        };

        HPX_CXX_CORE_EXPORT template <typename FwdIter, typename OutIter>
        struct iterators_are_not_segmented
          : std::integral_constant<bool,
                !hpx::traits::segmented_iterator_traits<
                    FwdIter>::is_segmented_iterator::value &&
                    !hpx::traits::segmented_iterator_traits<
                        OutIter>::is_segmented_iterator::value>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        // parallel version
        HPX_CXX_CORE_EXPORT template <typename Algo, typename ExPolicy,
            typename FwdIter1, typename Sent1, typename FwdIter2>
        decltype(auto) transfer_(ExPolicy&& policy, FwdIter1 first, Sent1 last,
            FwdIter2 dest, std::false_type)
        {
            return Algo().call(
                HPX_FORWARD(ExPolicy, policy), first, last, dest);
        }

#if !defined(HPX_COMPUTE_DEVICE_CODE)
        // forward declare segmented version
        template <typename Algo, typename ExPolicy, typename FwdIter1,
            typename Sent1, typename FwdIter2>
        util::detail::algorithm_result_t<ExPolicy,
            util::in_out_result<FwdIter1, FwdIter2>>
        transfer_(ExPolicy&& policy, FwdIter1 first, Sent1 last, FwdIter2 dest,
            std::true_type);
#endif

        // Executes transfer algorithm on the elements in the range [first, last),
        // to another range beginning at \a dest.
        //
        // \note   Complexity: Performs exactly \a last - \a first transfer assignments.
        //
        //
        // \tparam Algo        The algorithm that is used to transfer the elements.
        //                     Should be hpx::parallel::detail::copy or
        //                     hpx::parallel::detail::move.
        // \tparam ExPolicy    The type of the execution policy to use (deduced).
        //                     It describes the manner in which the execution
        //                     of the algorithm may be parallelized and the manner
        //                     in which it executes the move assignments.
        // \tparam FwdIter1    The type of the source iterators used (deduced).
        //                     This iterator type must meet the requirements of an
        //                     forward iterator.
        // \tparam FwdIter2    The type of the iterator representing the
        //                     destination range (deduced).
        //                     This iterator type must meet the requirements of an
        //                     output iterator.
        //
        // \param policy       The execution policy to use for the scheduling of
        //                     the iterations.
        // \param first        Refers to the beginning of the sequence of elements
        //                     the algorithm will be applied to.
        // \param last         Refers to the end of the sequence of elements the
        //                     algorithm will be applied to.
        // \param dest         Refers to the beginning of the destination range.
        //
        // \returns  The \a transfer algorithm returns a \a hpx::future<FwdIter2> if
        //           the execution policy is of type
        //           \a sequenced_task_policy or
        //           \a parallel_task_policy and
        //           returns \a FwdIter2 otherwise.
        //           The \a move algorithm returns the output iterator to the
        //           element in the destination range, one past the last element
        //           transferred.
        //
        HPX_CXX_CORE_EXPORT template <typename Algo, typename ExPolicy,
            typename FwdIter1, typename Sent1, typename FwdIter2>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent1, FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        decltype(auto) transfer(
            ExPolicy&& policy, FwdIter1 first, Sent1 last, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2> ||
                    (hpx::is_sequenced_execution_policy_v<ExPolicy> &&
                        std::output_iterator<
                            hpx::traits::iter_value_t<FwdIter1>, FwdIter2>),
                "Requires at least forward iterator or sequential execution.");

#if defined(HPX_COMPUTE_DEVICE_CODE)
            return transfer_<Algo>(HPX_FORWARD(ExPolicy, policy), first, last,
                dest, std::false_type());
#else
            using is_segmented = hpx::traits::is_segmented_iterator<FwdIter1>;

            return transfer_<Algo>(HPX_FORWARD(ExPolicy, policy), first, last,
                dest, is_segmented());
#endif
        }
    }    // namespace detail
}    // namespace hpx::parallel
