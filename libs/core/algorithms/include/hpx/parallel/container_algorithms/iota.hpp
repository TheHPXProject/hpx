//  Copyright (c) 2026 Anfsity
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if defined(DOXYGEN)

namespace hpx::ranges {

    /// Fills the range [first, last) with sequentially increasing values,
    /// starting with \a value and repetitively evaluating \c ++value.
    ///
    /// \note   Complexity: Exactly \a std::distance(first, last) assignments
    ///         and increments.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input or output iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam T           The type of the target value. This type must meet
    ///                     the requirements of weakly incrementable.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param value        The value to be stored in the first element.
    ///
    /// The assignments in the \a iota algorithm invoked without
    /// an execution policy object will execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a iota algorithm returns a \a hpx::ranges::iota_result
    ///           containing the final iterator and the final value.
    ///
    template <typename FwdIter, typename Sent, typename T>
    iota_result<FwdIter, T> iota(FwdIter first, Sent last, T value);

    /// Fills the range [first, last) with sequentially increasing values,
    /// starting with \a value and repetitively evaluating \c ++value.
    ///
    /// \note   Complexity: Exactly \a std::distance(first, last) assignments
    ///         and increments.
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
    /// \tparam T           The type of the target value. This type must meet
    ///                     the requirements of weakly incrementable.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param value        The value to be stored in the first element.
    ///
    /// The assignments in the parallel \a iota algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a iota algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a iota algorithm returns a \a hpx::future<iota_result>
    ///           if the execution policy is of type \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a iota_result otherwise.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent, typename T>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        iota_result<FwdIter, T>>
    iota(ExPolicy&& policy, FwdIter first, Sent last, T value);

    /// Fills the range \a r with sequentially increasing values,
    /// starting with \a value and repetitively evaluating \c ++value.
    ///
    /// \note   Complexity: Exactly \a std::ranges::distance(r) assignments
    ///         and increments.
    ///
    /// \tparam Range       The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input or output iterator
    //                      at least.
    /// \tparam T           The type of the target value. This type must meet
    ///                     the requirements of weakly incrementable.
    ///
    /// \param r            Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param value        The value to be stored in the first element.
    ///
    /// The assignments in the \a iota algorithm invoked without
    /// an execution policy object will execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a iota algorithm returns a \a hpx::ranges::iota_result
    ///           containing the final iterator and the final value.
    ///
    template <typename Range, typename T>
    iota_result<typename hpx::traits::range_traits<Range>::iterator_type, T>
    iota(Range&& r, T value);

    /// Fills the range \a r with sequentially increasing values,
    /// starting with \a value and repetitively evaluating \c ++value.
    ///
    /// \note   Complexity: Exactly \a std::ranges::distance(r) assignments
    ///         and increments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Range       The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam T           The type of the target value. This type must meet
    ///                     the requirements of weakly incrementable.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param r            Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param value        The value to be stored in the first element.
    ///
    /// The assignments in the parallel \a iota algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a iota algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a iota algorithm returns a \a hpx::future<iota_result>
    ///           if the execution policy is of type \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a iota_result otherwise.
    ///
    template <typename ExPolicy, typename Range, typename T>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        iota_result<typename hpx::traits::range_traits<Range>::iterator_type,
            T>>
    iota(ExPolicy&& policy, Range&& r, T value);

}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/concepts/concepts.hpp>
#include <hpx/iterator_support/traits/is_range.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/iota.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <concepts>
#include <iterator>
#include <type_traits>

namespace hpx::ranges {

    template <typename I, typename O>
    using iota_result = out_value_result<I, O>;

    HPX_CXX_CORE_EXPORT inline constexpr struct iota_t final
      : hpx::detail::tag_parallel_algorithm<iota_t>
    {
    private:
        template <typename ExPolicy, std::forward_iterator FwdIter,
            std::sentinel_for<FwdIter> Sent, std::weakly_incrementable T>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<std::decay_t<ExPolicy>> &&
                std::indirectly_writable<FwdIter, T const&>
            )
        // clang-format on
        friend hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            iota_result<FwdIter, T>> tag_fallback_invoke(iota_t,
            ExPolicy&& policy, FwdIter first, Sent last, T value)
        {
            auto dist = hpx::parallel::detail::distance(first, last);

            // clang-format off
            return hpx::parallel::util::detail::convert_to_result(
                hpx::parallel::detail::iota<FwdIter>().call(
                    HPX_FORWARD(ExPolicy, policy),
                    first,
                    last,
                    value),
                [value, dist](
                    FwdIter const& last_iter) -> iota_result<FwdIter, T> {
                    return {last_iter, static_cast<T>(value + dist)};
                });
            // clang-format on
        }

        template <typename ExPolicy, std::weakly_incrementable T,
            std::ranges::output_range<T const&> Range>
            requires(hpx::is_execution_policy_v<std::decay_t<ExPolicy>>)
        friend hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            iota_result<
                typename hpx::traits::range_traits<Range>::iterator_type, T>>
        tag_fallback_invoke(iota_t, ExPolicy&& policy, Range&& r, T value)
        {
            return tag_fallback_invoke(iota_t{}, HPX_FORWARD(ExPolicy, policy),
                hpx::util::begin(r), hpx::util::end(r), value);
        }

        template <std::input_or_output_iterator FwdIter,
            std::sentinel_for<FwdIter> Sent, std::weakly_incrementable T>
            requires(std::indirectly_writable<FwdIter, T const&>)
        friend iota_result<FwdIter, T> tag_fallback_invoke(
            iota_t, FwdIter first, Sent last, T value)
        {
            return tag_fallback_invoke(
                iota_t{}, hpx::execution::seq, first, last, value);
        }

        template <std::weakly_incrementable T,
            std::ranges::output_range<T const&> Range>
        friend iota_result<
            typename hpx::traits::range_traits<Range>::iterator_type, T>
        tag_fallback_invoke(iota_t, Range&& r, T value)
        {
            return tag_fallback_invoke(iota_t{}, hpx::execution::seq,
                hpx::util::begin(r), hpx::util::end(r), value);
        }
    } iota{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
