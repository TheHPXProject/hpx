//  Copyright (c) 2015-2023 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/shift_left.hpp
/// \page hpx::ranges::shift_left
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Shifts the elements in the range [first, last) by n positions towards
    /// the beginning of the range. For every integer i in [0, last - first
    ///  - n), moves the element originally at position first + n + i to
    /// position first + i.
    ///
    /// \note   Complexity: At most (last - first) - n assignments.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     positions to shift by.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param n            Refers to the number of positions to shift.
    ///
    /// The assignment operations in the parallel \a shift_left algorithm
    /// invoked without an execution policy object will execute in sequential
    /// order in the calling thread.
    ///
    /// \note The type of dereferenced \a FwdIter must meet the requirements
    ///       of \a MoveAssignable.
    ///
    /// \returns  The \a shift_left algorithm returns \a FwdIter.
    ///           The \a shift_left algorithm returns an iterator to the
    ///           end of the resulting range.
    ///
    template <typename FwdIter, typename Sent, typename Size>
    FwdIter shift_left(FwdIter first, Sent last, Size n);

    ///////////////////////////////////////////////////////////////////////////
    /// Shifts the elements in the range [first, last) by n positions towards
    /// the beginning of the range. For every integer i in [0, last - first
    ///  - n), moves the element originally at position first + n + i to
    /// position first + i.
    ///
    /// \note   Complexity: At most (last - first) - n assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     positions to shift by.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param n            Refers to the number of positions to shift.
    ///
    /// The assignment operations in the parallel \a shift_left algorithm
    /// invoked with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignment operations in the parallel \a shift_left algorithm
    /// invoked with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note The type of dereferenced \a FwdIter must meet the requirements
    ///       of \a MoveAssignable.
    ///
    /// \returns  The \a shift_left algorithm returns a
    ///           \a hpx::future<FwdIter> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a shift_left algorithm returns an iterator to the
    ///           end of the resulting range.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent,
        typename Size>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    shift_left(ExPolicy&& policy, FwdIter first, Sent last, Size n);

    ///////////////////////////////////////////////////////////////////////////
    /// Shifts the elements in the range [first, last) by n positions towards
    /// the beginning of the range. For every integer i in [0, last - first
    ///  - n), moves the element originally at position first + n + i to
    /// position first + i.
    ///
    /// \note   Complexity: At most (last - first) - n assignments.
    ///
    /// \tparam Rng         The type of the range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     positions to shift by.
    ///
    /// \param rng          Refers to the range in which the elements
    ///                     will be shifted.
    /// \param n            Refers to the number of positions to shift.
    ///
    /// The assignment operations in the parallel \a shift_left algorithm
    /// invoked without an execution policy object will execute in sequential
    /// order in the calling thread.
    ///
    /// \note The type of dereferenced \a std::ranges::iterator_t<Rng>
    ///       must meet the requirements of \a MoveAssignable.
    ///
    /// \returns  The \a shift_left algorithm returns \a
    ///           std::ranges::iterator_t<Rng>.
    ///           The \a shift_left algorithm returns an iterator to the
    ///           end of the resulting range.
    ///
    template <typename Rng, typename Size>
    std::ranges::iterator_t<Rng> shift_left(Rng&& rng, Size n);

    ///////////////////////////////////////////////////////////////////////////
    /// Shifts the elements in the range [first, last) by n positions towards
    /// the beginning of the range. For every integer i in [0, last - first
    ///  - n), moves the element originally at position first + n + i to
    /// position first + i.
    ///
    /// \note   Complexity: At most (last - first) - n assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     positions to shift by.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the range in which the elements
    ///                     will be shifted.
    /// \param n            Refers to the number of positions to shift.
    ///
    /// The assignment operations in the parallel \a shift_left algorithm
    /// invoked with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignment operations in the parallel \a shift_left algorithm
    /// invoked with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note The type of dereferenced \a std::ranges::iterator_t<Rng>
    ///       must meet the requirements of \a MoveAssignable.
    ///
    /// \returns  The \a shift_left algorithm returns a
    ///           \a hpx::future<std::ranges::iterator_t<Rng>> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a std::ranges::iterator_t<Rng> otherwise.
    ///           The \a shift_left algorithm returns an iterator to the
    ///           end of the resulting range.
    ///
    template <typename ExPolicy, typename Rng, typename Size>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        std::ranges::iterator_t<Rng>>
    shift_left(ExPolicy&& policy, Rng&& rng, Size n);

    // clang-format on
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/shift_left.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT inline constexpr struct shift_left_t final
      : hpx::detail::tag_dispatch<shift_left_t,
            hpx::functional::detail::tag_fallback<shift_left_t>>
    {
        template <typename FwdIter, typename Sent, typename Size>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, Sent last, Size n)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::shift_left<FwdIter>().call(
                hpx::execution::seq, first, last, n);
        }

        template <typename ExPolicy, typename FwdIter, typename Sent,
            typename Size>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            FwdIter>
        invoke_default(ExPolicy&& policy, FwdIter first, Sent last, Size n)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::shift_left<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, n);
        }

        template <typename Rng, typename Size>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static std::ranges::iterator_t<Rng> invoke_default(Rng&& rng, Size n)
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::shift_left<
                std::ranges::iterator_t<Rng>>()
                .call(hpx::execution::seq, std::begin(rng), std::end(rng), n);
        }

        template <typename ExPolicy, typename Rng, typename Size>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            std::ranges::iterator_t<Rng>>
        invoke_default(ExPolicy&& policy, Rng&& rng, Size n)
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::shift_left<
                std::ranges::iterator_t<Rng>>()
                .call(HPX_FORWARD(ExPolicy, policy), std::begin(rng),
                    std::end(rng), n);
        }
    } shift_left{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
