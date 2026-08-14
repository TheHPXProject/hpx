//  Copyright (c) 2026 Anfsity
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if defined(DOXYGEN)

namespace hpx {

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
    /// \tparam T           The type of the value to be assigned (deduced).
    ///                     This type must be weakly incrementable.
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
    /// \returns  The \a iota algorithm returns \a void.
    ///
    template <typename FwdIter, typename Sent, typename T>
    void iota(FwdIter first, Sent last, T value);

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
    /// \tparam T           The type of the value to be assigned (deduced).
    ///                     This type must be weakly incrementable.
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
    /// \returns  The \a iota algorithm returns a
    ///           \a hpx::future<void> if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a void otherwise.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent, typename T>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy> iota(
        ExPolicy&& policy, FwdIter first, Sent last, T value);

}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/concepts/concepts.hpp>
#include <hpx/parallel/algorithms/detail/iota.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <concepts>
#include <iterator>
#include <type_traits>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::iota
    HPX_CXX_CORE_EXPORT inline constexpr struct iota_t final
      : hpx::detail::tag_dispatch<iota_t,
            hpx::detail::tag_parallel_algorithm<iota_t>>
    {
        // parallel
        template <typename Expolicy, std::forward_iterator FwdIter,
            std::sentinel_for<FwdIter> Sent, std::weakly_incrementable T>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<std::decay_t<Expolicy>> &&
                std::indirectly_writable<FwdIter, T const&>
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<Expolicy>
        invoke_default(Expolicy&& policy, FwdIter first, Sent last, T value)
        {
            using result_type =
                hpx::parallel::util::detail::algorithm_result<Expolicy>::type;

            return hpx::util::void_guard<result_type>(),
                   hpx::parallel::detail::iota<FwdIter>().call(
                       HPX_FORWARD(Expolicy, policy), first, last, value);
        }

        // sequential
        template <std::input_or_output_iterator FwdIter,
            std::sentinel_for<FwdIter> Sent, std::weakly_incrementable T>
            requires(std::indirectly_writable<FwdIter, T const&>)
        static void invoke_default(FwdIter first, Sent last, T value)
        {
            hpx::parallel::detail::iota<FwdIter>().call(
                hpx::execution::seq, first, last, value);
        }

    } iota{};

}    // namespace hpx

#endif    // DOXYGEN
