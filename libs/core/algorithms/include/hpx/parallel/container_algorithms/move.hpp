//  Copyright (c) 2017 Bruno Pitrus
//  Copyright (c) 2017-2023 Hartmut Kaiser
//  Copyright (c) 2022 Dimitra Karatza
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/move.hpp
/// \page hpx::ranges::move
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
    // clang-format off

    /// Moves the elements in the range \a rng to another range beginning
    /// at \a dest. After this operation the elements in the moved-from
    /// range will still contain valid values of the appropriate type,
    /// but not necessarily the same values as before the move.
    ///
    /// \note   Complexity: Performs exactly
    ///         std::distance(begin(rng), end(rng)) assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Iter1       The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent1       The type of the source iterators used for the end of
    ///                     the first range (deduced).
    /// \tparam Iter2       The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a copy algorithm invoked with an
    /// execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a move algorithm returns a
    ///           \a hpx::future<ranges::move_result<iterator_t<Rng>, FwdIter2>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy and
    ///           returns \a ranges::move_result<iterator_t<Rng>, FwdIter2>
    ///           otherwise.
    ///           The \a move algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element moved.
    ///
    template <typename ExPolicy, typename Iter1, typename Sent1,
        typename Iter2>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        move_result<Iter1, Iter2>>::type
    move(ExPolicy&& policy, Iter1 first, Sent1 last, Iter2 dest);

    /// Moves the elements in the range \a rng to another range beginning
    /// at \a dest. After this operation the elements in the moved-from
    /// range will still contain valid values of the appropriate type,
    /// but not necessarily the same values as before the move.
    ///
    /// \note   Complexity: Performs exactly
    ///         std::distance(begin(rng), end(rng)) assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Iter2       The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a copy algorithm invoked with an
    /// execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a move algorithm returns a
    ///           \a hpx::future<ranges::move_result<iterator_t<Rng>, FwdIter2>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy and
    ///           returns \a ranges::move_result<iterator_t<Rng>, FwdIter2>
    ///           otherwise.
    ///           The \a move algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element moved.
    ///
    template <typename ExPolicy, typename Rng, typename Iter2>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        move_result<std::ranges::iterator_t<Rng>,
            Iter2>>::type
    move(ExPolicy&& policy, Rng&& rng, Iter2 dest);

    /// Moves the elements in the range \a rng to another range beginning
    /// at \a dest. After this operation the elements in the moved-from
    /// range will still contain valid values of the appropriate type,
    /// but not necessarily the same values as before the move.
    ///
    /// \note   Complexity: Performs exactly
    ///         std::distance(begin(rng), end(rng)) assignments.
    ///
    /// \tparam Iter1       The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent1       The type of the source iterators used for the end of
    ///                     the first range (deduced).
    /// \tparam Iter2       The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// \returns  The \a move algorithm returns \a
    ///           ranges::move_result<iterator_t<Rng>, FwdIter2>.
    ///           The \a move algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element moved.
    ///
    template <typename Iter1, typename Sent1, typename Iter2>
    move_result<Iter1, Iter2> move(Iter1 first, Sent1 last, Iter2 dest);

    /// Moves the elements in the range \a rng to another range beginning
    /// at \a dest. After this operation the elements in the moved-from
    /// range will still contain valid values of the appropriate type,
    /// but not necessarily the same values as before the move.
    ///
    /// \note   Complexity: Performs exactly
    ///         std::distance(begin(rng), end(rng)) assignments.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Iter2       The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// \returns  The \a move algorithm returns a
    ///           \a ranges::move_result<iterator_t<Rng>, FwdIter2>.
    ///           The \a move algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element moved.
    ///
    template <typename Rng, typename Iter2>
    move_result<std::ranges::iterator_t<Rng>, Iter2>
    move(Rng&& rng, Iter2 dest);
    // clang-format on
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/move.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using move_result = parallel::util::in_out_result<I, O>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::move
    HPX_CXX_CORE_EXPORT inline constexpr struct move_t final
      : hpx::detail::tag_dispatch<move_t,
            hpx::detail::tag_parallel_algorithm<move_t>>
    {
        template <typename ExPolicy, typename Iter1, typename Sent1,
            typename Iter2>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::sentinel_for<Sent1, Iter1> &&
                hpx::traits::is_iterator_v<Iter2>
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            move_result<Iter1, Iter2>>
        invoke_default(ExPolicy&& policy, Iter1 first, Sent1 last, Iter2 dest)
        {
            return hpx::parallel::detail::transfer<
                hpx::parallel::detail::move<Iter1, Iter2>>(
                HPX_FORWARD(ExPolicy, policy), first, last, dest);
        }

        template <typename ExPolicy, typename Rng, typename Iter2>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<Iter2>
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            move_result<std::ranges::iterator_t<Rng>, Iter2>>
        invoke_default(ExPolicy&& policy, Rng&& rng, Iter2 dest)
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            return hpx::parallel::detail::transfer<
                hpx::parallel::detail::move<iterator_type, Iter2>>(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::end(rng), dest);
        }

        template <typename Iter1, typename Sent1, typename Iter2>
        // clang-format off
            requires(
                std::sentinel_for<Sent1, Iter1> &&
                hpx::traits::is_iterator_v<Iter2>
            )
        // clang-format on
        static move_result<Iter1, Iter2> invoke_default(
            Iter1 first, Sent1 last, Iter2 dest)
        {
            return hpx::parallel::detail::transfer<
                hpx::parallel::detail::move<Iter1, Iter2>>(
                hpx::execution::seq, first, last, dest);
        }

        template <typename Rng, typename Iter2>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<Iter2>
            )
        // clang-format on
        static move_result<std::ranges::iterator_t<Rng>, Iter2> invoke_default(
            Rng&& rng, Iter2 dest)
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            return hpx::parallel::detail::transfer<
                hpx::parallel::detail::move<iterator_type, Iter2>>(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                dest);
        }
    } move{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
