//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Mamidi Surya Teja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/fold.hpp
/// \page hpx::ranges::fold_left, hpx::ranges::fold_left_first, hpx::ranges::fold_right, hpx::ranges::fold_right_last, hpx::ranges::fold_left_with_iter, hpx::ranges::fold_left_first_with_iter
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx::ranges {
    // clang-format off

    /// Left-folds a range of elements using a binary operator.
    ///
    /// \note   Complexity: Exactly N applications of the binary operator,
    ///                     where N = std::distance(first, last).
    ///
    /// \tparam InIter      The type of the source begin iterator (deduced).
    ///                     This iterator must meet the requirements of an
    ///                     input iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam T           The type of the initial value.
    /// \tparam F           The type of the binary function object.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param init         The initial value for the fold operation.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            The result of left-folding the range.
    ///
    template <typename InIter, typename Sent, typename T, typename F>
    auto fold_left(InIter first, Sent last, T init, F f);

    /// Left-folds a range of elements using a binary operator.
    ///
    /// \note   Complexity: Exactly N applications of the binary operator,
    ///                     where N = std::distance(begin(rng), end(rng)).
    ///
    /// \tparam Rng         The type of the source range (deduced).
    /// \tparam T           The type of the initial value.
    /// \tparam F           The type of the binary function object.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param init         The initial value for the fold operation.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            The result of left-folding the range.
    ///
    template <typename Rng, typename T, typename F>
    auto fold_left(Rng&& rng, T init, F f);

    /// Left-folds a range using the first element as the initial value.
    ///
    /// \note   Complexity: Exactly N-1 applications of the binary operator,
    ///                     where N = std::distance(first, last).
    ///
    /// \tparam InIter      The type of the source begin iterator (deduced).
    /// \tparam Sent        The type of the source sentinel (deduced).
    /// \tparam F           The type of the binary function object.
    ///
    /// \param first        Refers to the beginning of the sequence.
    /// \param last         Refers to the end of the sequence.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            An optional containing the fold result, or
    ///                     std::nullopt if the range is empty.
    ///
    template <typename InIter, typename Sent, typename F>
    auto fold_left_first(InIter first, Sent last, F f);

    /// Left-folds a range using the first element as the initial value.
    ///
    /// \tparam Rng         The type of the source range (deduced).
    /// \tparam F           The type of the binary function object.
    ///
    /// \param rng          Refers to the sequence of elements.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            An optional containing the fold result, or
    ///                     std::nullopt if the range is empty.
    ///
    template <typename Rng, typename F>
    auto fold_left_first(Rng&& rng, F f);

    /// Right-folds a range of elements using a binary operator.
    ///
    /// \note   Complexity: Exactly N applications of the binary operator.
    ///
    /// \tparam BidIter     The type of the source begin iterator (deduced).
    ///                     This iterator must meet the requirements of a
    ///                     bidirectional iterator.
    /// \tparam Sent        The type of the source sentinel (deduced).
    /// \tparam T           The type of the initial value.
    /// \tparam F           The type of the binary function object.
    ///
    /// \param first        Refers to the beginning of the sequence.
    /// \param last         Refers to the end of the sequence.
    /// \param init         The initial value for the fold operation.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            The result of right-folding the range.
    ///
    template <typename BidIter, typename Sent, typename T, typename F>
    auto fold_right(BidIter first, Sent last, T init, F f);

    /// Right-folds a range of elements using a binary operator.
    ///
    /// \tparam Rng         The type of the source range (deduced).
    /// \tparam T           The type of the initial value.
    /// \tparam F           The type of the binary function object.
    ///
    /// \param rng          Refers to the sequence of elements.
    /// \param init         The initial value for the fold operation.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            The result of right-folding the range.
    ///
    template <typename Rng, typename T, typename F>
    auto fold_right(Rng&& rng, T init, F f);

    /// Right-folds a range using the last element as the initial value.
    ///
    /// \tparam BidIter     The type of the source begin iterator (deduced).
    /// \tparam Sent        The type of the source sentinel (deduced).
    /// \tparam F           The type of the binary function object.
    ///
    /// \param first        Refers to the beginning of the sequence.
    /// \param last         Refers to the end of the sequence.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            An optional containing the fold result, or
    ///                     std::nullopt if the range is empty.
    ///
    template <typename BidIter, typename Sent, typename F>
    auto fold_right_last(BidIter first, Sent last, F f);

    /// Right-folds a range using the last element as the initial value.
    ///
    /// \tparam Rng         The type of the source range (deduced).
    /// \tparam F           The type of the binary function object.
    ///
    /// \param rng          Refers to the sequence of elements.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            An optional containing the fold result, or
    ///                     std::nullopt if the range is empty.
    ///
    template <typename Rng, typename F>
    auto fold_right_last(Rng&& rng, F f);

    /// Left-folds a range and returns both the result and the end iterator.
    ///
    /// \tparam InIter      The type of the source begin iterator (deduced).
    /// \tparam Sent        The type of the source sentinel (deduced).
    /// \tparam T           The type of the initial value.
    /// \tparam F           The type of the binary function object.
    ///
    /// \param first        Refers to the beginning of the sequence.
    /// \param last         Refers to the end of the sequence.
    /// \param init         The initial value for the fold operation.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            A fold_left_with_iter_result containing the end
    ///                     iterator and the fold result.
    ///
    template <typename InIter, typename Sent, typename T, typename F>
    auto fold_left_with_iter(InIter first, Sent last, T init, F f)
        -> fold_left_with_iter_result<InIter,
            std::decay_t<hpx::util::invoke_result_t<F&, T,
                hpx::traits::iter_reference_t<InIter>>>>;

    /// Left-folds a range and returns both the result and the end iterator.
    ///
    /// \tparam Rng         The type of the source range (deduced).
    /// \tparam T           The type of the initial value.
    /// \tparam F           The type of the binary function object.
    ///
    /// \param rng          Refers to the sequence of elements.
    /// \param init         The initial value for the fold operation.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            A fold_left_with_iter_result containing the end
    ///                     iterator and the fold result.
    ///
    template <typename Rng, typename T, typename F>
    auto fold_left_with_iter(Rng&& rng, T init, F f)
        -> fold_left_with_iter_result<
            std::ranges::iterator_t<Rng>,
            std::decay_t<hpx::util::invoke_result_t<F&, T,
                hpx::traits::range_reference_t<Rng>>>>;

    /// Left-folds a range using the first element and returns both an optional
    /// result and the end iterator.
    ///
    /// \tparam InIter      The type of the source begin iterator (deduced).
    /// \tparam Sent        The type of the source sentinel (deduced).
    /// \tparam F           The type of the binary function object.
    ///
    /// \param first        Refers to the beginning of the sequence.
    /// \param last         Refers to the end of the sequence.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            A fold_left_first_with_iter_result containing the end
    ///                     iterator and an optional with the fold result.
    ///
    template <typename InIter, typename Sent, typename F>
    auto fold_left_first_with_iter(InIter first, Sent last, F f)
        -> fold_left_first_with_iter_result<InIter,
            hpx::optional<
                std::decay_t<hpx::util::invoke_result_t<F&,
                    hpx::traits::iter_reference_t<InIter>,
                    hpx::traits::iter_reference_t<InIter>>>>>;

    /// Left-folds a range using the first element and returns both an optional
    /// result and the end iterator.
    ///
    /// \tparam Rng         The type of the source range (deduced).
    /// \tparam F           The type of the binary function object.
    ///
    /// \param rng          Refers to the sequence of elements.
    /// \param f            Binary function object that will be applied.
    ///
    /// \returns            A fold_left_first_with_iter_result containing the end
    ///                     iterator and an optional with the fold result.
    ///
    template <typename Rng, typename F>
    auto fold_left_first_with_iter(Rng&& rng, F f)
        -> fold_left_first_with_iter_result<
            std::ranges::iterator_t<Rng>,
            hpx::optional<
                std::decay_t<hpx::util::invoke_result_t<F&,
                    hpx::traits::range_reference_t<Rng>,
                    hpx::traits::range_reference_t<Rng>>>>>;

    // clang-format on
}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/datastructures/optional.hpp>
#include <hpx/iterator_support/traits/is_foldable.hpp>
#include <hpx/iterator_support/traits/is_iterator.hpp>
#include <hpx/iterator_support/traits/is_range.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/ranges_facilities.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    /// Result type for fold_left_with_iter algorithms
    template <typename I, typename T>
    using fold_left_with_iter_result = in_value_result<I, T>;

    template <typename I, typename T>
    using fold_left_first_with_iter_result = in_value_result<I, T>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::fold_left_with_iter
    HPX_CXX_CORE_EXPORT inline constexpr struct fold_left_with_iter_t final
      : hpx::detail::tag_dispatch<fold_left_with_iter_t,
            hpx::detail::tag_parallel_algorithm<fold_left_with_iter_t>>
    {
        template <typename InIter, typename Sent, typename T, typename F>
        // clang-format off
        requires (
            std::input_iterator<InIter> &&
            std::is_convertible_v<hpx::traits::iter_value_t<InIter>, T> &&
            std::sentinel_for<Sent, InIter> &&
            hpx::is_indirectly_binary_left_foldable<F, T, InIter>
        )
        // clang-format on
        static auto invoke_default(InIter first, Sent last, T init, F f)
        {
            using U = std::decay_t<hpx::util::invoke_result_t<F&, T,
                hpx::traits::iter_reference_t<InIter>>>;
            using result_type = fold_left_with_iter_result<InIter, U>;

            if (first == last)
            {
                return result_type{HPX_MOVE(first), U(HPX_MOVE(init))};
            }

            U result = HPX_INVOKE(f, HPX_MOVE(init), *first);
            ++first;

            for (; first != last; ++first)
            {
                result = HPX_INVOKE(f, HPX_MOVE(result), *first);
            }
            return result_type{HPX_MOVE(first), HPX_MOVE(result)};
        }

        template <typename Rng, typename T, typename F>
        // clang-format off
        requires (
            std::ranges::input_range<Rng> &&
            std::is_convertible_v<hpx::traits::iter_value_t<
                std::ranges::iterator_t<Rng>>, T> &&
            hpx::is_indirectly_binary_left_foldable<F, T,
                std::ranges::iterator_t<Rng>>
        )
        // clang-format on
        static auto invoke_default(Rng&& rng, T init, F f)
        {
            return invoke_default(hpx::util::begin(rng), hpx::util::end(rng),
                HPX_MOVE(init), HPX_MOVE(f));
        }
    } fold_left_with_iter{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::fold_left_first_with_iter
    HPX_CXX_CORE_EXPORT inline constexpr struct fold_left_first_with_iter_t
        final
      : hpx::detail::tag_dispatch<fold_left_first_with_iter_t,
            hpx::detail::tag_parallel_algorithm<fold_left_first_with_iter_t>>
    {
        template <typename InIter, typename Sent, typename F>
        // clang-format off
        requires (
            std::input_iterator<InIter> &&
            std::sentinel_for<Sent, InIter> &&
            hpx::is_indirectly_binary_left_foldable<F,
                hpx::traits::iter_value_t<InIter>, InIter>
        )
        // clang-format on
        static auto invoke_default(InIter first, Sent last, F f)
        {
            using U = decltype(HPX_INVOKE(f, *first, *first));
            using result_type =
                fold_left_first_with_iter_result<InIter, hpx::optional<U>>;

            if (first == last)
            {
                return result_type{HPX_MOVE(first), hpx::optional<U>()};
            }

            U result = *first;
            ++first;

            for (; first != last; ++first)
            {
                result = HPX_INVOKE(f, HPX_MOVE(result), *first);
            }
            return result_type{
                HPX_MOVE(first), hpx::optional<U>(HPX_MOVE(result))};
        }

        template <typename Rng, typename F>
        // clang-format off
        requires (
            std::ranges::input_range<Rng> &&
            hpx::is_indirectly_binary_left_foldable<F,
                hpx::traits::iter_value_t<std::ranges::iterator_t<Rng>>,
                std::ranges::iterator_t<Rng>>
        )
        // clang-format on
        static auto invoke_default(Rng&& rng, F f)
        {
            return invoke_default(
                hpx::util::begin(rng), hpx::util::end(rng), HPX_MOVE(f));
        }
    } fold_left_first_with_iter{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::fold_left
    HPX_CXX_CORE_EXPORT inline constexpr struct fold_left_t final
      : hpx::detail::tag_dispatch<fold_left_t,
            hpx::detail::tag_parallel_algorithm<fold_left_t>>
    {
        template <typename InIter, typename Sent, typename T, typename F>
        // clang-format off
        requires (
            std::input_iterator<InIter> &&
            std::sentinel_for<Sent, InIter> &&
            std::is_convertible_v<hpx::traits::iter_value_t<InIter>, T> &&
            hpx::is_indirectly_binary_left_foldable<F, T, InIter>
        )
        // clang-format on
        static auto invoke_default(InIter first, Sent last, T init, F f)
        {
            return hpx::ranges::fold_left_with_iter(
                HPX_MOVE(first), HPX_MOVE(last), HPX_MOVE(init), HPX_MOVE(f))
                .value;
        }

        template <typename Rng, typename T, typename F>
        // clang-format off
        requires (
            std::ranges::input_range<Rng> &&
            std::is_convertible_v<hpx::traits::iter_value_t<
                std::ranges::iterator_t<Rng>>, T> &&
            hpx::is_indirectly_binary_left_foldable<F, T,
                std::ranges::iterator_t<Rng>>
        )
        // clang-format on
        static auto invoke_default(Rng&& rng, T init, F f)
        {
            return hpx::ranges::fold_left_with_iter(hpx::util::begin(rng),
                hpx::util::end(rng), HPX_MOVE(init), HPX_MOVE(f))
                .value;
        }
    } fold_left{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::fold_left_first
    HPX_CXX_CORE_EXPORT inline constexpr struct fold_left_first_t final
      : hpx::detail::tag_dispatch<fold_left_first_t,
            hpx::detail::tag_parallel_algorithm<fold_left_first_t>>
    {
        template <typename InIter, typename Sent, typename F>
        // clang-format off
        requires (
            std::input_iterator<InIter> &&
            std::sentinel_for<Sent, InIter> &&
            hpx::is_indirectly_binary_left_foldable<F,
                hpx::traits::iter_value_t<InIter>, InIter>
        )
        // clang-format on
        static auto invoke_default(InIter first, Sent last, F f)
        {
            return hpx::ranges::fold_left_first_with_iter(
                HPX_MOVE(first), HPX_MOVE(last), HPX_MOVE(f))
                .value;
        }

        template <typename Rng, typename F>
        // clang-format off
        requires (
            std::ranges::input_range<Rng> &&
            hpx::is_indirectly_binary_left_foldable<F,
                hpx::traits::iter_value_t<std::ranges::iterator_t<Rng>>,
                std::ranges::iterator_t<Rng>>
        )
        // clang-format on
        static auto invoke_default(Rng&& rng, F f)
        {
            return hpx::ranges::fold_left_first_with_iter(
                hpx::util::begin(rng), hpx::util::end(rng), HPX_MOVE(f))
                .value;
        }
    } fold_left_first{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::fold_right
    HPX_CXX_CORE_EXPORT inline constexpr struct fold_right_t final
      : hpx::detail::tag_dispatch<fold_right_t,
            hpx::detail::tag_parallel_algorithm<fold_right_t>>
    {
        template <typename BidIter, typename Sent, typename T, typename F>
        // clang-format off
        requires (
            std::bidirectional_iterator<BidIter> &&
            std::is_convertible_v<hpx::traits::iter_value_t<BidIter>, T> &&
            std::sentinel_for<Sent, BidIter> &&
            hpx::is_indirectly_binary_right_foldable<F, T, BidIter>
        )
        // clang-format on
        static auto invoke_default(BidIter first, Sent last, T init, F f)
        {
            using U = std::decay_t<hpx::util::invoke_result_t<F&,
                hpx::traits::iter_reference_t<BidIter>, T>>;

            if (first == last)
            {
                return U(HPX_MOVE(init));
            }

            auto it = hpx::parallel::detail::advance_to_sentinel(first, last);
            U result = HPX_MOVE(init);

            while (it != first)
            {
                result = HPX_INVOKE(f, *--it, HPX_MOVE(result));
            }
            return result;
        }

        template <typename Rng, typename T, typename F>
        // clang-format off
        requires (
            std::ranges::bidirectional_range<Rng> &&
            std::is_convertible_v<hpx::traits::iter_value_t<
                std::ranges::iterator_t<Rng>>, T> &&
            hpx::is_indirectly_binary_right_foldable<F, T,
                std::ranges::iterator_t<Rng>>
        )
        // clang-format on
        static auto invoke_default(Rng&& rng, T init, F f)
        {
            return invoke_default(hpx::util::begin(rng), hpx::util::end(rng),
                HPX_MOVE(init), HPX_MOVE(f));
        }
    } fold_right{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::fold_right_last
    HPX_CXX_CORE_EXPORT inline constexpr struct fold_right_last_t final
      : hpx::detail::tag_dispatch<fold_right_last_t,
            hpx::detail::tag_parallel_algorithm<fold_right_last_t>>
    {
        template <typename BidIter, typename Sent, typename F>
        // clang-format off
        requires (
            std::bidirectional_iterator<BidIter> &&
            std::sentinel_for<Sent, BidIter> &&
            hpx::is_indirectly_binary_right_foldable<F,
                hpx::traits::iter_value_t<BidIter>, BidIter>
        )
        // clang-format on
        static auto invoke_default(BidIter first, Sent last, F f)
        {
            using U = decltype(HPX_INVOKE(f, *first, *first));
            using result_type = hpx::optional<U>;

            if (first == last)
            {
                return result_type();
            }

            auto it = hpx::parallel::detail::advance_to_sentinel(first, last);
            U result = *--it;

            while (it != first)
            {
                result = HPX_INVOKE(f, *--it, HPX_MOVE(result));
            }
            return result_type(HPX_MOVE(result));
        }

        template <typename Rng, typename F>
        // clang-format off
        requires (
            std::ranges::bidirectional_range<Rng> &&
            hpx::is_indirectly_binary_right_foldable<F,
                hpx::traits::iter_value_t<std::ranges::iterator_t<Rng>>,
                std::ranges::iterator_t<Rng>>
        )
        // clang-format on
        static auto invoke_default(Rng&& rng, F f)
        {
            return invoke_default(
                hpx::util::begin(rng), hpx::util::end(rng), HPX_MOVE(f));
        }
    } fold_right_last{};

}    // namespace hpx::ranges

#endif    // DOXYGEN
