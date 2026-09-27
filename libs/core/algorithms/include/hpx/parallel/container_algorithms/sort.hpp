//  Copyright (c) 2015-2023 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/sort.hpp
/// \page hpx::ranges::sort
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range [first, last) in ascending order. The
    /// order of equal elements is not guaranteed to be preserved. The function
    /// uses the given comparison function object comp (defaults to using
    /// operator<()).
    ///
    /// \note   Complexity: O(N log(N)), where N = detail::distance(first, last)
    ///                     comparisons.
    ///
    /// A sequence is sorted with respect to a comparator \a comp and a
    /// projection \a proj if for every iterator i pointing to the sequence and
    /// every non-negative integer n such that i + n is a valid iterator
    /// pointing to an element of the sequence, and
    /// INVOKE(comp, INVOKE(proj, *(i + n)), INVOKE(proj, *i)) == false.
    ///
    /// \tparam RandomIt    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for RandomIt.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that comp
    ///                     will not apply any non-constant function through the
    ///                     dereferenced iterator.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each pair of elements as a
    ///                     projection operation before the actual predicate
    ///                     \a comp is invoked.
    ///
    /// \a comp has to induce a strict weak ordering on the values.
    ///
    /// The assignments in the parallel \a sort algorithm invoked without
    /// an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a sort algorithm returns \a RandomIt.
    ///           The algorithm returns an iterator pointing to the first
    ///           element after the last element in the input sequence.
    ///
    template <typename RandomIt, typename Sent,
        typename Comp = ranges::less,
        typename Proj = hpx::identity>
    RandomIt sort(RandomIt first, Sent last,  Comp&& comp = Comp(),
        Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range \a rng  in ascending order. The
    /// order of equal elements is not guaranteed to be preserved. The function
    /// uses the given comparison function object comp (defaults to using
    /// operator<()).
    ///
    /// \note   Complexity: O(N log(N)),
    ///             where N = std::distance(begin(rng), end(rng)) comparisons.
    ///
    /// A sequence is sorted with respect to a comparator \a comp and a
    /// projection \a proj if for every iterator i pointing to the sequence and
    /// every non-negative integer n such that i + n is a valid iterator
    /// pointing to an element of the sequence, and
    /// INVOKE(comp, INVOKE(proj, *(i + n)), INVOKE(proj, *i)) == false.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Comp     The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that comp
    ///                     will not apply any non-constant function through the
    ///                     dereferenced iterator.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each pair of elements as a
    ///                     projection operation before the actual predicate
    ///                     \a comp is invoked.
    ///
    /// \a comp has to induce a strict weak ordering on the values.
    ///
    /// The assignments in the parallel \a sort algorithm invoked without
    /// an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a sort algorithm returns \a
    ///           std::ranges::iterator_t<Rng>.
    ///           It returns \a last.
    template <typename Rng, typename Comp, typename Proj>
    std::ranges::iterator_t<Rng>
    sort(Rng&& rng, Comp&& comp = Comp(), Proj&& proj = Proj());

    // clang-format on

    /// \brief Execution-policy overload of \c sort.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename Comp = std::ranges::less,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::sortable<I, Comp, Proj>
    parallel::util::detail::algorithm_result_t<ExPolicy, I> sort(
        ExPolicy&& policy, I first, S last, Comp comp = {}, Proj proj = {});

    /// \brief Execution-policy overload of \c sort.
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
    sort(ExPolicy&& policy, R&& rng, Comp comp = {}, Proj proj = {});
}}    // namespace hpx::ranges

#else

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/sort.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::sort
    HPX_CXX_CORE_EXPORT inline constexpr struct sort_t final
      : hpx::detail::tag_dispatch<sort_t,
            hpx::detail::tag_parallel_algorithm<sort_t>>
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
        static RandomIt invoke_default(
            RandomIt first, Sent last, Comp comp = Comp(), Proj proj = Proj())
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "Requires a random access iterator.");

            return hpx::parallel::detail::sort<RandomIt>().call(
                hpx::execution::seq, first, last, HPX_MOVE(comp),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Comp = std::ranges::less,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::sortable<I, Comp, Proj>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, Comp comp = {}, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::detail::sort<I>().call(
                HPX_FORWARD(ExPolicy, policy), first, end, HPX_MOVE(comp),
                HPX_MOVE(proj));
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
        static std::ranges::iterator_t<Rng> invoke_default(
            Rng&& rng, Comp comp = Comp(), Proj proj = Proj())
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::random_access_iterator<iterator_type>,
                "Requires a random access iterator.");

            return hpx::parallel::detail::sort<iterator_type>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                HPX_MOVE(comp), HPX_MOVE(proj));
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Comp = std::ranges::less, typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, Comp comp = {}, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), HPX_MOVE(comp),
                    HPX_MOVE(proj)),
                [](I result) -> std::ranges::borrowed_iterator_t<R> {
                    return result;
                });
        }
    } sort{};
}    // namespace hpx::ranges

#endif
