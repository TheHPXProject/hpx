//  Copyright (c) 2020-2023 Hartmut Kaiser
//  Copyright (c) 2022 Dimitra Karatza
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/destroy.hpp
/// \page hpx::ranges::destroy, hpx::ranges::destroy_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
    // clang-format off

    /// Destroys objects of type typename iterator_traits<ForwardIt>::value_type
    /// in the range [first, last).
    ///
    /// \note   Complexity: Performs exactly \a last - \a first operations.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    ///
    /// \returns  The \a destroy algorithm returns \a void.
    ///
    template <typename Rng> hpx::traits::range_iterator<Rng>::type destroy(Rng&& rng);

    /// Destroys objects of type typename iterator_traits<ForwardIt>::value_type
    /// in the range [first, last).
    ///
    /// \note   Complexity: Performs exactly \a last - \a first operations.
    ///
    /// \tparam Iter        The type of the source iterators used for the
    ///                     range (deduced).
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements
    ///                     the algorithm will be applied to.
    ///
    /// \returns  The \a destroy algorithm returns \a void.
    ///
    template <typename Iter, typename Sent> Iter destroy(Iter first, Sent last);

    /// Destroys objects of type typename iterator_traits<ForwardIt>::value_type
    /// in the range [first, first + count).
    ///
    /// \note   Complexity: Performs exactly \a count operations, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply this algorithm to.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    ///
    /// \returns  The \a destroy_n algorithm returns the
    ///           iterator to the element in the source range, one past
    ///           the last element constructed.
    ///
    template <typename FwdIter, typename Size>
    FwdIter destroy_n(FwdIter first, Size count);

    // clang-format on

    /// \brief Execution-policy overload of \c destroy.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    /// \pre Operations on destination iterators and sentinels do not throw.
    template <typename ExPolicy, std::ranges::random_access_range R>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::is_lvalue_reference_v<
            std::iter_reference_t<std::ranges::iterator_t<R>>> &&
        std::same_as<std::remove_cvref_t<
                         std::iter_reference_t<std::ranges::iterator_t<R>>>,
            std::iter_value_t<std::ranges::iterator_t<R>>> &&
        std::destructible<std::iter_value_t<std::ranges::iterator_t<R>>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_iterator_t<R>>
    destroy(ExPolicy&& policy, R&& rng);

    /// \brief Execution-policy overload of \c destroy.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// \pre Operations on destination iterators and sentinels do not throw.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::is_lvalue_reference_v<std::iter_reference_t<I>> &&
        std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>,
            std::iter_value_t<I>> &&
        std::destructible<std::iter_value_t<I>>
    parallel::util::detail::algorithm_result_t<ExPolicy, I> destroy(
        ExPolicy&& policy, I first, S last);

    /// \brief Execution-policy overload of \c destroy_n.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// \pre Operations on destination iterators and sentinels do not throw.
    template <typename ExPolicy, std::random_access_iterator I>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::is_lvalue_reference_v<std::iter_reference_t<I>> &&
        std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>,
            std::iter_value_t<I>> &&
        std::destructible<std::iter_value_t<I>>
    parallel::util::detail::algorithm_result_t<ExPolicy, I> destroy_n(
        ExPolicy&& policy, I first, std::iter_difference_t<I> count);
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/destroy.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::destroy
    HPX_CXX_CORE_EXPORT inline constexpr struct destroy_t final
      : hpx::detail::tag_dispatch<destroy_t,
            hpx::detail::tag_parallel_algorithm<destroy_t>>
    {
        template <typename ExPolicy, std::ranges::random_access_range R>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::is_lvalue_reference_v<
                std::iter_reference_t<std::ranges::iterator_t<R>>> &&
            std::same_as<std::remove_cvref_t<
                             std::iter_reference_t<std::ranges::iterator_t<R>>>,
                std::iter_value_t<std::ranges::iterator_t<R>>> &&
            std::destructible<std::iter_value_t<std::ranges::iterator_t<R>>>
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

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::is_lvalue_reference_v<std::iter_reference_t<I>> &&
            std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>,
                std::iter_value_t<I>> &&
            std::destructible<std::iter_value_t<I>>
        static decltype(auto) invoke_default(ExPolicy&& policy, I first, S last)
        {
            auto end = first + (last - first);
            return parallel::detail::destroy<I>().call(
                HPX_FORWARD(ExPolicy, policy), first, end);
        }

        template <typename Rng>
            requires(std::ranges::range<Rng>)
        static std::ranges::iterator_t<Rng> invoke_default(Rng&& rng)
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::forward_iterator<iterator_type>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::destroy<iterator_type>().call(
                hpx::execution::seq, hpx::util::begin(rng),
                hpx::util::end(rng));
        }

        template <typename Iter, typename Sent>
            requires(hpx::traits::is_iterator_v<Iter>)
        static Iter invoke_default(Iter first, Sent last)
        {
            static_assert(std::forward_iterator<Iter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::destroy<Iter>().call(
                hpx::execution::seq, first, last);
        }
    } destroy{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::destroy_n
    HPX_CXX_CORE_EXPORT inline constexpr struct destroy_n_t final
      : hpx::detail::tag_dispatch<destroy_n_t,
            hpx::detail::tag_parallel_algorithm<destroy_n_t>>
    {
        template <typename ExPolicy, std::random_access_iterator I>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::is_lvalue_reference_v<std::iter_reference_t<I>> &&
            std::same_as<std::remove_cvref_t<std::iter_reference_t<I>>,
                std::iter_value_t<I>> &&
            std::destructible<std::iter_value_t<I>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, std::iter_difference_t<I> count)
        {
            auto const size = (std::max) (std::iter_difference_t<I>(0), count);
            return parallel::detail::destroy_n<I>().call(
                HPX_FORWARD(ExPolicy, policy), first,
                static_cast<std::size_t>(size));
        }

        template <typename FwdIter, typename Size>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, Size count)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                return first;
            }

            return hpx::parallel::detail::destroy_n<FwdIter>().call(
                hpx::execution::seq, first, static_cast<std::size_t>(count));
        }
    } destroy_n{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
