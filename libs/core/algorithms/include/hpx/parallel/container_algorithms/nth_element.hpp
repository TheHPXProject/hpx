//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/nth_element.hpp
/// \page hpx::ranges::nth_element
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    /// nth_element is a partial sorting algorithm that rearranges elements in
    /// [first, last) such that the element pointed at by nth is changed to
    /// whatever element would occur in that position if [first, last) were
    /// sorted and all of the elements before this new nth element are less
    /// than or equal to the elements after the new nth element.
    ///
    /// \note   Complexity: Linear in std::distance(first, last) on average.
    ///         O(N) applications of the predicate, and O(N log N) swaps,
    ///         where N = last - first.
    ///
    /// \tparam RandomIt    The type of the source begin, nth, and end
    ///                     iterators used (deduced). This iterator type must
    ///                     meet the requirements of a random access iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for RandomIt.
    /// \tparam Pred        Comparison function object which returns true if
    ///                     the first argument is less than the second.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param nth          Refers to the iterator defining the sort partition
    ///                     point
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param pred         Specifies the comparison function object which
    ///                     returns true if the first argument is less than
    ///                     (i.e. is ordered before) the second.
    ///                     The signature of this
    ///                     comparison function should be equivalent to:
    ///                     \code
    ///                     bool cmp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type must be such that an object of
    ///                     type \a randomIt can be dereferenced and then
    ///                     implicitly converted to Type. This defaults
    ///                     to std::less<>.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///                     This defaults to hpx::identity.
    ///
    /// The comparison operations in the parallel \a nth_element
    /// algorithm invoked without an execution policy object execute in
    /// sequential order in the calling thread.
    ///
    /// \returns  The \a nth_element algorithm returns returns \a
    ///           RandomIt.
    ///           The \a nth_element algorithm returns an iterator equal
    ///           to last.
    ///
    template <typename RandomIt, typename Sent,
        typename Pred = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    RandomIt nth_element(RandomIt first, RandomIt nth, Sent last,
        Pred&& pred = Pred(), Proj&& proj = Proj());

    /// nth_element is a partial sorting algorithm that rearranges elements in
    /// [first, last) such that the element pointed at by nth is changed to
    /// whatever element would occur in that position if [first, last) were
    /// sorted and all of the elements before this new nth element are less
    /// than or equal to the elements after the new nth element.
    ///
    /// \note   Complexity: Linear in std::distance(first, last) on average.
    ///         O(N) applications of the predicate, and O(N log N) swaps,
    ///         where N = last - first.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam RandomIt    The type of the source begin, nth, and end
    ///                     iterators used (deduced). This iterator type must
    ///                     meet the requirements of a random access iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for RandomIt.
    /// \tparam Pred        Comparison function object which returns true if
    ///                     the first argument is less than the second.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param nth          Refers to the iterator defining the sort partition
    ///                     point
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param pred         Specifies the comparison function object which
    ///                     returns true if the first argument is less than
    ///                     (i.e. is ordered before) the second.
    ///                     The signature of this
    ///                     comparison function should be equivalent to:
    ///                     \code
    ///                     bool cmp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type must be such that an object of
    ///                     type \a randomIt can be dereferenced and then
    ///                     implicitly converted to Type. This defaults
    ///                     to std::less<>.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///                     This defaults to hpx::identity.
    ///
    /// The comparison operations in the parallel \a nth_element invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a nth_element algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a partition algorithm returns a \a
    ///           hpx::future<RandomIt>
    ///           if the execution policy is of type \a parallel_task_policy
    ///           and returns \a RandomIt otherwise.
    ///           The \a nth_element algorithm returns an iterator equal
    ///           to last.
    ///
    template <typename ExPolicy, typename RandomIt, typename Sent,
        typename Pred = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    parallel::util::detail::algorithm_result_t<ExPolicy, RandomIt>
    nth_element(ExPolicy&& policy, RandomIt first, RandomIt nth,
        Sent last, Pred&& pred = Pred(), Proj&& proj = Proj());

    /// nth_element is a partial sorting algorithm that rearranges elements in
    /// [first, last) such that the element pointed at by nth is changed to
    /// whatever element would occur in that position if [first, last) were
    /// sorted and all of the elements before this new nth element are less
    /// than or equal to the elements after the new nth element.
    ///
    /// \note   Complexity: Linear in std::distance(first, last) on average.
    ///         O(N) applications of the predicate, and O(N log N) swaps,
    ///         where N = last - first.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an random access iterator.
    /// \tparam Pred        Comparison function object which returns true if
    ///                     the first argument is less than the second.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param nth          Refers to the iterator defining the sort partition
    ///                     point
    /// \param pred         Specifies the comparison function object which
    ///                     returns true if the first argument is less than
    ///                     (i.e. is ordered before) the second.
    ///                     The signature of this
    ///                     comparison function should be equivalent to:
    ///                     \code
    ///                     bool cmp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type must be such that an object of
    ///                     type \a randomIt can be dereferenced and then
    ///                     implicitly converted to Type. This defaults
    ///                     to std::less<>.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///                     This defaults to hpx::identity.
    ///
    /// The comparison operations in the parallel \a nth_element
    /// algorithm invoked without an execution policy object execute in
    /// sequential order in the calling thread.
    ///
    /// \returns  The \a nth_element algorithm returns returns \a
    ///           std::ranges::iterator_t<Rng>.
    ///           The \a nth_element algorithm returns an iterator equal
    ///           to last.
    ///
    template <typename Rng,
        typename Pred = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    std::ranges::iterator_t<Rng> nth_element(Rng&& rng,
        std::ranges::iterator_t<Rng> nth, Pred&& pred = Pred(),
        Proj&& proj = Proj());

    /// nth_element is a partial sorting algorithm that rearranges elements in
    /// [first, last) such that the element pointed at by nth is changed to
    /// whatever element would occur in that position if [first, last) were
    /// sorted and all of the elements before this new nth element are less
    /// than or equal to the elements after the new nth element.
    ///
    /// \note   Complexity: Linear in std::distance(first, last) on average.
    ///         O(N) applications of the predicate, and O(N log N) swaps,
    ///         where N = last - first.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an random access iterator.
    /// \tparam Pred        Comparison function object which returns true if
    ///                     the first argument is less than the second.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param nth          Refers to the iterator defining the sort partition
    ///                     point
    /// \param pred         Specifies the comparison function object which
    ///                     returns true if the first argument is less than
    ///                     (i.e. is ordered before) the second.
    ///                     The signature of this
    ///                     comparison function should be equivalent to:
    ///                     \code
    ///                     bool cmp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type must be such that an object of
    ///                     type \a randomIt can be dereferenced and then
    ///                     implicitly converted to Type. This defaults
    ///                     to std::less<>.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///                     This defaults to hpx::identity.
    ///
    /// The comparison operations in the parallel \a nth_element invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a nth_element algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a partition algorithm returns a \a
    ///           hpx::future<std::ranges::iterator_t<Rng>>
    ///           if the execution policy is of type \a parallel_task_policy
    ///           and returns \a std::ranges::iterator_t<Rng> otherwise.
    ///           The \a nth_element algorithm returns an iterator equal
    ///           to last.
    ///
    template <typename ExPolicy, typename Rng,
        typename Pred = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::iterator_t<Rng>>
    nth_element(ExPolicy&& policy, Rng&& rng,
        std::ranges::iterator_t<Rng> nth,
        Pred&& pred = Pred(), Proj&& proj = Proj());

    // clang-format on
}}    // namespace hpx::ranges
#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/nth_element.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT inline constexpr struct nth_element_t final
      : hpx::detail::tag_dispatch<nth_element_t,
            hpx::detail::tag_parallel_algorithm<nth_element_t>>
    {
        template <typename RandomIt, typename Sent,
            typename Pred = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::random_access_iterator<RandomIt> &&
                std::sentinel_for<Sent, RandomIt> &&
                hpx::parallel::traits::is_projected_v<Proj, RandomIt> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj, RandomIt>,
                    hpx::parallel::traits::projected<Proj, RandomIt>
                >
            )
        // clang-format on
        static RandomIt invoke_default(RandomIt first, RandomIt nth, Sent last,
            Pred pred = Pred(), Proj proj = Proj())
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "Requires at least random access iterator.");

            return hpx::parallel::detail::nth_element<RandomIt>().call(
                hpx::execution::seq, first, nth, last, HPX_MOVE(pred),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename RandomIt, typename Sent,
            typename Pred = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::random_access_iterator<RandomIt> &&
                std::sentinel_for<Sent, RandomIt> &&
                hpx::parallel::traits::is_projected_v<Proj, RandomIt> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, Pred,
                    hpx::parallel::traits::projected<Proj, RandomIt>,
                    hpx::parallel::traits::projected<Proj, RandomIt>
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy, RandomIt>
        invoke_default(ExPolicy&& policy, RandomIt first, RandomIt nth,
            Sent last, Pred pred = Pred(), Proj proj = Proj())
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "Requires at least random access iterator.");

            return hpx::parallel::detail::nth_element<RandomIt>().call(
                HPX_FORWARD(ExPolicy, policy), first, nth, last, HPX_MOVE(pred),
                HPX_MOVE(proj));
        }

        template <typename Rng, typename Pred = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static std::ranges::iterator_t<Rng> invoke_default(Rng&& rng,
            std::ranges::iterator_t<Rng> nth, Pred pred = Pred(),
            Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::random_access_iterator<iterator_type>,
                "Requires at least random access iterator.");

            return hpx::parallel::detail::nth_element<iterator_type>().call(
                hpx::execution::seq, std::begin(rng), nth, std::end(rng),
                HPX_MOVE(pred), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng,
            typename Pred = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, Pred,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            std::ranges::iterator_t<Rng>>
        invoke_default(ExPolicy&& policy, Rng&& rng,
            std::ranges::iterator_t<Rng> nth, Pred pred = Pred(),
            Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::random_access_iterator<iterator_type>,
                "Requires at least random access iterator.");

            return hpx::parallel::detail::nth_element<iterator_type>().call(
                HPX_FORWARD(ExPolicy, policy), std::begin(rng), nth,
                std::end(rng), HPX_MOVE(pred), HPX_MOVE(proj));
        }
    } nth_element{};
}    // namespace hpx::ranges

#endif
