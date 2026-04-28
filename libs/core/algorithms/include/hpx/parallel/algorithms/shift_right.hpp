//  Copyright (c) 2007-2017 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/shift_right.hpp
/// \page hpx::shift_right
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Shifts the elements in the range [first, last) by n positions towards
    /// the end of the range. For every integer i in [0, last - first - n),
    /// moves the element originally at position first + i to position first
    /// + n + i.
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
    /// The assignment operations in the parallel \a shift_right algorithm
    /// invoked without an execution policy object will execute in sequential
    /// order in the calling thread.
    ///
    /// \note The type of dereferenced \a FwdIter must meet the requirements
    ///       of \a MoveAssignable.
    ///
    /// \returns  The \a shift_right algorithm returns \a FwdIter.
    ///           The \a shift_right algorithm returns an iterator to the
    ///           beginning of the resulting range. Specifically:
    ///           If \a n is 0 or \a n >= (last - first), does nothing and
    ///           returns \a first and \a last respectively.
    ///           Otherwise returns \a first + n.
    ///
    template <typename FwdIter, typename Size>
    FwdIter shift_right(FwdIter first, FwdIter last, Size n);

    ///////////////////////////////////////////////////////////////////////////
    /// Shifts the elements in the range [first, last) by n positions towards
    /// the end of the range. For every integer i in [0, last - first - n),
    /// moves the element originally at position first + i to position first
    /// + n + i. Executed according to the policy.
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
    /// The assignment operations in the parallel \a shift_right algorithm
    /// invoked with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignment operations in the parallel \a shift_right algorithm
    /// invoked with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note The type of dereferenced \a FwdIter must meet the requirements
    ///       of \a MoveAssignable.
    ///
    /// \returns  The \a shift_right algorithm returns a
    ///           \a hpx::future<FwdIter> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a shift_right algorithm returns an iterator to the
    ///           beginning of the resulting range. Specifically:
    ///           If \a n is 0, does nothing and returns \a first.
    ///           If \a n >= (last - first), does nothing and returns \a last.
    ///           Otherwise returns \a first + n.
    ///
    template <typename ExPolicy, typename FwdIter, typename Size>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy, FwdIter>
    shift_right(ExPolicy&& policy, FwdIter first, FwdIter last, Size n);

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/parallel/algorithms/copy.hpp>
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
    // shift_right
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename FwdIter,
            typename Sent>
        hpx::future<FwdIter> shift_right_helper(
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
                        [=](hpx::future<FwdIter>&& f) mutable -> FwdIter {
                            f.get();
                            std::advance(
                                first, detail::distance(new_first, last));
                            return first;
                        });
                },
                r.call2(p, non_seq(), first, new_first));
        }

        // Sequential shift_right implementation borrowed from
        // https://github.com/danra/shift_proposal

        HPX_CXX_CORE_EXPORT template <typename I>
        using difference_type_t =
            typename std::iterator_traits<I>::difference_type;

        HPX_CXX_CORE_EXPORT template <typename I>
        using iterator_category_t =
            typename std::iterator_traits<I>::iterator_category;

        HPX_CXX_CORE_EXPORT template <typename I, typename Tag, typename = void>
        inline constexpr bool is_category = false;

        HPX_CXX_CORE_EXPORT template <typename I, typename Tag>
        inline constexpr bool is_category<I, Tag,
            std::enable_if_t<
                std::is_convertible_v<iterator_category_t<I>, Tag>>> = true;

        HPX_CXX_CORE_EXPORT template <typename FwdIter>
        constexpr FwdIter sequential_shift_right(FwdIter first, FwdIter last,
            difference_type_t<FwdIter> n, std::size_t dist)
        {
            if constexpr (is_category<FwdIter, std::bidirectional_iterator_tag>)
            {
                auto mid = std::next(first, dist - n);
                return std::move_backward(
                    HPX_MOVE(first), HPX_MOVE(mid), HPX_MOVE(last));
            }
            else
            {
                auto result = std::next(first, n);
                auto lead = result;
                auto trail = first;

                for (/**/; trail != result; ++lead, void(++trail))
                {
                    if (lead == last)
                    {
                        std::move(HPX_MOVE(first), HPX_MOVE(trail), result);
                        return result;
                    }
                }

                for (;;)
                {
                    for (auto mid = first; mid != result;
                        ++lead, void(++trail), ++mid)
                    {
                        if (lead == last)
                        {
                            trail = std::move(mid, result, HPX_MOVE(trail));
                            std::move(HPX_MOVE(first), HPX_MOVE(mid),
                                HPX_MOVE(trail));
                            return result;
                        }
                        std::ranges::iter_swap(mid, trail);
                    }
                }
            }
        }

        HPX_CXX_CORE_EXPORT template <typename FwdIter2>
        struct shift_right : public algorithm<shift_right<FwdIter2>, FwdIter2>
        {
            constexpr shift_right() noexcept
              : algorithm<shift_right, FwdIter2>("shift_right")
            {
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename Size>
            static constexpr FwdIter sequential(
                ExPolicy, FwdIter first, Sent last, Size n)
            {
                auto dist =
                    static_cast<std::size_t>(detail::distance(first, last));
                // C++20 [alg.shift]: if n is 0, do nothing and return first.
                if (n == 0)
                {
                    return first;
                }
                // C++20 [alg.shift]: if n >= dist, do nothing and return last.
                if (static_cast<std::size_t>(n) >= dist)
                {
                    return std::next(first, dist);
                }

                auto last_iter = detail::advance_to_sentinel(first, last);
                return detail::sequential_shift_right(
                    first, last_iter, difference_type_t<FwdIter>(n), dist);
            }

            template <typename ExPolicy, typename Sent, typename Size>
            static typename util::detail::algorithm_result<ExPolicy,
                FwdIter2>::type
            parallel(ExPolicy&& policy, FwdIter2 first, Sent last, Size n)
            {
                auto dist =
                    static_cast<std::size_t>(detail::distance(first, last));
                // C++20 [alg.shift]: if n is 0, do nothing and return first.
                if (n == 0)
                {
                    return parallel::util::detail::algorithm_result<ExPolicy,
                        FwdIter2>::get(HPX_MOVE(first));
                }
                // C++20 [alg.shift]: if n >= dist, do nothing and return last.
                if (static_cast<std::size_t>(n) >= dist)
                {
                    return parallel::util::detail::algorithm_result<ExPolicy,
                        FwdIter2>::get(std::next(first, dist));
                }

                auto new_first = std::next(first, dist - n);
                return util::detail::algorithm_result<ExPolicy, FwdIter2>::get(
                    shift_right_helper(policy, first, last, new_first));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::shift_right
    HPX_CXX_CORE_EXPORT inline constexpr struct shift_right_t final
      : hpx::functional::detail::tag_fallback<shift_right_t>
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
            shift_right_t, FwdIter first, FwdIter last, Size n)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::shift_right<FwdIter>().call(
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
            FwdIter>::type tag_fallback_invoke(shift_right_t, ExPolicy&& policy,
            FwdIter first, FwdIter last, Size n)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::shift_right<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, n);
        }
    } shift_right{};
}    // namespace hpx

#endif    // DOXYGEN
