//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/uninitialized_move.hpp
/// \page hpx::ranges::uninitialized_move, hpx::ranges::uninitialized_move_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    /// Moves the elements in the range, defined by [first, last), to an
    /// uninitialized memory area beginning at \a dest. If an exception is
    /// thrown during the initialization, some objects in [first, last) are
    /// left in a valid but unspecified state.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Sent1       The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent2       The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter2.
    ///
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     that will be moved from
    /// \param last1        Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied
    /// \param first2       Refers to the beginning of the destination range.
    /// \param last2        Refers to sentinel value denoting the end of the
    ///                     second range the algorithm will be applied to.
    ///
    /// The assignments in the parallel \a uninitialized_move algorithm invoked
    /// without an execution policy object will execute in sequential order in
    /// the calling thread.
    ///
    /// \returns  The \a uninitialized_move algorithm returns an
    ///           \a in_out_result<InIter, FwdIter>.
    ///           The \a uninitialized_move algorithm returns an input iterator
    ///           to one past the last element moved from and the output
    ///           iterator to the element in the destination range, one past
    ///           the last element moved.
    ///
    template <typename InIter, typename Sent1, typename FwdIter, typename Sent2>
    hpx::parallel::util::in_out_result<InIter, FwdIter> uninitialized_move(
        InIter first1, Sent1 last1, FwdIter first2, Sent2 last2);

    /// Moves the elements in the range, defined by [first, last), to an
    /// uninitialized memory area beginning at \a dest. If an exception is
    /// thrown during the initialization, some objects in [first, last) are
    /// left in a valid but unspecified state.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam Rng1        The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Rng2        The type of the destination range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    ///
    /// \param rng1         Refers to the range from which the elements
    ///                     will be moved from
    /// \param rng2         Refers to the range to which the elements
    ///                     will be moved to
    ///
    /// The assignments in the parallel \a uninitialized_move algorithm invoked
    /// without an execution policy object will execute in sequential order in
    /// the calling thread.
    ///
    /// \returns  The \a uninitialized_move algorithm returns an \a
    ///           in_out_result<typename hpx::traits::range_traits<Rng1>::iterator_type,
    ///           typename hpx::traits::range_traits<Rng2>::iterator_type>.
    ///           The \a uninitialized_move algorithm returns an input iterator
    ///           to one past the last element moved from and the output
    ///           iterator to the element in the destination range, one past
    ///           the last element moved.
    ///
    template <typename Rng1, typename Rng2>
    hpx::parallel::util::in_out_result<
        typename hpx::traits::range_traits<Rng1>::iterator_type,
        typename hpx::traits::range_traits<Rng2>::iterator_type>
    uninitialized_move(Rng1&& rng1, Rng2&& rng2);

    /// Moves the elements in the range [first, first + count), starting from
    /// first and proceeding to first + count - 1., to another range beginning
    /// at dest. If an exception is
    /// thrown during the initialization, some objects in [first, first + count)
    /// are left in a valid but unspecified state.
    ///
    /// \note   Complexity: Performs exactly \a count movements, if
    ///         count > 0, no move operations otherwise.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent2       The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    ///
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     that will be moved from
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the destination range.
    /// \param last2        Refers to sentinel value denoting the end of the
    ///                     second range the algorithm will be applied to.
    ///
    /// The assignments in the parallel \a uninitialized_move_n algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a uninitialized_move_n algorithm returns
    ///           \a in_out_result<InIter, FwdIter>.
    ///           The \a uninitialized_move_n algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element moved.
    ///
    template <typename InIter, typename Size, typename FwdIter, typename Sent2>
    hpx::parallel::util::in_out_result<InIter, FwdIter> uninitialized_move_n(
        InIter first1, Size count, FwdIter first2, Sent2 last2);

    // clang-format on

    /// \brief Execution-policy overload of \c uninitialized_move.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// The operation is bounded by the supplied output range(s). Returned
    /// input positions identify where processing can resume.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// \pre Operations on destination iterators and sentinels do not throw.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, std::random_access_iterator O,
        std::sized_sentinel_for<O> OutS>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::is_lvalue_reference_v<std::iter_reference_t<O>> &&
        std::same_as<std::remove_cvref_t<std::iter_reference_t<O>>,
            std::iter_value_t<O>> &&
        std::constructible_from<std::iter_value_t<O>,
            std::iter_rvalue_reference_t<I>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        uninitialized_move_result<I, O>>
    uninitialized_move(
        ExPolicy&& policy, I first, S last, O dest, OutS dest_last);

    /// \brief Execution-policy overload of \c uninitialized_move.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// The operation is bounded by the supplied output range(s). Returned
    /// input positions identify where processing can resume.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    /// \pre Operations on destination iterators and sentinels do not throw.
    template <typename ExPolicy, std::ranges::random_access_range R,
        std::ranges::random_access_range OutR>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> && std::ranges::sized_range<OutR> &&
        std::is_lvalue_reference_v<
            std::iter_reference_t<std::ranges::iterator_t<OutR>>> &&
        std::same_as<std::remove_cvref_t<
                         std::iter_reference_t<std::ranges::iterator_t<OutR>>>,
            std::iter_value_t<std::ranges::iterator_t<OutR>>> &&
        std::constructible_from<
            std::iter_value_t<std::ranges::iterator_t<OutR>>,
            std::iter_rvalue_reference_t<std::ranges::iterator_t<R>>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        uninitialized_move_result<std::ranges::borrowed_iterator_t<R>,
            std::ranges::borrowed_iterator_t<OutR>>>
    uninitialized_move(ExPolicy&& policy, R&& rng, OutR&& output);

    /// \brief Execution-policy overload of \c uninitialized_move_n.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// The operation is bounded by the supplied output range(s). Returned
    /// input positions identify where processing can resume.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// \pre Operations on destination iterators and sentinels do not throw.
    template <typename ExPolicy, std::random_access_iterator I,
        std::random_access_iterator O, std::sized_sentinel_for<O> OutS>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::is_lvalue_reference_v<std::iter_reference_t<O>> &&
        std::same_as<std::remove_cvref_t<std::iter_reference_t<O>>,
            std::iter_value_t<O>> &&
        std::constructible_from<std::iter_value_t<O>,
            std::iter_rvalue_reference_t<I>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        uninitialized_move_n_result<I, O>>
    uninitialized_move_n(ExPolicy&& policy, I first,
        std::iter_difference_t<I> count, O dest, OutS dest_last);
}}    // namespace hpx::ranges
#else

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/uninitialized_move.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {
    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using uninitialized_move_result = parallel::util::in_out_result<I, O>;

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using uninitialized_move_n_result = parallel::util::in_out_result<I, O>;

    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_move_t final
      : hpx::detail::tag_dispatch<uninitialized_move_t,
            hpx::detail::tag_parallel_algorithm<uninitialized_move_t>>
    {
        template <typename InIter, typename Sent1, typename FwdIter,
            typename Sent2>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<InIter> &&
                std::sentinel_for<Sent1, InIter> &&
                std::forward_iterator<FwdIter> &&
                std::sentinel_for<Sent2, FwdIter>
            )
        // clang-format on
        static hpx::parallel::util::in_out_result<InIter, FwdIter>
        invoke_default(InIter first1, Sent1 last1, FwdIter first2, Sent2 last2)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_move_sent<
                parallel::util::in_out_result<InIter, FwdIter>>()
                .call(hpx::execution::seq, first1, last1, first2, last2);
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::is_lvalue_reference_v<std::iter_reference_t<O>> &&
            std::same_as<std::remove_cvref_t<std::iter_reference_t<O>>,
                std::iter_value_t<O>> &&
            std::constructible_from<std::iter_value_t<O>,
                std::iter_rvalue_reference_t<I>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, O dest, OutS dest_last)
        {
            return parallel::detail::uninitialized_move_sent<
                uninitialized_move_result<I, O>>()
                .call(HPX_FORWARD(ExPolicy, policy), first,
                    first + (last - first), dest, dest + (dest_last - dest));
        }

        template <typename Rng1, typename Rng2>
        // clang-format off
            requires(
                std::ranges::range<Rng1> &&
                std::ranges::range<Rng2>
            )
        // clang-format on
        static hpx::parallel::util::in_out_result<
            typename hpx::traits::range_traits<Rng1>::iterator_type,
            typename hpx::traits::range_traits<Rng2>::iterator_type>
        invoke_default(Rng1&& rng1, Rng2&& rng2)
        {
            using iterator_type1 =
                typename hpx::traits::range_traits<Rng1>::iterator_type;
            using iterator_type2 =
                typename hpx::traits::range_traits<Rng2>::iterator_type;

            static_assert(std::input_iterator<iterator_type1>,
                "Requires at least input iterator.");

            static_assert(std::forward_iterator<iterator_type2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::uninitialized_move_sent<
                parallel::util::in_out_result<iterator_type1, iterator_type2>>()
                .call(hpx::execution::seq, std::begin(rng1), std::end(rng1),
                    std::begin(rng2), std::end(rng2));
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            std::ranges::random_access_range OutR>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> && std::ranges::sized_range<OutR> &&
            std::is_lvalue_reference_v<
                std::iter_reference_t<std::ranges::iterator_t<OutR>>> &&
            std::same_as<std::remove_cvref_t<std::iter_reference_t<
                             std::ranges::iterator_t<OutR>>>,
                std::iter_value_t<std::ranges::iterator_t<OutR>>> &&
            std::constructible_from<
                std::iter_value_t<std::ranges::iterator_t<OutR>>,
                std::iter_rvalue_reference_t<std::ranges::iterator_t<R>>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, OutR&& output)
        {
            using iterator_result =
                uninitialized_move_result<std::ranges::iterator_t<R>,
                    std::ranges::iterator_t<OutR>>;
            using result_type =
                uninitialized_move_result<std::ranges::borrowed_iterator_t<R>,
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
    } uninitialized_move{};

    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_move_n_t final
      : hpx::detail::tag_dispatch<uninitialized_move_n_t,
            hpx::detail::tag_parallel_algorithm<uninitialized_move_n_t>>
    {
        template <typename InIter, typename Size, typename FwdIter,
            typename Sent2>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<InIter> &&
                std::forward_iterator<FwdIter> &&
                std::sentinel_for<Sent2, FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static hpx::parallel::util::in_out_result<InIter, FwdIter>
        invoke_default(InIter first1, Size count, FwdIter first2, Sent2 last2)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            std::size_t d = parallel::detail::distance(first2, last2);
            return hpx::parallel::detail::uninitialized_move_n<
                parallel::util::in_out_result<InIter, FwdIter>>()
                .call(hpx::execution::seq, first1, count <= d ? count : d,
                    first2);
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::random_access_iterator O, std::sized_sentinel_for<O> OutS>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::is_lvalue_reference_v<std::iter_reference_t<O>> &&
            std::same_as<std::remove_cvref_t<std::iter_reference_t<O>>,
                std::iter_value_t<O>> &&
            std::constructible_from<std::iter_value_t<O>,
                std::iter_rvalue_reference_t<I>>
        static decltype(auto) invoke_default(ExPolicy&& policy, I first,
            std::iter_difference_t<I> count, O dest, OutS dest_last)
        {
            using difference_type =
                std::common_type_t<std::iter_difference_t<I>,
                    std::iter_difference_t<O>>;
            auto const size =
                (std::min) (difference_type(
                                (std::max) (std::iter_difference_t<I>(0),
                                    count)),
                    difference_type(dest_last - dest));
            return parallel::detail::uninitialized_move_n<
                uninitialized_move_result<I, O>>()
                .call(HPX_FORWARD(ExPolicy, policy), first,
                    static_cast<std::size_t>(size), dest);
        }
    } uninitialized_move_n{};
}    // namespace hpx::ranges

#endif
