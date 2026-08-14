//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/uninitialized_value_construct.hpp
/// \page hpx::ranges::uninitialized_value_construct, hpx::ranges::uninitialized_value_construct_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    /// Constructs objects of type typename iterator_traits<ForwardIt>::value_type
    /// in the uninitialized storage designated by the range
    /// by value-initialization. If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct
    /// algorithm invoked without an execution policy object will execute in
    /// sequential order in the calling thread.
    ///
    /// \returns  The \a uninitialized_value_construct algorithm returns a
    ///           returns \a FwdIter.
    ///           The \a uninitialized_value_construct algorithm returns the
    ///           output iterator to the element in the range, one past
    ///           the last element constructed.
    ///
    template <typename FwdIter, typename Sent>
    FwdIter uninitialized_value_construct(FwdIter first, Sent last);

    /// Constructs objects of type typename iterator_traits<ForwardIt>::value_type
    /// in the uninitialized storage designated by the range
    /// by value-initialization. If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
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
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct
    /// algorithm invoked with an execution policy object of type \a
    /// sequenced_policy execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct
    /// algorithm invoked with an execution policy object of type \a
    /// parallel_policy or \a parallel_task_policy are permitted to execute
    /// in an unordered fashion in unspecified threads, and indeterminately
    /// sequenced within each thread.
    ///
    /// \returns  The \a uninitialized_value_construct algorithm returns a
    ///           \a hpx::future<FwdIter> if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a uninitialized_value_construct algorithm returns
    ///           the iterator to the element in the source range, one past
    ///           the last element constructed.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    uninitialized_value_construct(ExPolicy&& policy, FwdIter first, Sent last);

    /// Constructs objects of type typename iterator_traits<ForwardIt>::value_type
    /// in the uninitialized storage designated by the range
    /// by value-initialization. If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    ///
    /// \param rng          Refers to the range to which will be value
    ///                     constructed.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct
    /// algorithm invoked without an execution policy object will execute in
    /// sequential order in the calling thread.
    ///
    /// \returns  The \a uninitialized_value_construct algorithm returns a
    ///           returns \a hpx::traits::range_traits<Rng>::iterator_type.
    ///           The \a uninitialized_value_construct algorithm returns
    ///           the output iterator to the element in the range, one past
    ///           the last element constructed.
    ///
    template <typename Rng>
    typename hpx::traits::range_traits<Rng>::iterator_type
    uninitialized_value_construct(Rng&& rng);

    /// Constructs objects of type typename iterator_traits<ForwardIt>::value_type
    /// in the uninitialized storage designated by the range
    /// by value-initialization. If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the range to which the value
    ///                     will be value consutrcted
    ///
    /// The assignments in the parallel \a uninitialized_value_construct
    /// algorithm invoked with an execution policy object of type \a
    /// sequenced_policy execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct
    /// algorithm invoked with an execution policy object of type \a
    /// parallel_policy or \a parallel_task_policy are permitted to execute
    /// in an unordered fashion in unspecified threads, and indeterminately
    /// sequenced within each thread.
    ///
    /// \returns  The \a uninitialized_value_construct algorithm returns a \a
    ///           hpx::future<typename hpx::traits::range_traits<Rng>::iterator_type>,
    ///           if the execution policy is of type \a sequenced_task_policy
    ///           or \a parallel_task_policy and returns \a typename
    ///           hpx::traits::range_traits<Rng>::iterator_type otherwise.
    ///           The \a uninitialized_value_construct algorithm returns
    ///           the output iterator to the element in the range, one past
    ///           the last element constructed.
    ///
    template <typename ExPolicy, typename Rng>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        typename hpx::traits::range_traits<Rng>::iterator_type>::type
    uninitialized_value_construct(ExPolicy&& policy, Rng&& rng);

    /// Constructs objects of type typename iterator_traits<ForwardIt>::value_type
    /// in the uninitialized storage designated by the range
    /// [first, first + count) by value-initialization. If an exception
    /// is thrown during the initialization, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct_n
    /// algorithm invoked without an execution policy object execute in
    /// sequential order in the calling thread.
    ///
    /// \returns  The \a uninitialized_value_construct_n algorithm returns a
    ///           returns \a FwdIter.
    ///           The \a uninitialized_value_construct_n algorithm returns
    ///           the iterator to the element in the source range, one past
    ///           the last element constructed.
    ///
    template <typename FwdIter, typename Size>
    FwdIter uninitialized_value_construct_n(FwdIter first, Size count);

    /// Constructs objects of type typename iterator_traits<ForwardIt>::value_type
    /// in the uninitialized storage designated by the range
    /// [first, first + count) by value-initialization. If an exception
    /// is thrown during the initialization, the function has no effects.
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
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct_n
    /// algorithm invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct_n
    /// algorithm invoked with an execution policy object of type
    /// \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a uninitialized_value_construct_n algorithm returns a
    ///           \a hpx::future<FwdIter> if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a uninitialized_value_construct_n algorithm returns
    ///           the iterator to the element in the source range, one past
    ///           the last element constructed.
    ///
    template <typename ExPolicy, typename FwdIter, typename Size>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    uninitialized_value_construct_n(ExPolicy&& policy, FwdIter first, Size count);

    // clang-format on
}}    // namespace hpx::ranges
#else

#include <hpx/config.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/uninitialized_value_construct.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_value_construct_t
        final
      : hpx::detail::tag_dispatch<uninitialized_value_construct_t,
            hpx::detail::tag_parallel_algorithm<
                uninitialized_value_construct_t>>
    {
        template <typename FwdIter, typename Sent>
        // clang-format off
            requires(
                std::forward_iterator<FwdIter> &&
                std::sentinel_for<Sent, FwdIter>
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, Sent last)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_value_construct<
                FwdIter>()
                .call(hpx::execution::seq, first, last);
        }

        template <typename ExPolicy, typename FwdIter, typename Sent>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                std::sentinel_for<Sent, FwdIter>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
        invoke_default(ExPolicy&& policy, FwdIter first, Sent last)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_value_construct<
                FwdIter>()
                .call(HPX_FORWARD(ExPolicy, policy), first, last);
        }

        template <typename Rng>
            requires(std::ranges::range<Rng>)
        static typename hpx::traits::range_traits<Rng>::iterator_type
        invoke_default(Rng&& rng)
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_value_construct<
                iterator_type>()
                .call(hpx::execution::seq, std::begin(rng), std::end(rng));
        }

        template <typename ExPolicy, typename Rng>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            typename hpx::traits::range_traits<Rng>::iterator_type>
        invoke_default(ExPolicy&& policy, Rng&& rng)
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_value_construct<
                iterator_type>()
                .call(HPX_FORWARD(ExPolicy, policy), std::begin(rng),
                    std::end(rng));
        }
    } uninitialized_value_construct{};

    HPX_CXX_CORE_EXPORT inline constexpr struct
        uninitialized_value_construct_n_t final
      : hpx::detail::tag_dispatch<uninitialized_value_construct_n_t,
            hpx::detail::tag_parallel_algorithm<
                uninitialized_value_construct_n_t>>
    {
        template <typename FwdIter, typename Size>
        // clang-format off
            requires(
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, Size count)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_value_construct_n<
                FwdIter>()
                .call(hpx::execution::seq, first, count);
        }

        template <typename ExPolicy, typename FwdIter, typename Size>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
        invoke_default(ExPolicy&& policy, FwdIter first, Size count)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_value_construct_n<
                FwdIter>()
                .call(HPX_FORWARD(ExPolicy, policy), first, count);
        }
    } uninitialized_value_construct_n{};
}    // namespace hpx::ranges

#endif
