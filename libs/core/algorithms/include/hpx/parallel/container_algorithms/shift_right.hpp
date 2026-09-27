//  Copyright (c) 2015-2023 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/shift_right.hpp
/// \page hpx::ranges::shift_right
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
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
    /// The assignment operations in the parallel \a shift_right algorithm
    /// invoked without an execution policy object will execute in sequential
    /// order in the calling thread.
    ///
    /// \note The type of dereferenced \a FwdIter must meet the requirements
    ///       of \a MoveAssignable.
    ///
    /// \returns  The \a shift_right algorithm returns \a FwdIter.
    ///           The \a shift_right algorithm returns an iterator to the
    ///           end of the resulting range.
    ///
    template <typename FwdIter, typename Sent, typename Size>
    FwdIter shift_right(FwdIter first, Sent last, Size n);

    ///////////////////////////////////////////////////////////////////////////
    /// Shifts the elements in the range [first, last) by n positions towards
    /// the end of the range. For every integer i in [0, last - first - n),
    /// moves the element originally at position first + i to position first
    /// + n + i.
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
    /// The assignment operations in the parallel \a shift_right algorithm
    /// invoked without an execution policy object will execute in sequential
    /// order in the calling thread.
    ///
    /// \note The type of dereferenced \a std::ranges::iterator_t<Rng>
    ///       must meet the requirements of \a MoveAssignable.
    ///
    /// \returns  The \a shift_right algorithm returns \a
    ///           std::ranges::iterator_t<Rng>.
    ///           The \a shift_right algorithm returns an iterator to the
    ///           end of the resulting range.
    ///
    template <typename Rng, typename Size>
    std::ranges::iterator_t<Rng> shift_right(Rng&& rng, Size n);

    // clang-format on

    /// \brief Execution-policy overload of \c shift_right.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S>
        requires hpx::is_execution_policy_v<ExPolicy> && std::permutable<I>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I>>
    shift_right(
        ExPolicy&& policy, I first, S last, std::iter_difference_t<I> count);

    /// \brief Execution-policy overload of \c shift_right.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::permutable<std::ranges::iterator_t<R>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R>>
    shift_right(ExPolicy&& policy, R&& rng,
        std::iter_difference_t<std::ranges::iterator_t<R>> count);
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/shift_right.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT inline constexpr struct shift_right_t final
      : hpx::detail::tag_dispatch<shift_right_t,
            hpx::detail::tag_parallel_algorithm<shift_right_t>>
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

            return hpx::parallel::detail::shift_right<FwdIter>().call(
                hpx::execution::seq, first, last, n);
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S>
            requires hpx::is_execution_policy_v<ExPolicy> && std::permutable<I>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, std::iter_difference_t<I> count)
        {
            auto end = first + (last - first);
            return parallel::util::detail::convert_to_result(
                parallel::detail::shift_right<I>().call(
                    HPX_FORWARD(ExPolicy, policy), first, end, count),
                [end](I position) -> std::ranges::subrange<I> {
                    return {position, end};
                });
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

            return hpx::parallel::detail::shift_right<
                std::ranges::iterator_t<Rng>>()
                .call(hpx::execution::seq, std::begin(rng), std::end(rng), n);
        }

        template <typename ExPolicy, std::ranges::random_access_range R>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::permutable<std::ranges::iterator_t<R>>
        static decltype(auto) invoke_default(ExPolicy&& policy, R&& rng,
            std::iter_difference_t<std::ranges::iterator_t<R>> count)
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), count),
                [](std::ranges::subrange<I> result)
                    -> std::ranges::borrowed_subrange_t<R> { return result; });
        }
    } shift_right{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
