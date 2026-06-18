//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/ends_with.hpp
/// \page hpx::ends_with
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    // clang-format off

    /// Checks whether the second range defined by [first1, last1) matches the
    /// suffix of the first range defined by [first2, last2)
    ///
    /// \note   Complexity: Linear: at most min(N1, N2) applications of the
    ///                     predicate.
    ///
    /// \tparam InIter1     The type of the begin source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an input iterator.
    /// \tparam InIter2     The type of the begin destination iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of a input iterator.
    /// \tparam Pred        The binary predicate that compares the elements.
    ///                     This defaults to \a hpx::parallel::detail::equal_to.
    ///
    /// \param first1       Refers to the beginning of the source range.
    /// \param last1        Refers to the end of the source range.
    /// \param first2       Refers to the beginning of the destination range.
    /// \param last2        Refers to the end of the destination range.
    /// \param pred         Specifies the binary predicate function
    ///                     (or function object) which will be invoked for
    ///                     comparison of the elements in the two ranges.
    ///
    /// The assignments in the parallel \a ends_with algorithm invoked
    /// without an execution policy object execute in sequential order
    /// in the calling thread.
    ///
    /// \returns  The \a ends_with algorithm returns \a bool.
    ///           The \a ends_with algorithm returns a boolean with the
    ///           value true if the second range matches the suffix of the
    ///           first range, false otherwise.
    template <typename InIter1, typename InIter2,
        typename Pred = hpx::parallel::detail::equal_to>
    bool ends_with(InIter1 first1, InIter1 last1, InIter2 first2,
        InIter2 last2, Pred&& pred = Pred());

    /// Checks whether the second range defined by [first1, last1) matches the
    /// suffix of the first range defined by [first2, last2). Executed
    /// according to the policy.
    ///
    /// \note   Complexity: Linear: at most min(N1, N2) applications of the
    ///                     predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the begin source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an forward iterator.
    /// \tparam FwdIter2    The type of the begin destination iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of a forward iterator.
    /// \tparam Pred        The binary predicate that compares the elements.
    ///                     This defaults to \a ranges::equal_to.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first1       Refers to the beginning of the source range.
    /// \param last1        Refers to the end of the source range.
    /// \param first2       Refers to the beginning of the destination range.
    /// \param last2        Refers to the end of the destination range.
    /// \param pred         Specifies the binary predicate function
    ///                     (or function object) which will be invoked for
    ///                     comparison of the elements in the two ranges.
    ///
    /// The assignments in the parallel \a ends_with algorithm invoked with an
    /// execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a ends_with algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a ends_with algorithm returns a
    ///           \a hpx::future<bool> if the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy and
    ///           returns \a bool otherwise.
    ///           The \a ends_with algorithm returns a boolean with the
    ///           value true if the second range matches the suffix of the
    ///           first range, false otherwise.
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename Pred = ranges::equal_to>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        bool>::type
    ends_with(ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1,
        FwdIter2 first2, FwdIter2 last2, Pred&& pred = Pred());

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/equal.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // ends_with
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT struct ends_with : public algorithm<ends_with, bool>
        {
            constexpr ends_with() noexcept
              : algorithm("ends_with")
            {
            }

            template <typename ExPolicy, typename Iter1, typename Sent1,
                typename Iter2, typename Sent2, typename Pred, typename Proj1,
                typename Proj2>
            static constexpr bool sequential(ExPolicy, Iter1 first1,
                Sent1 last1, Iter2 first2, Sent2 last2, Pred&& pred,
                Proj1&& proj1, Proj2&& proj2)
            {
                auto distance1 = detail::distance(first1, last1);
                auto distance2 = detail::distance(first2, last2);

                if (distance1 < distance2)
                    return false;

                std::advance(first1, distance1 - distance2);

                return hpx::parallel::detail::equal_binary().call(
                    hpx::execution::seq, HPX_MOVE(first1), HPX_MOVE(last1),
                    HPX_MOVE(first2), HPX_MOVE(last2), HPX_FORWARD(Pred, pred),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
            }

            template <typename ExPolicy, typename FwdIter1, typename Sent1,
                typename FwdIter2, typename Sent2, typename Pred,
                typename Proj1, typename Proj2>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter1 first1,
                Sent1 last1, FwdIter2 first2, Sent2 last2, Pred&& pred,
                Proj1&& proj1, Proj2&& proj2)
            {
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                auto distance1 = detail::distance(first1, last1);
                auto distance2 = detail::distance(first2, last2);
                auto diff = distance1 - distance2;

                if (distance1 < distance2)
                {
                    if constexpr (has_scheduler_executor)
                    {
                        diff = 0;
                    }
                    else
                    {
                        return util::detail::algorithm_result<ExPolicy,
                            bool>::get(false);
                    }
                }

                std::advance(first1, diff);

                return hpx::parallel::detail::equal_binary().call(
                    HPX_FORWARD(ExPolicy, policy), HPX_MOVE(first1),
                    HPX_MOVE(last1), HPX_MOVE(first2), HPX_MOVE(last2),
                    HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj1, proj1),
                    HPX_FORWARD(Proj2, proj2));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ends_with
    HPX_CXX_CORE_EXPORT inline constexpr struct ends_with_t final
      : hpx::detail::tag_parallel_algorithm<ends_with_t>
    {
    private:
        template <typename InIter1, typename InIter2,
            typename Pred = hpx::parallel::detail::equal_to>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter1> &&
                hpx::traits::is_iterator_v<InIter2> &&
                hpx::is_invocable_v<Pred,
                    hpx::traits::iter_value_t<InIter1>,
                    hpx::traits::iter_value_t<InIter2>
                >
            )
        // clang-format on
        friend bool tag_fallback_invoke(hpx::ends_with_t, InIter1 first1,
            InIter1 last1, InIter2 first2, InIter2 last2, Pred pred = Pred())
        {
            static_assert(std::input_iterator<InIter1>,
                "Required at least input iterator.");

            static_assert(std::input_iterator<InIter2>,
                "Required at least input iterator.");

            return hpx::parallel::detail::ends_with().call(hpx::execution::seq,
                first1, last1, first2, last2, HPX_MOVE(pred), hpx::identity_v,
                hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename Pred = ranges::equal_to>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    hpx::traits::iter_value_t<FwdIter1>,
                    hpx::traits::iter_value_t<FwdIter2>
                >
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::ends_with_t,
            ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1, FwdIter2 first2,
            FwdIter2 last2, Pred pred = Pred())
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::ends_with().call(
                HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2,
                HPX_MOVE(pred), hpx::identity_v, hpx::identity_v);
        }
    } ends_with{};
}    // namespace hpx

#endif    // DOXYGEN
