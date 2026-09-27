//  Copyright (c) 2020-2023 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/partial_sort.hpp
/// \page hpx::ranges::partial_sort
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Places the first middle - first elements from the range [first, last)
    /// as sorted with respect to comp into the range [first, middle). The rest
    /// of the elements in the range [middle, last) are placed in an unspecified
    /// order.
    ///
    /// \note   Complexity: Approximately (last - first) * log(middle - first)
    ///         comparisons.
    ///
    /// \tparam RandomIt    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for RandomIt.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Comp defaults to detail::less.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param middle       Refers to the middle of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that
    ///                     comp will not apply any non-constant function
    ///                     through the dereferenced iterator. Comp defaults
    ///                     to detail::less.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each pair of elements as a
    ///                     projection operation before the actual predicate
    ///                     \a comp is invoked.
    ///
    /// The assignments in the parallel \a partial_sort algorithm invoked without
    /// an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a partial_sort algorithm returns \a RandomIt.
    ///           The algorithm returns an iterator pointing to the first
    ///           element after the last element in the input sequence.
    ///
    template <typename RandomIt, typename Sent,
        typename Comp = ranges::less,
        typename Proj = hpx::identity>
    RandomIt partial_sort(RandomIt first, RandomIt middle, Sent last,
        Comp&& comp = Comp(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Places the first middle - first elements from the range [first, last)
    /// as sorted with respect to comp into the range [first, middle). The rest
    /// of the elements in the range [middle, last) are placed in an unspecified
    /// order.
    ///
    /// \note   Complexity: Approximately (last - first) * log(middle - first)
    ///         comparisons.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Comp     The type of the function/function object to use
    ///                     (deduced). Comp defaults to detail::less.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param middle       Refers to the middle of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that
    ///                     comp will not apply any non-constant function
    ///                     through the dereferenced iterator. Comp defaults
    ///                     to detail::less.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each pair of elements as a
    ///                     projection operation before the actual predicate
    ///                     \a comp is invoked.
    ///
    /// The assignments in the parallel \a partial_sort algorithm invoked without
    /// an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a partial_sort algorithm returns \a
    ///           std::ranges::iterator_t<Rng>.
    ///           It returns \a last.
    template <typename Rng,
        typename Comp = ranges::less,
        typename Proj = hpx::identity>
    std::ranges::iterator_t<Rng>
    partial_sort(Rng&& rng, std::ranges::iterator_t<Rng> middle,
        Comp&& comp = Comp(), Proj&& proj = Proj());

    // clang-format on

    /// \brief Execution-policy overload of \c partial_sort.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename Comp = std::ranges::less,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::sortable<I, Comp, Proj>
    parallel::util::detail::algorithm_result_t<ExPolicy, I> partial_sort(
        ExPolicy&& policy, I first, I middle, S last, Comp comp = {},
        Proj proj = {});

    /// \brief Execution-policy overload of \c partial_sort.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        typename Comp = std::ranges::less, typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_iterator_t<R>>
    partial_sort(ExPolicy&& policy, R&& rng, std::ranges::iterator_t<R> middle,
        Comp comp = {}, Proj proj = {});
}}    // namespace hpx::ranges

#else

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/partial_sort.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::partial_sort
    HPX_CXX_CORE_EXPORT inline constexpr struct partial_sort_t final
      : hpx::detail::tag_dispatch<partial_sort_t,
            hpx::detail::tag_parallel_algorithm<partial_sort_t>>
    {
        template <typename RandomIt, typename Sent,
            typename Comp = ranges::less, typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<RandomIt> &&
                std::sentinel_for<Sent, RandomIt> &&
                parallel::traits::is_projected_v<Proj, RandomIt> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Comp,
                    parallel::traits::projected<Proj, RandomIt>,
                    parallel::traits::projected<Proj, RandomIt>
                >
            )
        // clang-format on
        static RandomIt invoke_default(RandomIt first, RandomIt middle,
            Sent last, Comp comp = Comp(), Proj proj = Proj())
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "Requires a random access iterator.");

            return hpx::parallel::partial_sort<RandomIt>().call(
                hpx::execution::seq, first, middle, last, HPX_MOVE(comp),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Comp = std::ranges::less,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::sortable<I, Comp, Proj>
        static decltype(auto) invoke_default(ExPolicy&& policy, I first,
            I middle, S last, Comp comp = {}, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::partial_sort<I>().call(
                HPX_FORWARD(ExPolicy, policy), first, middle, end,
                HPX_MOVE(comp), HPX_MOVE(proj));
        }

        template <typename Rng, typename Comp = ranges::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                parallel::traits::is_projected_range_v<Proj, Rng> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Comp,
                    parallel::traits::projected_range<Proj, Rng>,
                    parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static std::ranges::iterator_t<Rng> invoke_default(Rng&& rng,
            std::ranges::iterator_t<Rng> middle, Comp comp = Comp(),
            Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::random_access_iterator<iterator_type>,
                "Requires a random access iterator.");

            return hpx::parallel::partial_sort<iterator_type>().call(
                hpx::execution::seq, hpx::util::begin(rng), middle,
                hpx::util::end(rng), HPX_MOVE(comp), HPX_MOVE(proj));
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Comp = std::ranges::less, typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
        static decltype(auto) invoke_default(ExPolicy&& policy, R&& rng,
            std::ranges::iterator_t<R> middle, Comp comp = {}, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first, middle,
                    first + std::ranges::distance(rng), HPX_MOVE(comp),
                    HPX_MOVE(proj)),
                [](I result) -> std::ranges::borrowed_iterator_t<R> {
                    return result;
                });
        }
    } partial_sort{};
}    // namespace hpx::ranges

#endif
