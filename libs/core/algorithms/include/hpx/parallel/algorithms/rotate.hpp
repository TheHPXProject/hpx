//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2021-2022 Chuanqiu He
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
/// \page hpx::rotate, hpx::rotate_copy
/// \headerfile hpx/algorithm.hpp
/// \file hpx/parallel/algorithms/rotate.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    /// Performs a left rotation on a range of elements. Specifically,
    /// \a rotate swaps the elements in the range [first, last) in such a way
    /// that the element new_first becomes the first element of the new range
    /// and new_first - 1 becomes the last element.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param new_first    Refers to the element that should appear at the
    ///                     beginning of the rotated range.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    ///
    /// The assignments in the parallel \a rotate algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \note The type of dereferenced \a FwdIter must meet the requirements
    ///       of \a MoveAssignable and \a MoveConstructible.
    ///
    /// \returns  The \a rotate algorithm returns a FwdIter.
    ///           The \a rotate algorithm returns the iterator to the new
    ///           location of the element pointed by first,equal to first +
    ///           (last - new_first).
    ///
    template <typename FwdIter>
    FwdIter rotate(FwdIter first, FwdIter new_first, FwdIter last);

    /// Performs a left rotation on a range of elements. Specifically,
    /// \a rotate swaps the elements in the range [first, last) in such a way
    /// that the element new_first becomes the first element of the new range
    /// and new_first - 1 becomes the last element. Executed according to the
    /// policy.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param new_first    Refers to the element that should appear at the
    ///                     beginning of the rotated range.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    ///
    /// The assignments in the parallel \a rotate algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a rotate algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note The type of dereferenced \a FwdIter must meet the requirements
    ///       of \a MoveAssignable and \a MoveConstructible.
    ///
    /// \returns  The \a rotate algorithm returns a \a hpx::future<FwdIter>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a rotate algorithm returns the iterator equal to
    ///           first + (last - new_first).
    ///
    template <typename ExPolicy, typename FwdIter>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter> rotate(
        ExPolicy&& policy, FwdIter first, FwdIter new_first, FwdIter last);

    /// Copies the elements from the range [first, last), to another range
    /// beginning at \a dest_first in such a way, that the element
    /// \a new_first becomes the first element of the new range and
    /// \a new_first - 1 becomes the last element.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of
    ///                     a forward iterator.
    /// \tparam OutIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of
    ///                     a output iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of
    ///                     elements the algorithm will be applied to.
    /// \param new_first    Refers to the element that should appear at the
    ///                     beginning of the rotated range.
    /// \param last         Refers to the end of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param dest_first   Refers to the begin of the destination range.
    ///
    /// The assignments in the parallel \a rotate_copy algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a rotate_copy algorithm returns a output iterator,
    ///           The \a rotate_copy algorithm returns the output iterator
    ///           to the element past the last element copied.
    ///
    template <typename FwdIter, typename OutIter>
    OutIter rotate_copy(
        FwdIter first, FwdIter new_first, FwdIter last, OutIter dest_first);

    /// Copies the elements from the range [first, last), to another range
    /// beginning at \a dest_first in such a way, that the element
    /// \a new_first becomes the first element of the new range and
    /// \a new_first - 1 becomes the last element. Executed according to
    /// the policy.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use
    ///                     (deduced). It describes the manner in which the
    ///                     execution of the algorithm may be parallelized
    ///                     and the manner in which it executes the
    ///                     assignments.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of
    ///                     a forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of
    ///                     a forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling
    ///                     of the iterations.
    /// \param first        Refers to the beginning of the sequence of
    ///                     elements the algorithm will be applied to.
    /// \param new_first    Refers to the element that should appear at the
    ///                     beginning of the rotated range.
    /// \param last         Refers to the end of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param dest_first   Refers to the begin of the destination range.
    ///
    /// The assignments in the parallel \a rotate_copy algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a rotate_copy algorithm
    /// execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a rotate_copy algorithm returns a
    ///           \a hpx::future<FwdIter2>
    ///           if the execution policy is of type
    ///           \a parallel_task_policy and
    ///           returns FwdIter2
    ///           otherwise.
    ///           The \a rotate_copy algorithm returns the output iterator
    ///           to the element past the last element copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    rotate_copy(ExPolicy&& policy, FwdIter1 first, FwdIter1 new_first,
        FwdIter1 last, FwdIter2 dest_first);

}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/copy.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/rotate.hpp>
#include <hpx/parallel/algorithms/reverse.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/transfer.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // rotate
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename FwdIter,
            typename Sent>
        decltype(auto) rotate_helper(
            ExPolicy policy, FwdIter first, FwdIter new_first, Sent last)
        {
            std::ptrdiff_t const size_left = detail::distance(first, new_first);
            std::ptrdiff_t size_right = detail::distance(new_first, last);

            // get number of cores currently used
            std::size_t const cores =
                hpx::execution::experimental::processing_units_count(
                    policy.parameters(), policy.executor(),
                    hpx::chrono::null_duration, size_left + size_right);

            // get currently used first core
            std::size_t first_core =
                hpx::execution::experimental::get_first_core(policy);

            // calculate number of cores to be used for left and right section
            // proportional to the ratio of their sizes
            std::size_t cores_left = 1;
            if (size_right > 0)
            {
                double const partition_size_ratio =
                    static_cast<double>(size_left) /
                    static_cast<double>(size_left + size_right);

                // avoid cores_left == 0 after integer rounding
                cores_left = (std::max) (static_cast<std::size_t>(1),
                    static_cast<std::size_t>(
                        partition_size_ratio * static_cast<double>(cores)));
            }

            // cores_right should be at least 1.
            std::size_t cores_right =
                (std::max) (static_cast<std::size_t>(1), cores - cores_left);

            // invoke the reverse operations on the left and right sections
            // concurrently
            auto p = policy(hpx::execution::task);

            auto rebound_left_params =
                hpx::execution::experimental::rebind_executor_parameters(
                    p.parameters(),
                    hpx::execution::experimental::num_cores(cores_left));
            auto left_policy =
                hpx::execution::experimental::create_rebound_policy(
                    p, rebound_left_params);

            auto rebound_right_params =
                hpx::execution::experimental::rebind_executor_parameters(
                    p.parameters(),
                    hpx::execution::experimental::num_cores(cores_right));
            auto right_policy =
                hpx::execution::experimental::create_rebound_policy(
                    hpx::execution::experimental::with_first_core(
                        p, cores == 1 ? first_core : first_core + cores_left),
                    rebound_right_params);

            detail::reverse<FwdIter> r;

            auto&& process = [=](auto&& f1, auto&& f2) mutable {
                // propagate exceptions, if appropriate
                constexpr bool handle_futures =
                    hpx::traits::is_future_v<decltype((f1))> &&
                    hpx::traits::is_future_v<decltype((f2))>;

                if constexpr (handle_futures)
                {
                    f1.get();
                    f2.get();
                }

                r.call(p(hpx::execution::non_task), first, last);

                std::advance(first, size_right);
                return util::in_out_result<FwdIter, Sent>{first, last};
            };

            using rcall_left_t =
                decltype(r.call(left_policy, first, new_first));
            using rcall_right_t =
                decltype(r.call(right_policy, new_first, last));

            // Sanity check
            static_assert(std::is_same_v<rcall_left_t, rcall_right_t>);

            constexpr bool handle_senders =
                hpx::execution::experimental::is_sender_v<rcall_left_t>;
            constexpr bool handle_futures =
                hpx::traits::is_future_v<rcall_left_t>;
            constexpr bool handle_both = handle_senders && handle_futures;

            static_assert(handle_senders || handle_futures,
                "the reverse operation must return either a sender or a "
                "future");

            // Futures pass the concept check for senders, so if something is
            // both a future and a sender we treat it as a future.
            if constexpr (handle_futures || handle_both)
            {
                return hpx::dataflow(hpx::launch::sync, std::move(process),
                    r.call(left_policy, first, new_first),
                    r.call(right_policy, new_first, last));
            }
            else if constexpr (handle_senders && !handle_both)
            {
                return hpx::execution::experimental::then(
                    hpx::execution::experimental::when_all(
                        r.call(left_policy, first, new_first),
                        r.call(right_policy, new_first, last)),
                    std::move(process));
            }
        }

        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct rotate : algorithm<rotate<IterPair>, IterPair>
        {
            constexpr rotate() noexcept
              : algorithm<rotate, IterPair>("rotate")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent>
            static constexpr IterPair sequential(
                ExPolicy, InIter first, InIter new_first, Sent last)
            {
                return detail::sequential_rotate(first, new_first, last);
            }

            template <typename ExPolicy, typename FwdIter, typename Sent>
            static decltype(auto) parallel(
                ExPolicy&& policy, FwdIter first, FwdIter new_first, Sent last)
            {
                return util::detail::algorithm_result<ExPolicy, IterPair>::get(
                    rotate_helper(
                        HPX_FORWARD(ExPolicy, policy), first, new_first, last));
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // rotate_copy
    namespace detail {
        /// \cond NOINTERNAL

        // sequential rotate_copy
        HPX_CXX_CORE_EXPORT template <typename InIter, typename Sent,
            typename OutIter>
        constexpr util::in_out_result<InIter, OutIter> sequential_rotate_copy(
            InIter first, InIter new_first, Sent last, OutIter dest_first)
        {
            util::in_out_result<InIter, OutIter> p1 =
                util::copy(new_first, last, dest_first);
            util::in_out_result<InIter, OutIter> p2 =
                util::copy(first, new_first, HPX_MOVE(p1.out));
            return util::in_out_result<InIter, OutIter>{
                HPX_MOVE(p1.in), HPX_MOVE(p2.out)};
        }

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename FwdIter1,
            typename Sent, typename FwdIter2>
        decltype(auto) rotate_copy_helper(ExPolicy policy, FwdIter1 first,
            FwdIter1 new_first, Sent last, FwdIter2 dest_first)
        {
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            using non_seq = std::false_type;
            using copy_return_type = util::in_out_result<FwdIter1, FwdIter2>;

            if constexpr (!has_scheduler_executor)
            {
                auto p = hpx::execution::parallel_task_policy()
                             .on(policy.executor())
                             .with(policy.parameters());

                hpx::future<copy_return_type> f =
                    detail::copy<copy_return_type>().call2(
                        p, non_seq(), new_first, last, dest_first);

                return f.then([p, first, new_first](
                                  hpx::future<copy_return_type>&& result)
                                  -> hpx::future<copy_return_type> {
                    copy_return_type p1 = result.get();
                    return detail::copy<copy_return_type>().call2(
                        p, non_seq(), first, new_first, p1.out);
                });
            }
            else
            {
                return detail::copy<copy_return_type>().call(
                           policy, new_first, last, dest_first) |
                    hpx::execution::experimental::let_value(
                        [policy, first, new_first](copy_return_type& result) {
                            return detail::copy<copy_return_type>().call(
                                policy, first, new_first, result.out);
                        });
            }
        }

        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct rotate_copy : algorithm<rotate_copy<IterPair>, IterPair>
        {
            constexpr rotate_copy() noexcept
              : algorithm<rotate_copy, IterPair>("rotate_copy")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent,
                typename OutIter>
            static constexpr util::in_out_result<InIter, OutIter> sequential(
                ExPolicy, InIter first, InIter new_first, Sent last,
                OutIter dest_first)
            {
                return sequential_rotate_copy(
                    first, new_first, last, dest_first);
            }

            template <typename ExPolicy, typename FwdIter1, typename Sent,
                typename FwdIter2>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter1 first,
                FwdIter1 new_first, Sent last, FwdIter2 dest_first)
            {
                return util::detail::algorithm_result<ExPolicy, IterPair>::get(
                    rotate_copy_helper(HPX_FORWARD(ExPolicy, policy), first,
                        new_first, last, dest_first));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::rotate
    HPX_CXX_CORE_EXPORT inline constexpr struct rotate_t final
      : hpx::detail::tag_parallel_algorithm<rotate_t>
    {
        template <typename FwdIter>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            hpx::rotate_t, FwdIter first, FwdIter new_first, FwdIter last)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return parallel::util::get_first_element(
                hpx::parallel::detail::rotate<
                    hpx::parallel::util::in_out_result<FwdIter, FwdIter>>()
                    .call(hpx::execution::seq, first, new_first, last));
        }

        template <typename ExPolicy, typename FwdIter>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::rotate_t,
            ExPolicy&& policy, FwdIter first, FwdIter new_first, FwdIter last)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            using is_seq = std::integral_constant<bool,
                hpx::is_sequenced_execution_policy_v<ExPolicy> ||
                    !std::bidirectional_iterator<FwdIter>>;

            return parallel::util::get_first_element(
                hpx::parallel::detail::rotate<
                    hpx::parallel::util::in_out_result<FwdIter, FwdIter>>()
                    .call2(HPX_FORWARD(ExPolicy, policy), is_seq(), first,
                        new_first, last));
        }
    } rotate{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::rotate_copy
    HPX_CXX_CORE_EXPORT inline constexpr struct rotate_copy_t final
      : hpx::detail::tag_parallel_algorithm<rotate_copy_t>
    {
        template <typename FwdIter, typename OutIter>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::traits::is_iterator_v<OutIter>
            )
        // clang-format on
        friend OutIter tag_fallback_invoke(hpx::rotate_copy_t, FwdIter first,
            FwdIter new_first, FwdIter last, OutIter dest_first)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");
            static_assert(std::output_iterator<OutIter,
                              hpx::traits::iter_value_t<FwdIter>>,
                "Requires at least output iterator.");

            return parallel::util::get_second_element(
                hpx::parallel::detail::rotate_copy<
                    hpx::parallel::util::in_out_result<FwdIter, OutIter>>()
                    .call(hpx::execution::seq, first, new_first, last,
                        dest_first));
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::rotate_copy_t,
            ExPolicy&& policy, FwdIter1 first, FwdIter1 new_first,
            FwdIter1 last, FwdIter2 dest_first)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            using is_seq = std::integral_constant<bool,
                hpx::is_sequenced_execution_policy_v<ExPolicy> ||
                    !std::forward_iterator<FwdIter1>>;

            return parallel::util::get_second_element(
                hpx::parallel::detail::rotate_copy<
                    hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call2(HPX_FORWARD(ExPolicy, policy), is_seq(), first,
                        new_first, last, dest_first));
        }
    } rotate_copy{};
}    // namespace hpx

#endif    // DOXYGEN
