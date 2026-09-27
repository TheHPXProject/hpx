//  Copyright (c) 2015-2023 Hartmut Kaiser
//  Copyright (c) 2021 Akhli J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/swap_ranges.hpp
/// \page hpx::ranges::swap_ranges
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Exchanges elements between range [first1, last1) and another range
    /// starting at \a first2.
    ///
    /// \note   Complexity: Linear in the distance between \a first1 and \a last1
    ///
    /// \tparam InIter1     The type of the first range of iterators to swap
    ///                     (deduced).
    /// \tparam Sent1       The type of the first sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter1.
    /// \tparam InIter2     The type of the second range of iterators to swap
    ///                     (deduced).
    /// \tparam Sent2       The type of the second sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter2.
    ///
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     for the first range.
    /// \param last1        Refers to sentinel value denoting the end of the
    ///                     sequence of elements for the first range.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     for the second range.
    /// \param last2        Refers to sentinel value denoting the end of the
    ///                     sequence of elements for the second range.
    ///
    /// The swap operations in the parallel \a swap_ranges algorithm
    /// invoked without an execution policy object  execute in sequential
    /// order in the calling thread.
    ///
    /// \returns  The \a swap_ranges algorithm returns
    ///           \a swap_ranges_result<InIter1, InIter2>.
    ///           The \a swap_ranges algorithm returns in_in_result with the
    ///           first element as the iterator to the element past the last
    ///           element exchanged in range beginning with \a first1 and the
    ///           second element as the iterator to the element past the last
    ///           element exchanged in the range beginning with \a first2.
    ///
    template <typename InIter1, typename Sent1, typename InIter2,
        typename Sent2>
    swap_ranges_result<InIter1, InIter2>
    swap_ranges(InIter1 first1, Sent1 last1, InIter2 first2, Sent2 last2);

    ///////////////////////////////////////////////////////////////////////////
    /// Exchanges elements between range [first1, last1) and another range
    /// starting at \a first2.
    ///
    /// \note   Complexity: Linear in the distance between \a first1 and \a last1
    ///
    /// \tparam Rng1        The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Rng2        The type of the destination range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    ///
    /// \param rng1         Refers to the sequence of elements of the first
    ///                     range.
    /// \param rng2         Refers to the sequence of elements of the second
    ///                     range.
    ///
    /// The swap operations in the parallel \a swap_ranges algorithm
    /// invoked without an execution policy object  execute in sequential
    /// order in the calling thread.
    ///
    /// \returns  The \a swap_ranges algorithm returns
    ///           \a swap_ranges_result<
    ///           std::ranges::iterator_t<Rng1>,
    ///           std::ranges::iterator_t<Rng1>>.
    ///           The \a swap_ranges algorithm returns in_in_result with the
    ///           first element as the iterator to the element past the last
    ///           element exchanged in range beginning with \a first1 and the
    ///           second element as the iterator to the element past the last
    ///           element exchanged in the range beginning with \a first2.
    ///
    template <typename Rng1, typename Rng2>
    swap_ranges_result<std::ranges::iterator_t<Rng1>,
        std::ranges::iterator_t<Rng2>>
    swap_ranges(Rng1&& rng1, Rng2&& rng2);

    // clang-format on

    /// \brief Execution-policy overload of \c swap_ranges.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I1,
        std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
        std::sized_sentinel_for<I2> S2>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_swappable<I1, I2>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        swap_ranges_result<I1, I2>>
    swap_ranges(ExPolicy&& policy, I1 first1, S1 last1, I2 first2, S2 last2);

    /// \brief Execution-policy overload of \c swap_ranges.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R1,
        std::ranges::random_access_range R2>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R1> && std::ranges::sized_range<R2> &&
        std::indirectly_swappable<std::ranges::iterator_t<R1>,
            std::ranges::iterator_t<R2>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        swap_ranges_result<std::ranges::borrowed_iterator_t<R1>,
            std::ranges::borrowed_iterator_t<R2>>>
    swap_ranges(ExPolicy&& policy, R1&& rng1, R2&& rng2);
}}    // namespace hpx::ranges

#else

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/swap_ranges.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
    using swap_ranges_result = hpx::parallel::util::in_in_result<Iter1, Iter2>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::swap_ranges
    HPX_CXX_CORE_EXPORT inline constexpr struct swap_ranges_t final
      : hpx::detail::tag_dispatch<swap_ranges_t,
            hpx::detail::tag_parallel_algorithm<swap_ranges_t>>
    {
        template <typename InIter1, typename Sent1, typename InIter2,
            typename Sent2>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<InIter1> &&
                std::sentinel_for<Sent1, InIter1> &&
                hpx::traits::is_iterator_v<InIter2> &&
                std::sentinel_for<Sent2, InIter2>
            )
        // clang-format on
        static swap_ranges_result<InIter1, InIter2> invoke_default(
            InIter1 first1, Sent1 last1, InIter2 first2, Sent2 last2)
        {
            static_assert(std::input_iterator<InIter1>,
                "Requires at least input iterator.");
            static_assert(std::input_iterator<InIter2>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::swap_ranges<
                swap_ranges_result<InIter1, InIter2>>()
                .call(hpx::execution::seq, first1, last1, first2, last2);
        }

        template <typename ExPolicy, std::random_access_iterator I1,
            std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_swappable<I1, I2>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I1 first1, S1 last1, I2 first2, S2 last2)
        {
            return parallel::detail::swap_ranges<swap_ranges_result<I1, I2>>()
                .call(HPX_FORWARD(ExPolicy, policy), first1,
                    first1 + (last1 - first1), first2,
                    first2 + (last2 - first2));
        }

        template <typename Rng1, typename Rng2>
        // clang-format off
            requires(
                std::ranges::range<Rng1> &&
                std::ranges::range<Rng2>
            )
        // clang-format on
        static swap_ranges_result<std::ranges::iterator_t<Rng1>,
            std::ranges::iterator_t<Rng2>>
        invoke_default(Rng1&& rng1, Rng2&& rng2)
        {
            using iterator_type1 = std::ranges::iterator_t<Rng1>;
            using iterator_type2 = std::ranges::iterator_t<Rng2>;

            static_assert(std::input_iterator<iterator_type1>,
                "Requires at least input iterator.");
            static_assert(std::input_iterator<iterator_type2>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::swap_ranges<
                swap_ranges_result<iterator_type1, iterator_type2>>()
                .call(hpx::execution::seq, std::begin(rng1), std::end(rng1),
                    std::begin(rng2), std::end(rng2));
        }

        template <typename ExPolicy, std::ranges::random_access_range R1,
            std::ranges::random_access_range R2>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R1> && std::ranges::sized_range<R2> &&
            std::indirectly_swappable<std::ranges::iterator_t<R1>,
                std::ranges::iterator_t<R2>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R1&& rng1, R2&& rng2)
        {
            using I1 = std::ranges::iterator_t<R1>;
            using I2 = std::ranges::iterator_t<R2>;
            auto first1 = std::ranges::begin(rng1);
            auto first2 = std::ranges::begin(rng2);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first1,
                    first1 + std::ranges::distance(rng1), first2,
                    first2 + std::ranges::distance(rng2)),
                [](swap_ranges_result<I1, I2> result)
                    -> swap_ranges_result<std::ranges::borrowed_iterator_t<R1>,
                        std::ranges::borrowed_iterator_t<R2>> {
                    return {result.in1, result.in2};
                });
        }
    } swap_ranges{};
}    // namespace hpx::ranges

#endif
