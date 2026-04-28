//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/shift_left.hpp
/// \page hpx::shift_left
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
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
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     positions to shift by.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
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
    ///           end of the resulting range. Specifically:
    ///           If \a n is 0 or \a n >= (last - first), does nothing and
    ///           returns \a last and \a first respectively.
    ///           Otherwise returns \a first + ((last - first) - n).
    ///
    template <typename FwdIter, typename Size>
    FwdIter shift_left(FwdIter first, FwdIter last, Size n);

    ///////////////////////////////////////////////////////////////////////////
    /// Shifts the elements in the range [first, last) by n positions towards
    /// the beginning of the range. For every integer i in [0, last - first
    ///  - n), moves the element originally at position first + n + i to
    /// position first + i. Executed according to the policy.
    ///
    /// \note   Complexity: At most (last - first) - n assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     positions to shift by.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
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
    ///           end of the resulting range. Specifically:
    ///           If \a n is 0, does nothing and returns \a last.
    ///           If \a n >= (last - first), does nothing and returns \a first.
    ///           Otherwise returns \a first + ((last - first) - n).
    ///
    template <typename ExPolicy, typename FwdIter, typename Size>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy, FwdIter>
    shift_left(ExPolicy&& policy, FwdIter first, FwdIter last, Size n);

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/reverse.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/transfer.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // shift_left
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename FwdIter,
            typename Sent>
        hpx::future<FwdIter> shift_left_helper(
            ExPolicy policy, FwdIter first, Sent last, FwdIter new_first)
        {
            using non_seq = std::false_type;
            auto p = hpx::execution::parallel_task_policy()
                         .on(policy.executor())
                         .with(policy.parameters());

            detail::reverse<FwdIter> r;
            return dataflow(
                [=](hpx::future<FwdIter>&& f1) mutable -> hpx::future<FwdIter> {
                    f1.get();

                    hpx::future<FwdIter> f = r.call2(p, non_seq(), first, last);
                    return f.then(
                        [=](hpx::future<FwdIter>&& fut) mutable -> FwdIter {
                            fut.get();
                            std::advance(
                                first, detail::distance(new_first, last));
                            return first;
                        });
                },
                r.call2(p, non_seq(), new_first, last));
        }

        // Sequential shift_left implementation inspired from
        // https://github.com/danra/shift_proposal

        HPX_CXX_CORE_EXPORT template <typename FwdIter, typename Sent,
            typename Size>
        constexpr FwdIter sequential_shift_left(
            FwdIter first, Sent last, Size n, std::size_t dist)
        {
            auto mid = std::next(first, n);

            if constexpr (std::random_access_iterator<FwdIter>)
            {
                return parallel::util::get_second_element(
                    util::move_n(mid, dist - n, HPX_MOVE(first)));
            }
            else
            {
                return parallel::util::get_second_element(
                    util::move(HPX_MOVE(mid), HPX_MOVE(last), HPX_MOVE(first)));
            }
        }

        HPX_CXX_CORE_EXPORT template <typename FwdIter2>
        struct shift_left : public algorithm<shift_left<FwdIter2>, FwdIter2>
        {
            constexpr shift_left() noexcept
              : algorithm<shift_left, FwdIter2>("shift_left")
            {
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename Size>
            static FwdIter sequential(
                ExPolicy, FwdIter first, Sent last, Size n)
            {
                auto dist =
                    static_cast<std::size_t>(detail::distance(first, last));
                // C++20 [alg.shift]: if n is 0, do nothing and return last.
                if (n == 0)
                {
                    return std::next(first, dist);
                }
                // C++20 [alg.shift]: if n >= dist, do nothing and return first.
                if (static_cast<std::size_t>(n) >= dist)
                {
                    return first;
                }

                return detail::sequential_shift_left(first, last, n, dist);
            }

            template <typename ExPolicy, typename Sent, typename Size>
            static typename util::detail::algorithm_result<ExPolicy,
                FwdIter2>::type
            parallel(ExPolicy&& policy, FwdIter2 first, Sent last, Size n)
            {
                auto const dist =
                    static_cast<std::size_t>(detail::distance(first, last));
                // C++20 [alg.shift]: if n is 0, do nothing and return last.
                if (n == 0)
                {
                    return parallel::util::detail::algorithm_result<ExPolicy,
                        FwdIter2>::get(std::next(first, dist));
                }
                // C++20 [alg.shift]: if n >= dist, do nothing and return first.
                if (static_cast<std::size_t>(n) >= dist)
                {
                    return parallel::util::detail::algorithm_result<ExPolicy,
                        FwdIter2>::get(HPX_MOVE(first));
                }

                return util::detail::algorithm_result<ExPolicy, FwdIter2>::get(
                    shift_left_helper(
                        policy, first, last, std::next(first, n)));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::shift_left
    HPX_CXX_CORE_EXPORT inline constexpr struct shift_left_t final
      : hpx::functional::detail::tag_fallback<shift_left_t>
    {
    private:
        template <typename FwdIter, typename Size>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            shift_left_t, FwdIter first, FwdIter last, Size n)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::shift_left<FwdIter>().call(
                hpx::execution::seq, first, last, n);
        }

        template <typename ExPolicy, typename FwdIter, typename Size>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        friend typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
            FwdIter>::type tag_fallback_invoke(shift_left_t, ExPolicy&& policy,
            FwdIter first, FwdIter last, Size n)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::shift_left<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, n);
        }
    } shift_left{};
}    // namespace hpx

#endif    // DOXYGEN
