//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c)      2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/reverse.hpp
/// \page hpx::ranges::reverse, hpx::ranges::reverse_copy
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {

    ///////////////////////////////////////////////////////////////////////////
    /// Reverses the order of the elements in the range [first, last).
    /// Behaves as if applying std::iter_swap to every pair of iterators
    /// first+i, (last-i) - 1 for each non-negative i < (last-first)/2.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last.
    ///
    /// \tparam Iter        The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for Iter.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    ///
    /// The assignments in the parallel \a reverse algorithm
    /// execute in sequential order in the calling thread.
    ///
    ///
    /// \returns  The \a reverse algorithm returns a \a Iter.
    ///           It returns \a last.
    ///
    template <typename Iter, typename Sent>
    Iter reverse(Iter first, Sent sent);

    /// Uses \a rng as the source range, as if using \a util::begin(rng) as
    /// \a first and \a ranges::end(rng) as \a last.
    /// Reverses the order of the elements in the range [first, last).
    /// Behaves as if applying std::iter_swap to every pair of iterators
    /// first+i, (last-i) - 1 for each non-negative i < (last-first)/2.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a bidirectional iterator.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    ///
    /// The assignments in the parallel \a reverse algorithm
    /// execute in sequential order in the calling thread.
    ///
    ///
    /// \returns  The \a reverse algorithm returns a
    ///           \a hpx::traits::range_iterator<Rng>::type.
    ///           It returns \a last.
    ///
    template <typename Rng>
    std::ranges::iterator_t<Rng> reverse(Rng&& rng);

    ///////////////////////////////////////////////////////////////////////////
    /// Copies the elements from the range [first, last) to another range
    /// beginning at result in such a way that the elements in the new
    /// range are in reverse order.
    /// Behaves as if by executing the assignment
    /// *(result + (last - first) - 1 - i) = *(first + i) once for each
    /// non-negative i < (last - first)
    /// If the source and destination ranges (that is, [first, last) and
    /// [result, result+(last-first)) respectively) overlap, the
    /// behavior is undefined.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam Iter        The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for Iter.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param result   Refers to the begin of the destination range.
    ///
    /// The assignments in the parallel \a reverse_copy algorithm
    /// execute in sequential order in the calling thread.
    ///
    ///
    /// \returns  The \a reverse_copy algorithm returns a
    ///           \a reverse_copy_result<Iter, OutIter>.
    ///           The \a reverse_copy algorithm returns the pair of the input iterator
    ///           forwarded to the first element after the last in the input
    ///           sequence and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename Iter, typename Sent, typename OutIter>
    reverse_copy_result<Iter, OutIter> reverse_copy(
        Iter first, Sent last, OutIter result);

    /// Uses \a rng as the source range, as if using \a util::begin(rng) as
    /// \a first and \a ranges::end(rng) as \a last.
    /// Copies the elements from the range [first, last) to another range
    /// beginning at result in such a way that the elements in the new
    /// range are in reverse order.
    /// Behaves as if by executing the assignment
    /// *(result + (last - first) - 1 - i) = *(first + i) once for each
    /// non-negative i < (last - first)
    /// If the source and destination ranges (that is, [first, last) and
    /// [result, result+(last-first)) respectively) overlap, the
    /// behavior is undefined.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a bidirectional iterator.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param result       Refers to the begin of the destination range.
    ///
    /// The assignments in the parallel \a reverse_copy algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a reverse_copy algorithm returns a
    ///           \a ranges::reverse_copy_result<
    ///           std::ranges::iterator_t<Rng>, OutIter>.
    ///           The \a reverse_copy algorithm returns
    ///           an object equal to {last, result + N} where N = last - first
    ///
    template <typename Rng, typename OutIter>
    reverse_copy_result<std::ranges::iterator_t<Rng>, OutIter> reverse_copy(
        Rng&& rng, OutIter result);

    /// Copies the elements from the range [first, last) to another range
    /// beginning at result in such a way that the elements in the new
    /// range are in reverse order.
    /// Behaves as if by executing the assignment
    /// *(result + (last - first) - 1 - i) = *(first + i) once for each
    /// non-negative i < (last - first)
    /// If the source and destination ranges (that is, [first, last) and
    /// [result, result+(last-first)) respectively) overlap, the
    /// behavior is undefined.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Iter        The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for Iter.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param result   Refers to the begin of the destination range.
    ///
    /// The assignments in the parallel \a reverse_copy algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a reverse_copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a reverse_copy algorithm returns a
    ///           \a hpx::future<reverse_copy_result<Iter, FwdIter> >
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a reverse_copy_result<Iter, FwdIter>
    ///           otherwise.
    ///           The \a reverse_copy algorithm returns the pair of the input iterator
    ///           forwarded to the first element after the last in the input
    ///           sequence and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename Iter, typename Sent, typename FwdIter>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        reverse_copy_result<Iter, FwdIter>>::type
    reverse_copy(ExPolicy&& policy, Iter first, Sent last, FwdIter result);

    /// Uses \a rng as the source range, as if using \a util::begin(rng) as
    /// \a first and \a ranges::end(rng) as \a last.
    /// Copies the elements from the range [first, last) to another range
    /// beginning at result in such a way that the elements in the new
    /// range are in reverse order.
    /// Behaves as if by executing the assignment
    /// *(result + (last - first) - 1 - i) = *(first + i) once for each
    /// non-negative i < (last - first)
    /// If the source and destination ranges (that is, [first, last) and
    /// [result, result+(last-first)) respectively) overlap, the
    /// behavior is undefined.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a bidirectional iterator.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param result   Refers to the begin of the destination range.
    ///
    /// The assignments in the parallel \a reverse_copy algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a reverse_copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a reverse_copy algorithm returns a
    ///           \a hpx::future<ranges::reverse_copy_result<
    ///            std::ranges::iterator_t<Rng>, OutIter>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a ranges::reverse_copy_result<
    ///            std::ranges::iterator_t<Rng>, OutIter>
    ///           otherwise.
    ///           The \a reverse_copy algorithm returns
    ///           an object equal to {last, result + N} where N = last - first
    ///
    template <typename ExPolicy, typename Rng, typename OutIter>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        reverse_copy_result<std::ranges::iterator_t<Rng>, OutIter>>::type
    reverse_copy(ExPolicy&& policy, Rng&& rng, OutIter result);

    /// \brief Execution-policy overload of \c reverse.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S>
        requires hpx::is_execution_policy_v<ExPolicy> && std::permutable<I>
    parallel::util::detail::algorithm_result_t<ExPolicy, I> reverse(
        ExPolicy&& policy, I first, S last);

    /// \brief Execution-policy overload of \c reverse.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::permutable<std::ranges::iterator_t<R>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_iterator_t<R>>
    reverse(ExPolicy&& policy, R&& rng);

    /// \brief Execution-policy overload of \c reverse_copy.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// The operation is bounded by the supplied output range(s). Returned
    /// input positions identify where processing can resume.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, std::random_access_iterator O,
        std::sized_sentinel_for<O> OutS>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_copyable<I, O>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        reverse_copy_result<I, O>>
    reverse_copy(ExPolicy&& policy, I first, S last, O dest, OutS dest_last);

    /// \brief Execution-policy overload of \c reverse_copy.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// The operation is bounded by the supplied output range(s). Returned
    /// input positions identify where processing can resume.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        std::ranges::random_access_range OutR>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> && std::ranges::sized_range<OutR> &&
        std::indirectly_copyable<std::ranges::iterator_t<R>,
            std::ranges::iterator_t<OutR>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        reverse_copy_result<std::ranges::borrowed_iterator_t<R>,
            std::ranges::borrowed_iterator_t<OutR>>>
    reverse_copy(ExPolicy&& policy, R&& rng, OutR&& output);
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/reverse.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    /// `reverse_copy_result` is equivalent to
    /// `hpx::parallel::util::in_out_result`
    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using reverse_copy_result = hpx::parallel::util::in_out_result<I, O>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::reverse
    HPX_CXX_CORE_EXPORT inline constexpr struct reverse_t final
      : hpx::detail::tag_dispatch<reverse_t,
            hpx::detail::tag_parallel_algorithm<reverse_t>>
    {
        template <typename Iter, typename Sent>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<Iter> &&
                std::sentinel_for<Sent, Iter>
            )
        // clang-format on
        static Iter invoke_default(Iter first, Sent sent)
        {
            static_assert(std::bidirectional_iterator<Iter>,
                "Required at least bidirectional iterator.");

            return parallel::detail::reverse<Iter>().call(
                hpx::execution::sequenced_policy{}, first, sent);
        }

        template <typename Rng>
            requires(std::ranges::range<Rng>)
        static std::ranges::iterator_t<Rng> invoke_default(Rng&& rng)
        {
            static_assert(
                std::bidirectional_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least bidirectional iterator.");

            return parallel::detail::reverse<std::ranges::iterator_t<Rng>>()
                .call(hpx::execution::sequenced_policy{}, hpx::util::begin(rng),
                    hpx::util::end(rng));
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S>
            requires hpx::is_execution_policy_v<ExPolicy> && std::permutable<I>
        static decltype(auto) invoke_default(ExPolicy&& policy, I first, S last)
        {
            auto end = first + (last - first);
            return parallel::detail::reverse<I>().call(
                HPX_FORWARD(ExPolicy, policy), first, end);
        }

        template <typename ExPolicy, std::ranges::random_access_range R>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::permutable<std::ranges::iterator_t<R>>
        static decltype(auto) invoke_default(ExPolicy&& policy, R&& rng)
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng)),
                [](I result) -> std::ranges::borrowed_iterator_t<R> {
                    return result;
                });
        }
    } reverse{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::reverse_copy
    HPX_CXX_CORE_EXPORT inline constexpr struct reverse_copy_t final
      : hpx::detail::tag_dispatch<reverse_copy_t,
            hpx::detail::tag_parallel_algorithm<reverse_copy_t>>
    {
        template <typename Iter, typename Sent, typename OutIter>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<Iter> &&
                std::sentinel_for<Sent, Iter> &&
                hpx::traits::is_iterator_v<OutIter>
            )
        // clang-format on
        static reverse_copy_result<Iter, OutIter> invoke_default(
            Iter first, Sent last, OutIter result)
        {
            static_assert(std::bidirectional_iterator<Iter>,
                "Required at least bidirectional iterator.");

            static_assert(
                std::output_iterator<OutIter, hpx::traits::iter_value_t<Iter>>,
                "Required at least output iterator.");

            return parallel::detail::reverse_copy<
                hpx::parallel::util::in_out_result<Iter, OutIter>>()
                .call(hpx::execution::sequenced_policy{}, first, last, result);
        }

        template <typename Rng, typename OutIter>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<OutIter>
            )
        // clang-format on
        static reverse_copy_result<std::ranges::iterator_t<Rng>, OutIter>
        invoke_default(Rng&& rng, OutIter result)
        {
            static_assert(
                std::bidirectional_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least bidirectional iterator.");

            static_assert(
                std::output_iterator<OutIter,
                    hpx::traits::iter_value_t<std::ranges::iterator_t<Rng>>>,
                "Required at least output iterator.");

            return parallel::detail::reverse_copy<hpx::parallel::util::
                    in_out_result<std::ranges::iterator_t<Rng>, OutIter>>()
                .call(hpx::execution::sequenced_policy{}, hpx::util::begin(rng),
                    hpx::util::end(rng), result);
        }

        template <typename ExPolicy, typename Iter, typename Sent,
            typename FwdIter>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<Iter> &&
                std::sentinel_for<Sent, Iter> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            reverse_copy_result<Iter, FwdIter>>
        invoke_default(ExPolicy&& policy, Iter first, Sent last, FwdIter result)
        {
            static_assert(std::bidirectional_iterator<Iter>,
                "Required at least bidirectional iterator.");

            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return parallel::detail::reverse_copy<
                hpx::parallel::util::in_out_result<Iter, FwdIter>>()
                .call(HPX_FORWARD(ExPolicy, policy), first, last, result);
        }

        template <typename ExPolicy, typename Rng, typename OutIter>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<OutIter>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            reverse_copy_result<std::ranges::iterator_t<Rng>, OutIter>>
        invoke_default(ExPolicy&& policy, Rng&& rng, OutIter result)
        {
            static_assert(
                std::bidirectional_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least bidirectional iterator.");

            static_assert(
                std::output_iterator<OutIter,
                    hpx::traits::iter_value_t<std::ranges::iterator_t<Rng>>>,
                "Required at least output iterator.");

            return parallel::detail::reverse_copy<hpx::parallel::util::
                    in_out_result<std::ranges::iterator_t<Rng>, OutIter>>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                    hpx::util::end(rng), result);
        }

        /// \brief Copy reordered elements into a bounded destination.
        /// \returns Input and output resume positions, or their future.
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_copyable<I, O>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, O dest, OutS dest_last)
        {
            using difference_type =
                std::common_type_t<std::iter_difference_t<I>,
                    std::iter_difference_t<O>>;
            auto const size = difference_type(last - first);
            auto const count =
                (std::min) (size, difference_type(dest_last - dest));
            auto resume = first + size - count;
            return parallel::util::detail::convert_to_result(
                parallel::detail::reverse_copy<reverse_copy_result<I, O>>()
                    .call(HPX_FORWARD(ExPolicy, policy), resume, first + size,
                        dest),
                [resume](reverse_copy_result<I, O> result) {
                    return reverse_copy_result<I, O>{resume, result.out};
                });
        }

        /// \brief Copy a reordered range into a bounded output range.
        /// \returns Borrowed resume positions, or their future.
        template <typename ExPolicy, std::ranges::random_access_range R,
            std::ranges::random_access_range OutR>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> && std::ranges::sized_range<OutR> &&
            std::indirectly_copyable<std::ranges::iterator_t<R>,
                std::ranges::iterator_t<OutR>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, OutR&& output)
        {
            using iterator_result =
                reverse_copy_result<std::ranges::iterator_t<R>,
                    std::ranges::iterator_t<OutR>>;
            using result_type =
                reverse_copy_result<std::ranges::borrowed_iterator_t<R>,
                    std::ranges::borrowed_iterator_t<OutR>>;
            auto first = std::ranges::begin(rng);
            auto dest = std::ranges::begin(output);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), dest,
                    dest + std::ranges::distance(output)),
                [](iterator_result result) -> result_type {
                    return {result.in, result.out};
                });
        }
    } reverse_copy{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
