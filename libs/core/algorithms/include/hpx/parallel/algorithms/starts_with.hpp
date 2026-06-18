//  Copyright (c) 2022 Bhumit Attarde
//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/starts_with.hpp
/// \page hpx::starts_with
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    // clang-format off

    /// Checks whether the second range defined by [first1, last1) matches the
    /// prefix of the first range defined by [first2, last2)
    ///
    /// \note   Complexity: Linear: at most min(N1, N2) applications of the
    ///                     predicate.
    ///
    /// \tparam InIter1     The type of the source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an input iterator.
    /// \tparam InIter2     The type of the destination iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an input iterator.
    /// \tparam Pred        The binary predicate that compares the elements.
    ///                     This defaults to \a hpx::parallel::detail::equal_to.
    ///
    /// \param first1       Refers to the beginning of the source range.
    /// \param last1        Sentinel value referring to the end of the source
    ///                     range.
    /// \param first2       Refers to the beginning of the destination range.
    /// \param last2        Sentinel value referring to the end of the
    ///                     destination range.
    /// \param pred         Specifies the binary predicate function
    ///                     (or function object) which will be invoked for
    ///                     comparison of the elements in the two ranges.
    ///
    /// The assignments in the parallel \a starts_with algorithm invoked
    /// without an execution policy object execute in sequential order
    /// in the calling thread.
    ///
    /// \returns  The \a starts_with algorithm returns \a bool.
    ///           The \a starts_with algorithm returns a boolean with the
    ///           value true if the second range matches the prefix of the
    ///           first range, false otherwise.
    template <typename InIter1, typename InIter2,
        typename Pred = hpx::parallel::detail::equal_to>
    bool starts_with(InIter1 first1, InIter1 last1, InIter2 first2,
        InIter2 last2, Pred&& pred = Pred());

    /// Checks whether the second range defined by [first1, last1) matches the
    /// prefix of the first range defined by [first2, last2).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: Linear: at most min(N1, N2) applications of the
    ///                     predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam InIter1     The type of the source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an input iterator.
    /// \tparam InIter2     The type of the destination iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an input iterator.
    /// \tparam Pred        The binary predicate that compares the elements.
    ///                     This defaults to \a ranges::equal_to.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first1       Refers to the beginning of the source range.
    /// \param last1        Sentinel value referring to the end of the source
    ///                     range.
    /// \param first2       Refers to the beginning of the destination range.
    /// \param last2        Sentinel value referring to the end of the
    ///                     destination range.
    /// \param pred         Specifies the binary predicate function
    ///                     (or function object) which will be invoked for
    ///                     comparison of the elements in the two ranges.
    ///
    /// The assignments in the parallel \a starts_with algorithm invoked
    /// without an execution policy object execute in sequential order
    /// in the calling thread.
    ///
    /// \returns  The \a starts_with algorithm returns a
    ///           \a hpx::future<bool> if the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy and
    ///           returns \a bool otherwise.
    ///           The \a starts_with algorithm returns a boolean with the
    ///           value true if the second range matches the prefix of the
    ///           first range, false otherwise.
    template <typename ExPolicy, typename InIter1, typename InIter2,
        typename Pred = ranges::equal_to>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, bool>
    starts_with(ExPolicy&& policy, InIter1 first1, InIter1 last1,
        InIter2 first2, InIter2 last2, Pred&& pred = Pred());

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/equal.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // starts_with
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename FwdIter1, typename FwdIter2,
            typename Sent2>
        constexpr bool get_starts_with_result(
            util::in_in_result<FwdIter1, FwdIter2>&& p, Sent2 last2)
        {
            return p.in2 == last2;
        }

        HPX_CXX_CORE_EXPORT template <typename FwdIter1, typename FwdIter2,
            typename Sent2>
        hpx::future<bool> get_starts_with_result(
            hpx::future<util::in_in_result<FwdIter1, FwdIter2>>&& f,
            Sent2 last2)
        {
            return hpx::make_future<bool>(HPX_MOVE(f),
                [last2 = HPX_MOVE(last2)](
                    util::in_in_result<FwdIter1, FwdIter2>&& p) -> bool {
                    return p.in2 == last2;
                });
        }

        HPX_CXX_CORE_EXPORT struct starts_with
          : public algorithm<starts_with, bool>
        {
            constexpr starts_with() noexcept
              : algorithm("starts_with")
            {
            }

            template <typename ExPolicy, typename Iter1, typename Sent1,
                typename Iter2, typename Sent2, typename Pred, typename Proj1,
                typename Proj2>
            static constexpr bool sequential(ExPolicy, Iter1 first1,
                Sent1 last1, Iter2 first2, Sent2 last2, Pred&& pred,
                Proj1&& proj1, Proj2&& proj2)
            {
                auto dist1 = detail::distance(first1, last1);
                auto dist2 = detail::distance(first2, last2);

                if (dist1 < dist2)
                {
                    return false;
                }

                auto end_first = std::next(first1, dist2);

                return detail::equal_binary{}.call(hpx::execution::seq, first1,
                    end_first, first2, last2, HPX_FORWARD(Pred, pred),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
            }

            template <typename ExPolicy, typename FwdIter1, typename Sent1,
                typename FwdIter2, typename Sent2, typename Pred,
                typename Proj1, typename Proj2>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter1 first1,
                Sent1 last1, FwdIter2 first2, Sent2 last2, Pred&& pred,
                Proj1&& proj1, Proj2&& proj2)
            {
                auto dist1 = detail::distance(first1, last1);
                auto dist2 = detail::distance(first2, last2);

                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if (dist1 < dist2)
                {
                    if constexpr (has_scheduler_executor)
                    {
                        // make sure that the two ranges passed to equal_binary
                        // have different length to get `false` as return value
                        dist2 = 0;
                    }
                    else
                    {
                        return util::detail::algorithm_result<ExPolicy,
                            bool>::get(false);
                    }
                }

                auto end_first = std::next(first1, dist2);

                return detail::equal_binary{}.call(
                    HPX_FORWARD(ExPolicy, policy), first1, end_first, first2,
                    last2, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj1, proj1),
                    HPX_FORWARD(Proj2, proj2));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::starts_with
    HPX_CXX_CORE_EXPORT inline constexpr struct starts_with_t final
      : hpx::detail::tag_parallel_algorithm<starts_with_t>
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
        friend bool tag_fallback_invoke(hpx::starts_with_t, InIter1 first1,
            InIter1 last1, InIter2 first2, InIter2 last2, Pred pred = Pred())
        {
            static_assert(std::input_iterator<InIter1>,
                "Required at least input iterator.");

            static_assert(std::input_iterator<InIter2>,
                "Required at least input iterator.");

            return hpx::parallel::detail::starts_with().call(
                hpx::execution::seq, first1, last1, first2, last2,
                HPX_MOVE(pred), hpx::identity_v, hpx::identity_v);
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
        friend decltype(auto) tag_fallback_invoke(hpx::starts_with_t,
            ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1, FwdIter2 first2,
            FwdIter2 last2, Pred pred = Pred())
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::starts_with().call(
                HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2,
                HPX_MOVE(pred), hpx::identity_v, hpx::identity_v);
        }
    } starts_with{};
}    // namespace hpx

#endif
