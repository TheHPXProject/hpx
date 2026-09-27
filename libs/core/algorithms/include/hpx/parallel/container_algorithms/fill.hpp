//  Copyright (c) 2018 Christopher Ogle
//  Copyright (c) 2020-2023 Hartmut Kaiser
//  Copyright (c) 2022 Dimitra Karatza
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/fill.hpp
/// \page hpx::ranges::fill, hpx::ranges::fill_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
    // clang-format off

    /// Assigns the given value to the elements in the range [first, last).
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// \returns  The \a fill algorithm returns \a void.
    ///
    template <typename Rng, typename T = typename std::iterator_traits<
        std::ranges::iterator_t<Rng>>::value_type>
    std::ranges::iterator_t<Rng> fill(Rng&& rng, T const& value);

    /// Assigns the given value to the elements in the range [first, last).
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam Iter        The type of the source iterators used for the
    ///                     range (deduced).
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// \returns  The \a fill algorithm returns \a void.
    ///
    template <typename Iter, typename Sent,
        typename T = typename std::iterator_traits<Iter>::value_type>
    Iter fill(Iter first, Sent last, T const& value);

    /// Assigns the given value value to the first count elements in the range
    /// beginning at first if count > 0. Does nothing otherwise.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// The comparisons in the parallel \a fill_n algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparisons in the parallel \a fill_n algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a fill_n algorithm returns a \a hpx::future<void> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a difference_type otherwise (where \a difference_type
    ///           is defined by \a void.
    ///
    template <typename ExPolicy, typename Rng,
        typename T = typename std::iterator_traits<
            std::ranges::iterator_t<Rng>>::value_type>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::iterator_t<Rng>>
    fill_n(ExPolicy&& policy, Rng&& rng, T const& value);

    /// Assigns the given value value to the first count elements in the range
    /// beginning at first if count > 0. Does nothing otherwise.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, for
    ///         count > 0.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// The comparisons in the parallel \a fill_n algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparisons in the parallel \a fill_n algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a fill_n algorithm returns a \a hpx::future<void> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a difference_type otherwise (where \a difference_type
    ///           is defined by \a void.
    ///
    template <typename ExPolicy, typename FwdIter, typename Size,
        typename T = typename std::iterator_traits<FwdIter>::value_type>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        FwdIter>::type
    fill_n(ExPolicy&& policy, FwdIter first, Size count, T const& value);

    /// Assigns the given value value to the first count elements in the range
    /// beginning at first if count > 0. Does nothing otherwise.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// \returns  The \a fill_n algorithm returns an output iterator that
    ///           compares equal to last.
    ///
    template <typename Rng,
        typename T = typename std::iterator_traits<
            std::ranges::iterator_t<Rng>>::value_type>
    typename hpx::traits::range_traits<Rng>::iterator_type
    fill_n(Rng&& rng, T const& value);

    /// Assigns the given value value to the first count elements in the range
    /// beginning at first if count > 0. Does nothing otherwise.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, for
    ///         count > 0.
    ///
    /// \tparam Iterator    The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
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
    /// \returns  The \a fill_n algorithm returns an output iterator that
    ///           compares equal to last.
    ///
    template <typename FwdIter, typename Size,
        typename T = typename std::iterator_traits<FwdIter>::value_type>
    FwdIter fill_n(Iterator first, Size count, T const& value);

    // clang-format on

    /// \brief Execution-policy overload of \c fill.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        typename T = std::iter_value_t<std::ranges::iterator_t<R>>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::indirectly_writable<std::ranges::iterator_t<R>, T const&>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_iterator_t<R>>
    fill(ExPolicy&& policy, R&& rng, T const& value);

    /// \brief Execution-policy overload of \c fill.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename T = std::iter_value_t<I>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_writable<I, T const&>
    parallel::util::detail::algorithm_result_t<ExPolicy, I> fill(
        ExPolicy&& policy, I first, S last, T const& value);

    /// \brief Execution-policy overload of \c fill_n.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        typename T = std::iter_value_t<I>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_writable<I, T const&>
    parallel::util::detail::algorithm_result_t<ExPolicy, I> fill_n(
        ExPolicy&& policy, I first, std::iter_difference_t<I> count,
        T const& value);
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/algorithm_value.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/fill.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::fill
    HPX_CXX_CORE_EXPORT inline constexpr struct fill_t final
      : hpx::detail::tag_dispatch<fill_t,
            hpx::detail::tag_parallel_algorithm<fill_t>>
    {
        template <typename ExPolicy, std::ranges::random_access_range R,
            typename T = std::iter_value_t<std::ranges::iterator_t<R>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::indirectly_writable<std::ranges::iterator_t<R>, T const&>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, T const& value)
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), value),
                [](I result) -> std::ranges::borrowed_iterator_t<R> {
                    return result;
                });
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename T = std::iter_value_t<I>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_writable<I, T const&>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, T const& value)
        {
            auto end = first + (last - first);
            return parallel::detail::fill<I>().call(
                HPX_FORWARD(ExPolicy, policy), first, end,
                parallel::detail::algorithm_value<T>(value));
        }

        template <typename Rng,
            typename T = typename std::iterator_traits<
                std::ranges::iterator_t<Rng>>::value_type>
            requires(std::ranges::range<Rng>)
        static std::ranges::iterator_t<Rng> invoke_default(
            Rng&& rng, T const& value)
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::fill<iterator_type>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                value);
        }

        template <typename Iter, typename Sent,
            typename T = typename std::iterator_traits<Iter>::value_type>
            requires(std::sentinel_for<Sent, Iter>)
        static Iter invoke_default(Iter first, Sent last, T const& value)
        {
            static_assert(std::forward_iterator<Iter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::fill<Iter>().call(
                hpx::execution::seq, first, last, value);
        }

        using base_type = hpx::detail::tag_dispatch<fill_t,
            hpx::detail::tag_parallel_algorithm<fill_t>>;
        using base_type::operator();

        // Typed value parameters permit list-initialized arguments.
        template <typename ExPolicy, std::ranges::random_access_range R,
            typename T = std::iter_value_t<std::ranges::iterator_t<R>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::indirectly_writable<std::ranges::iterator_t<R>, T const&>
        decltype(auto) operator()(
            ExPolicy&& policy, R&& rng, T const& value) const
        {
            return base_type::operator()(
                HPX_FORWARD(ExPolicy, policy), HPX_FORWARD(R, rng), value);
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename T = std::iter_value_t<I>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_writable<I, T const&>
        decltype(auto) operator()(
            ExPolicy&& policy, I first, S last, T const& value) const
        {
            return base_type::operator()(
                HPX_FORWARD(ExPolicy, policy), first, last, value);
        }
    } fill{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::fill_n
    HPX_CXX_CORE_EXPORT inline constexpr struct fill_n_t final
      : hpx::detail::tag_dispatch<fill_n_t,
            hpx::detail::tag_parallel_algorithm<fill_n_t>>
    {
        template <typename ExPolicy, typename Rng,
            typename T = typename std::iterator_traits<
                std::ranges::iterator_t<Rng>>::value_type>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng>
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            std::ranges::iterator_t<Rng>>
        invoke_default(ExPolicy&& policy, Rng&& rng, T const& value)
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(hpx::util::size(rng)))
            {
                auto first = hpx::util::begin(rng);
                return hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    iterator_type>::get(HPX_MOVE(first));
            }

            return hpx::parallel::detail::fill_n<iterator_type>().call(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::size(rng), value);
        }

        template <typename ExPolicy, std::random_access_iterator I,
            typename T = std::iter_value_t<I>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_writable<I, T const&>
        static decltype(auto) invoke_default(ExPolicy&& policy, I first,
            std::iter_difference_t<I> count, T const& value)
        {
            auto const size = (std::max) (std::iter_difference_t<I>(0), count);
            return parallel::detail::fill_n<I>().call(
                HPX_FORWARD(ExPolicy, policy), first,
                static_cast<std::size_t>(size),
                parallel::detail::algorithm_value<T>(value));
        }

        template <typename Rng,
            typename T = typename std::iterator_traits<
                std::ranges::iterator_t<Rng>>::value_type>
            requires(std::ranges::range<Rng>)
        static typename hpx::traits::range_traits<Rng>::iterator_type
        invoke_default(Rng&& rng, T const& value)
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(hpx::util::size(rng)))
            {
                return hpx::util::begin(rng);
            }

            return hpx::parallel::detail::fill_n<iterator_type>().call(
                hpx::execution::seq, hpx::util::begin(rng),
                hpx::util::size(rng), value);
        }

        template <typename FwdIter, typename Size,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
            requires(
                hpx::traits::is_iterator_v<FwdIter> && std::is_integral_v<Size>)
        static FwdIter invoke_default(FwdIter first, Size count, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                return first;
            }

            return hpx::parallel::detail::fill_n<FwdIter>().call(
                hpx::execution::seq, first, static_cast<std::size_t>(count),
                value);
        }

        using base_type = hpx::detail::tag_dispatch<fill_n_t,
            hpx::detail::tag_parallel_algorithm<fill_n_t>>;
        using base_type::operator();

        // Typed value parameters permit list-initialized arguments.
        template <typename ExPolicy, std::random_access_iterator I,
            typename T = std::iter_value_t<I>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_writable<I, T const&>
        decltype(auto) operator()(ExPolicy&& policy, I first,
            std::iter_difference_t<I> count, T const& value) const
        {
            return base_type::operator()(
                HPX_FORWARD(ExPolicy, policy), first, count, value);
        }
    } fill_n{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
