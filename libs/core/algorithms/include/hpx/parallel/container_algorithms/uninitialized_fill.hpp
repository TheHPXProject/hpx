//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/uninitialized_fill.hpp
/// \page hpx::ranges::uninitialized_fill, hpx::ranges::uninitialized_fill_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    /// Copies the given \a value to an uninitialized memory area, defined by
    /// the range [first, last). If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param value        The value to be assigned.
    ///
    /// The assignments in the ranges \a uninitialized_fill algorithm invoked
    /// without an execution policy object will execute in sequential order in
    /// the calling thread.
    ///
    /// \returns  The \a uninitialized_fill algorithm returns a
    ///           returns \a FwdIter.
    ///           The \a uninitialized_fill algorithm returns the output
    ///           iterator to the element in the range, one past
    ///           the last element copied.
    ///
    template <typename FwdIter, typename Sent, typename T>
    FwdIter uninitialized_fill(FwdIter first, Sent last, T const& value);

    /// Copies the given \a value to an uninitialized memory area, defined by
    /// the range [first, last). If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param value        The value to be assigned.
    ///
    /// The assignments in the parallel \a uninitialized_fill algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_fill algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a uninitialized_fill algorithm returns a
    ///           returns \a FwdIter.
    ///           The \a uninitialized_fill algorithm returns the output
    ///           iterator to the element in the range, one past
    ///           the last element copied.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent, typename T>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    uninitialized_fill(ExPolicy&& policy, FwdIter first, Sent last,
        T const& value);

    /// Copies the given \a value to an uninitialized memory area, defined by
    /// the range [first, last). If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param rng          Refers to the range to which the value
    ///                     will be filled
    /// \param value        The value to be assigned.
    ///
    /// The assignments in the parallel \a uninitialized_fill algorithm invoked
    /// without an execution policy object will execute in sequential order in
    /// the calling thread.
    ///
    /// \returns  The \a uninitialized_fill algorithm returns a
    ///           returns \a hpx::traits::range_traits<Rng>::iterator_type.
    ///           The \a uninitialized_fill algorithm returns the output
    ///           iterator to the element in the range, one past
    ///           the last element copied.
    ///
    template <typename Rng, typename T>
    typename hpx::traits::range_traits<Rng>::iterator_type uninitialized_fill(
        Rng&& rng, T const& value);

    /// Copies the given \a value to an uninitialized memory area, defined by
    /// the range [first, last). If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the range to which the value
    ///                     will be filled
    /// \param value        The value to be assigned.
    ///
    /// The assignments in the parallel \a uninitialized_fill algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_fill algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a uninitialized_fill algorithm returns a \a
    ///           hpx::future<typename hpx::traits::range_traits<Rng>::iterator_type>,
    ///           if the execution policy is of type \a sequenced_task_policy
    ///           or \a parallel_task_policy and returns \a typename
    ///           hpx::traits::range_traits<Rng>::iterator_type otherwise.
    ///           The \a uninitialized_fill algorithm returns the
    ///           iterator to one past the last element filled in the range.
    ///
    template <typename ExPolicy, typename Rng, typename T>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        typename hpx::traits::range_traits<Rng1>::iterator_type>::type
    uninitialized_fill(ExPolicy&& policy, Rng&& rng, T const& value);

    /// Copies the given \a value value to the first count elements in an
    /// uninitialized memory area beginning at first. If an exception is thrown
    /// during the initialization, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// The assignments in the parallel \a uninitialized_fill_n algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a uninitialized_fill_n algorithm returns a
    ///           returns \a FwdIter.
    ///           The \a uninitialized_fill_n algorithm returns the output
    ///           iterator to the element in the range, one past
    ///           the last element copied.
    ///
    template <typename FwdIter, typename Size, typename T>
    FwdIter uninitialized_fill_n(FwdIter first, Size count, T const& value);

    /// Copies the given \a value value to the first count elements in an
    /// uninitialized memory area beginning at first. If an exception is thrown
    /// during the initialization, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// The assignments in the parallel \a uninitialized_fill_n algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_fill_n algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a uninitialized_fill_n algorithm returns a
    ///           \a hpx::future<FwdIter>, if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns FwdIter
    ///           otherwise.
    ///           The \a uninitialized_fill_n algorithm returns the output
    ///           iterator to the element in the range, one past
    ///           the last element copied.
    ///
    template <typename ExPolicy, typename FwdIter, typename Size, typename T>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    uninitialized_fill_n(ExPolicy&& policy, FwdIter first, Size count,
        T const& value);

    // clang-format on
}}    // namespace hpx::ranges
#else

#include <hpx/config.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/uninitialized_fill.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_fill_t final
      : hpx::detail::tag_dispatch<uninitialized_fill_t,
            hpx::detail::tag_parallel_algorithm<uninitialized_fill_t>>
    {
        template <typename FwdIter, typename Sent, typename T>
        // clang-format off
            requires(
                std::forward_iterator<FwdIter> &&
                std::sentinel_for<Sent, FwdIter>
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, Sent last, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_fill<FwdIter>().call(
                hpx::execution::seq, first, last, value);
        }

        template <typename ExPolicy, typename FwdIter, typename Sent,
            typename T>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                std::sentinel_for<Sent, FwdIter>
            )
        // clang-format on
        static typename parallel::util::detail::algorithm_result<ExPolicy,
            FwdIter>::type
        invoke_default(
            ExPolicy&& policy, FwdIter first, Sent last, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_fill<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, value);
        }

        template <typename Rng, typename T>
            requires(std::ranges::range<Rng>)
        static typename hpx::traits::range_traits<Rng>::iterator_type
        invoke_default(Rng&& rng, T const& value)
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_fill<iterator_type>()
                .call(
                    hpx::execution::seq, std::begin(rng), std::end(rng), value);
        }

        template <typename ExPolicy, typename Rng, typename T>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            typename hpx::traits::range_traits<Rng>::iterator_type>
        invoke_default(ExPolicy&& policy, Rng&& rng, T const& value)
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_fill<iterator_type>()
                .call(HPX_FORWARD(ExPolicy, policy), std::begin(rng),
                    std::end(rng), value);
        }
    } uninitialized_fill{};

    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_fill_n_t final
      : hpx::detail::tag_dispatch<uninitialized_fill_n_t,
            hpx::detail::tag_parallel_algorithm<uninitialized_fill_n_t>>
    {
        template <typename FwdIter, typename Size, typename T>
        // clang-format off
            requires(
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, Size count, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_fill_n<FwdIter>().call(
                hpx::execution::seq, first, count, value);
        }

        template <typename ExPolicy, typename FwdIter, typename Size,
            typename T>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
        invoke_default(
            ExPolicy&& policy, FwdIter first, Size count, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_fill_n<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, count, value);
        }
    } uninitialized_fill_n{};
}    // namespace hpx::ranges

#endif
