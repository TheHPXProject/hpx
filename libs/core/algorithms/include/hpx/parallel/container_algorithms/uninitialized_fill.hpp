//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/uninitialized_fill.hpp
/// \page hpx::ranges::uninitialized_fill, hpx::ranges::uninitialized_fill_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    /// Copies the given \a value to an uninitialized memory area, defined by
    /// the range [first, last). If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param value        The value to be assigned.
    ///
    /// The assignments in the ranges \a uninitialized_fill algorithm invoked
    /// without an execution policy object will execute in sequential order in
    /// the calling thread.
    ///
    /// \returns  The \a uninitialized_fill algorithm returns a
    ///           returns \a FwdIter.
    ///           The \a uninitialized_fill algorithm returns the output
    ///           iterator to the element in the range, one past
    ///           the last element copied.
    ///
    template <typename FwdIter, typename Sent, typename T>
    FwdIter uninitialized_fill(FwdIter first, Sent last, T const& value);

    /// Copies the given \a value to an uninitialized memory area, defined by
    /// the range [first, last). If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param rng          Refers to the range to which the value
    ///                     will be filled
    /// \param value        The value to be assigned.
    ///
    /// The assignments in the parallel \a uninitialized_fill algorithm invoked
    /// without an execution policy object will execute in sequential order in
    /// the calling thread.
    ///
    /// \returns  The \a uninitialized_fill algorithm returns a
    ///           returns \a hpx::traits::range_traits<Rng>::iterator_type.
    ///           The \a uninitialized_fill algorithm returns the output
    ///           iterator to the element in the range, one past
    ///           the last element copied.
    ///
    template <typename Rng, typename T>
    typename hpx::traits::range_traits<Rng>::iterator_type uninitialized_fill(
        Rng&& rng, T const& value);

    /// Copies the given \a value value to the first count elements in an
    /// uninitialized memory area beginning at first. If an exception is thrown
    /// during the initialization, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// The assignments in the parallel \a uninitialized_fill_n algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a uninitialized_fill_n algorithm returns a
    ///           returns \a FwdIter.
    ///           The \a uninitialized_fill_n algorithm returns the output
    ///           iterator to the element in the range, one past
    ///           the last element copied.
    ///
    template <typename FwdIter, typename Size, typename T>
    FwdIter uninitialized_fill_n(FwdIter first, Size count, T const& value);

    // clang-format on

    /// \brief Execution-policy overload of \c uninitialized_fill.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// \pre Operations on destination iterators and sentinels do not throw.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename T = std::iter_value_t<I>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::is_lvalue_reference_v<std::iter_reference_t<I>> &&
        std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>,
            std::iter_value_t<I>> &&
        std::constructible_from<std::iter_value_t<I>, T const&>
    parallel::util::detail::algorithm_result_t<ExPolicy, I> uninitialized_fill(
        ExPolicy&& policy, I first, S last, T const& value);

    /// \brief Execution-policy overload of \c uninitialized_fill.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    /// \pre Operations on destination iterators and sentinels do not throw.
    template <typename ExPolicy, std::ranges::random_access_range R,
        typename T = std::iter_value_t<std::ranges::iterator_t<R>>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::is_lvalue_reference_v<
            std::iter_reference_t<std::ranges::iterator_t<R>>> &&
        std::same_as<std::remove_cvref_t<
                         std::iter_reference_t<std::ranges::iterator_t<R>>>,
            std::iter_value_t<std::ranges::iterator_t<R>>> &&
        std::constructible_from<std::iter_value_t<std::ranges::iterator_t<R>>,
            T const&>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_iterator_t<R>>
    uninitialized_fill(ExPolicy&& policy, R&& rng, T const& value);

    /// \brief Execution-policy overload of \c uninitialized_fill_n.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// \pre Operations on destination iterators and sentinels do not throw.
    template <typename ExPolicy, std::random_access_iterator I,
        typename T = std::iter_value_t<I>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::is_lvalue_reference_v<std::iter_reference_t<I>> &&
        std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>,
            std::iter_value_t<I>> &&
        std::constructible_from<std::iter_value_t<I>, T const&>
    parallel::util::detail::algorithm_result_t<ExPolicy, I>
    uninitialized_fill_n(ExPolicy&& policy, I first,
        std::iter_difference_t<I> count, T const& value);
}}    // namespace hpx::ranges
#else

#include <hpx/config.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/algorithm_value.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/uninitialized_fill.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_fill_t final
      : hpx::detail::tag_dispatch<uninitialized_fill_t,
            hpx::detail::tag_parallel_algorithm<uninitialized_fill_t>>
    {
        template <typename FwdIter, typename Sent, typename T>
        // clang-format off
            requires(
                std::forward_iterator<FwdIter> &&
                std::sentinel_for<Sent, FwdIter>
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, Sent last, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_fill<FwdIter>().call(
                hpx::execution::seq, first, last, value);
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename T = std::iter_value_t<I>>
            requires parallel::detail::algorithm_value_argument<T> &&
            hpx::is_execution_policy_v<ExPolicy> &&
            std::is_lvalue_reference_v<std::iter_reference_t<I>> &&
            std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>,
                std::iter_value_t<I>> &&
            std::constructible_from<std::iter_value_t<I>,
                std::remove_reference_t<T> const&>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, T&& value)
        {
            auto end = first + (last - first);
            return parallel::detail::uninitialized_fill<I>().call(
                HPX_FORWARD(ExPolicy, policy), first, end,
                parallel::detail::algorithm_value<std::remove_cvref_t<T>>(
                    HPX_FORWARD(T, value)));
        }

        template <typename Rng, typename T>
            requires(std::ranges::range<Rng>)
        static typename hpx::traits::range_traits<Rng>::iterator_type
        invoke_default(Rng&& rng, T const& value)
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_fill<iterator_type>()
                .call(
                    hpx::execution::seq, std::begin(rng), std::end(rng), value);
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename T = std::iter_value_t<std::ranges::iterator_t<R>>>
            requires parallel::detail::algorithm_value_argument<T> &&
            hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::is_lvalue_reference_v<
                std::iter_reference_t<std::ranges::iterator_t<R>>> &&
            std::same_as<std::remove_cvref_t<
                             std::iter_reference_t<std::ranges::iterator_t<R>>>,
                std::iter_value_t<std::ranges::iterator_t<R>>> &&
            std::constructible_from<
                std::iter_value_t<std::ranges::iterator_t<R>>,
                std::remove_reference_t<T> const&>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, T&& value)
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), HPX_FORWARD(T, value)),
                [](I result) -> std::ranges::borrowed_iterator_t<R> {
                    return result;
                });
        }

        using base_type = hpx::detail::tag_dispatch<uninitialized_fill_t,
            hpx::detail::tag_parallel_algorithm<uninitialized_fill_t>>;
        using base_type::operator();

        // Typed value parameters permit list-initialized arguments.
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename T = std::iter_value_t<I>>
            requires parallel::detail::algorithm_value_argument<T> &&
            hpx::is_execution_policy_v<ExPolicy> &&
            std::is_lvalue_reference_v<std::iter_reference_t<I>> &&
            std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>,
                std::iter_value_t<I>> &&
            std::constructible_from<std::iter_value_t<I>,
                std::remove_reference_t<T> const&>
        decltype(auto) operator()(
            ExPolicy&& policy, I first, S last, T&& value) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy), first,
                last, HPX_FORWARD(T, value));
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename T = std::iter_value_t<std::ranges::iterator_t<R>>>
            requires parallel::detail::algorithm_value_argument<T> &&
            hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::is_lvalue_reference_v<
                std::iter_reference_t<std::ranges::iterator_t<R>>> &&
            std::same_as<std::remove_cvref_t<
                             std::iter_reference_t<std::ranges::iterator_t<R>>>,
                std::iter_value_t<std::ranges::iterator_t<R>>> &&
            std::constructible_from<
                std::iter_value_t<std::ranges::iterator_t<R>>,
                std::remove_reference_t<T> const&>
        decltype(auto) operator()(ExPolicy&& policy, R&& rng, T&& value) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy),
                HPX_FORWARD(R, rng), HPX_FORWARD(T, value));
        }
    } uninitialized_fill{};

    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_fill_n_t final
      : hpx::detail::tag_dispatch<uninitialized_fill_n_t,
            hpx::detail::tag_parallel_algorithm<uninitialized_fill_n_t>>
    {
        template <typename FwdIter, typename Size, typename T>
        // clang-format off
            requires(
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, Size count, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_fill_n<FwdIter>().call(
                hpx::execution::seq, first, count, value);
        }

        template <typename ExPolicy, std::random_access_iterator I,
            typename T = std::iter_value_t<I>>
            requires parallel::detail::algorithm_value_argument<T> &&
            hpx::is_execution_policy_v<ExPolicy> &&
            std::is_lvalue_reference_v<std::iter_reference_t<I>> &&
            std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>,
                std::iter_value_t<I>> &&
            std::constructible_from<std::iter_value_t<I>,
                std::remove_reference_t<T> const&>
        static decltype(auto) invoke_default(ExPolicy&& policy, I first,
            std::iter_difference_t<I> count, T&& value)
        {
            auto const size = (std::max) (std::iter_difference_t<I>(0), count);
            return parallel::detail::uninitialized_fill_n<I>().call(
                HPX_FORWARD(ExPolicy, policy), first,
                static_cast<std::size_t>(size),
                parallel::detail::algorithm_value<std::remove_cvref_t<T>>(
                    HPX_FORWARD(T, value)));
        }

        using base_type = hpx::detail::tag_dispatch<uninitialized_fill_n_t,
            hpx::detail::tag_parallel_algorithm<uninitialized_fill_n_t>>;
        using base_type::operator();

        // Typed value parameters permit list-initialized arguments.
        template <typename ExPolicy, std::random_access_iterator I,
            typename T = std::iter_value_t<I>>
            requires parallel::detail::algorithm_value_argument<T> &&
            hpx::is_execution_policy_v<ExPolicy> &&
            std::is_lvalue_reference_v<std::iter_reference_t<I>> &&
            std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>,
                std::iter_value_t<I>> &&
            std::constructible_from<std::iter_value_t<I>,
                std::remove_reference_t<T> const&>
        decltype(auto) operator()(ExPolicy&& policy, I first,
            std::iter_difference_t<I> count, T&& value) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy), first,
                count, HPX_FORWARD(T, value));
        }
    } uninitialized_fill_n{};
}    // namespace hpx::ranges

#endif
