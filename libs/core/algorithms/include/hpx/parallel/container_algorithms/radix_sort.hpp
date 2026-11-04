//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/radix_sort.hpp
/// \page hpx::ranges::radix_sort
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx::ranges {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range [first, last) in ascending order of
    /// their integral keys, using an in-place MSD radix sort. The key of an
    /// element is the result of applying \a proj to it; it must be of an
    /// integral type other than bool. The order of elements with equal keys
    /// is not guaranteed to be preserved. See \a hpx::radix_sort.
    ///
    /// \note   Complexity: O(N * W / 8) element moves and key extractions,
    ///         where N = std::distance(first, last) and W is the number of
    ///         bits in the key type, plus O(N log N) comparisons for the
    ///         small partitions sorted by comparison.
    ///
    /// \tparam RandomIt    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for RandomIt.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param proj         Specifies the function (or function object) which
    ///                     yields the integral key of an element.
    ///
    /// The assignments in the parallel \a radix_sort algorithm invoked
    /// without an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a radix_sort algorithm returns \a RandomIt.
    ///           The algorithm returns an iterator pointing to the first
    ///           element after the last element in the input sequence.
    ///
    template <typename RandomIt, typename Sent, typename Proj = hpx::identity>
    RandomIt radix_sort(RandomIt first, Sent last, Proj proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range [first, last) in ascending order of
    /// their integral keys, using an in-place MSD radix sort. The key of an
    /// element is the result of applying \a proj to it; it must be of an
    /// integral type other than bool. The order of elements with equal keys
    /// is not guaranteed to be preserved. See \a hpx::radix_sort.
    ///
    /// \note   Complexity: O(N * W / 8) element moves and key extractions,
    ///         where N = std::distance(first, last) and W is the number of
    ///         bits in the key type, plus O(N log N) comparisons for the
    ///         small partitions sorted by comparison.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam RandomIt    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for RandomIt.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param proj         Specifies the function (or function object) which
    ///                     yields the integral key of an element.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a radix_sort algorithm returns a
    ///           \a hpx::future<RandomIt> if the execution policy is of
    ///           type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a RandomIt
    ///           otherwise.
    ///           The algorithm returns an iterator pointing to the first
    ///           element after the last element in the input sequence.
    ///
    template <typename ExPolicy, typename RandomIt, typename Sent,
        typename Proj = hpx::identity>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, RandomIt>
    radix_sort(ExPolicy&& policy, RandomIt first, Sent last,
        Proj proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range \a rng in ascending order of their
    /// integral keys, using an in-place MSD radix sort. The key of an
    /// element is the result of applying \a proj to it; it must be of an
    /// integral type other than bool. The order of elements with equal keys
    /// is not guaranteed to be preserved. See \a hpx::radix_sort.
    ///
    /// \note   Complexity: O(N * W / 8) element moves and key extractions,
    ///         where N = std::size(rng) and W is the number of bits in the
    ///         key type, plus O(N log N) comparisons for the small
    ///         partitions sorted by comparison.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a random access iterator.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param proj         Specifies the function (or function object) which
    ///                     yields the integral key of an element.
    ///
    /// The assignments in the parallel \a radix_sort algorithm invoked
    /// without an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a radix_sort algorithm returns
    ///           \a std::ranges::iterator_t<Rng>.
    ///           The algorithm returns an iterator pointing to the first
    ///           element after the last element in the input sequence.
    ///
    template <typename Rng, typename Proj = hpx::identity>
    std::ranges::iterator_t<Rng> radix_sort(Rng&& rng, Proj proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range \a rng in ascending order of their
    /// integral keys, using an in-place MSD radix sort. The key of an
    /// element is the result of applying \a proj to it; it must be of an
    /// integral type other than bool. The order of elements with equal keys
    /// is not guaranteed to be preserved. See \a hpx::radix_sort.
    ///
    /// \note   Complexity: O(N * W / 8) element moves and key extractions,
    ///         where N = std::size(rng) and W is the number of bits in the
    ///         key type, plus O(N log N) comparisons for the small
    ///         partitions sorted by comparison.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a random access iterator.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param proj         Specifies the function (or function object) which
    ///                     yields the integral key of an element.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a radix_sort algorithm returns a
    ///           \a hpx::future<std::ranges::iterator_t<Rng>> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns
    ///           \a std::ranges::iterator_t<Rng> otherwise.
    ///           The algorithm returns an iterator pointing to the first
    ///           element after the last element in the input sequence.
    ///
    template <typename ExPolicy, typename Rng, typename Proj = hpx::identity>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::iterator_t<Rng>>
    radix_sort(ExPolicy&& policy, Rng&& rng, Proj proj = Proj());

    // clang-format on
}    // namespace hpx::ranges

#else

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/radix_sort.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::radix_sort
    HPX_CXX_CORE_EXPORT inline constexpr struct radix_sort_t final
      : hpx::detail::tag_dispatch<radix_sort_t,
            hpx::detail::tag_parallel_algorithm<radix_sort_t>>
    {
        template <typename RandomIt, typename Sent,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<RandomIt> &&
                std::sentinel_for<Sent, RandomIt> &&
                hpx::parallel::detail::is_radix_sort_key_v<RandomIt, Proj>
            )
        // clang-format on
        static RandomIt invoke_default(
            RandomIt first, Sent last, Proj proj = Proj())
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "Requires a random access iterator.");

            return hpx::parallel::detail::radix_sort_algo<RandomIt>().call(
                hpx::execution::seq, first, last, HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename RandomIt, typename Sent,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<RandomIt> &&
                std::sentinel_for<Sent, RandomIt> &&
                hpx::parallel::detail::is_radix_sort_key_v<RandomIt, Proj>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy, RandomIt>
        invoke_default(
            ExPolicy&& policy, RandomIt first, Sent last, Proj proj = Proj())
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "Requires a random access iterator.");

            return hpx::parallel::detail::radix_sort_algo<RandomIt>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(proj));
        }

        template <typename Rng, typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::detail::is_radix_sort_key_v<
                    std::ranges::iterator_t<Rng>, Proj>
            )
        // clang-format on
        static std::ranges::iterator_t<Rng> invoke_default(
            Rng&& rng, Proj proj = Proj())
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::random_access_iterator<iterator_type>,
                "Requires a random access iterator.");

            return hpx::parallel::detail::radix_sort_algo<iterator_type>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::detail::is_radix_sort_key_v<
                    std::ranges::iterator_t<Rng>, Proj>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            std::ranges::iterator_t<Rng>>
        invoke_default(ExPolicy&& policy, Rng&& rng, Proj proj = Proj())
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::random_access_iterator<iterator_type>,
                "Requires a random access iterator.");

            return hpx::parallel::detail::radix_sort_algo<iterator_type>().call(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::end(rng), HPX_MOVE(proj));
        }
    } radix_sort{};
}    // namespace hpx::ranges

#endif
