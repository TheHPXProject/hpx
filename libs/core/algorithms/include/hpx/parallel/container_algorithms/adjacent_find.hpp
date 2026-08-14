//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/adjacent_find.hpp
/// \page hpx::ranges::adjacent_find
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    /// Searches the range [first, last) for two consecutive identical elements.
    ///
    /// \note   Complexity: Exactly the smaller of (result - first) + 1 and
    ///                     (last - first) - 1 application of the predicate
    ///                     where \a result is the value returned
    ///
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    /// \tparam Pred        The type of an optional function/function object to use.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param pred         The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The
    ///                     signature should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 must be such
    ///                     that objects of type \a FwdIter
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 .
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a adjacent_find algorithm returns an iterator to the
    ///           first of the identical elements. If no such elements are
    ///           found, \a last is returned.
    template <typename FwdIter, typename Sent,
        typename Proj = hpx::identity,
        typename Pred = detail::equal_to>
    FwdIter adjacent_find(
        FwdIter first, Sent last, Pred&& pred = Pred(), Proj&& proj = Proj());

    /// Searches the range [first, last) for two consecutive identical elements.
    /// This version uses the given binary predicate pred
    ///
    /// \note   Complexity: Exactly the smaller of (result - first) + 1 and
    ///                     (last - first) - 1 application of the predicate
    ///                     where \a result is the value returned
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_find requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param pred         The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The
    ///                     signature should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 must be such
    ///                     that objects of type \a FwdIter
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 .
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparison operations in the parallel \a adjacent_find invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a adjacent_find invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a adjacent_find algorithm returns a \a hpx::future<InIter>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a InIter otherwise.
    ///           The \a adjacent_find algorithm returns an iterator to the
    ///           first of the identical elements. If no such elements are
    ///           found, \a last is returned.
    ///
    ///           This overload of \a adjacent_find is available if the user
    ///           decides to provide their algorithm their own binary
    ///           predicate \a pred.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent,
        typename Proj = hpx::identity,
        typename Pred = detail::equal_to>
    typename parallel::util::detail::algorithm_result<ExPolicy,
            FwdIter>::type
    adjacent_find(ExPolicy&& policy, FwdIter first, Sent last,
        Pred&& pred = Pred(), Proj&& proj = Proj());

    /// Searches the range rng for two consecutive identical elements.
    ///
    /// \note   Complexity: Exactly the smaller of (result - std::begin(rng)) + 1
    ///                     and (std::begin(rng) - std::end(rng)) - 1 applications
    ///                     of the predicate where \a result is the value returned
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    /// \tparam Pred        The type of an optional function/function object to use.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param pred         The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The
    ///                     signature should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 must be such
    ///                     that objects of type \a FwdIter
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 .
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a adjacent_find algorithm returns an iterator to the
    ///           first of the identical elements. If no such elements are
    ///           found, \a last is returned.
    template <typename Rng,
        typename Proj = hpx::identity,
        typename Pred = detail::equal_to>
    typename hpx::traits::range_traits<Rng>::iterator_type adjacent_find(
        Rng&& rng, Pred&& pred = Pred(), Proj&& proj = Proj());

    /// Searches the range rng for two consecutive identical elements.
    ///
    /// \note   Complexity: Exactly the smaller of (result - std::begin(rng)) + 1
    ///                     and (std::begin(rng) - std::end(rng)) - 1 applications
    ///                     of the predicate where \a result is the value returned
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_find requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param pred         The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The
    ///                     signature should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 must be such
    ///                     that objects of type \a FwdIter
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 .
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparison operations in the parallel \a adjacent_find invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a adjacent_find invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a adjacent_find algorithm returns a \a hpx::future<InIter>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a InIter otherwise.
    ///           The \a adjacent_find algorithm returns an iterator to the
    ///           first of the identical elements. If no such elements are
    ///           found, \a last is returned.
    ///
    ///           This overload of \a adjacent_find is available if the user
    ///           decides to provide their algorithm their own binary
    ///           predicate \a pred.
    ///
    template <typename ExPolicy, typename Rng,
        typename Proj = hpx::identity,
        typename Pred = detail::equal_to>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        typename hpx::traits::range_traits<Rng>::iterator_type>::type
    adjacent_find(ExPolicy&& policy, Rng&& rng, Pred&& pred = Pred(),
        Proj&& proj = Proj());
    // clang-format on
}}    // namespace hpx::ranges
#else

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/adjacent_find.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT inline constexpr struct adjacent_find_t final
      : hpx::detail::tag_dispatch<adjacent_find_t,
            hpx::detail::tag_parallel_algorithm<adjacent_find_t>>
    {
        template <typename FwdIter, typename Sent,
            typename Proj = hpx::identity,
            typename Pred = hpx::parallel::detail::equal_to>
        // clang-format off
            requires (
                std::forward_iterator<FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >::value
            )
        // clang-format on
        static FwdIter invoke_default(
            FwdIter first, Sent last, Pred pred = Pred(), Proj proj = Proj())
        {
            return hpx::parallel::detail::adjacent_find<FwdIter, FwdIter>()
                .call(hpx::execution::seq, first, last, HPX_MOVE(pred),
                    HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename FwdIter, typename Sent,
            typename Proj = hpx::identity,
            typename Pred = hpx::parallel::detail::equal_to>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable<
                    ExPolicy, Pred,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >::value
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
        invoke_default(ExPolicy&& policy, FwdIter first, Sent last,
            Pred pred = Pred(), Proj proj = Proj())
        {
            return hpx::parallel::detail::adjacent_find<FwdIter, FwdIter>()
                .call(HPX_FORWARD(ExPolicy, policy), first, last,
                    HPX_MOVE(pred), HPX_MOVE(proj));
        }

        template <typename Rng, typename Proj = hpx::identity,
            typename Pred = hpx::parallel::detail::equal_to>
        // clang-format off
            requires (
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >::value
            )
        // clang-format on
        static typename hpx::traits::range_traits<Rng>::iterator_type
        invoke_default(Rng&& rng, Pred pred = Pred(), Proj proj = Proj())
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::adjacent_find<iterator_type,
                iterator_type>()
                .call(hpx::execution::seq, std::begin(rng), std::end(rng),
                    HPX_MOVE(pred), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng,
            typename Proj = hpx::identity,
            typename Pred = hpx::parallel::detail::equal_to>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable<
                    ExPolicy, Pred,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >::value
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            typename hpx::traits::range_traits<Rng>::iterator_type>
        invoke_default(ExPolicy&& policy, Rng&& rng, Pred&& pred = Pred(),
            Proj&& proj = Proj())
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::adjacent_find<iterator_type,
                iterator_type>()
                .call(HPX_FORWARD(ExPolicy, policy), std::begin(rng),
                    std::end(rng), HPX_FORWARD(Pred, pred),
                    HPX_FORWARD(Proj, proj));
        }
    } adjacent_find{};
}    // namespace hpx::ranges

#endif
