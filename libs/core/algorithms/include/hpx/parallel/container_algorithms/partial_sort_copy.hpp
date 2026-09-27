//  Copyright (c) 2020-2023 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/partial_sort_copy.hpp
/// \page hpx::ranges::partial_sort_copy
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts some of the elements in the range [first, last) in ascending
    /// order, storing the result in the range [r_first, r_last). At most
    /// r_last - r_first of the elements are placed sorted to the range
    /// [r_first, r_first + n) where n is the number of elements to sort
    /// (n = min(last - first, r_last - r_first)).
    ///
    /// \note   Complexity: O(N log(min(D,N))), where N =
    ///         std::distance(first, last) and D = std::distance(r_first,
    ///         r_last) comparisons.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Sent1       The type of the source sentinel (deduced).This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam RandIter    The type of the destination iterators used(deduced)
    ///                     This iterator type must meet the requirements of an
    ///                     random iterator.
    /// \tparam Sent2       The type of the destination sentinel (deduced).This
    ///                     sentinel type must be a sentinel for RandIter.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Comp defaults to detail::less.
    /// \tparam Proj1       The type of an optional projection function for the
    ///                     input range. This defaults to
    ///                     \a hpx::identity.
    /// \tparam Proj1       The type of an optional projection function for the
    ///                     output range. This defaults to
    ///                     \a hpx::identity.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the sentinel value denoting the end of
    ///                     the sequence of elements the algorithm will be
    ///                     applied to.
    /// \param r_first      Refers to the beginning of the destination range.
    /// \param r_last       Refers to the sentinel denoting the end of the
    ///                     destination range.
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that
    ///                     comp will not apply any non-constant function
    ///                     through the dereferenced iterator. This defaults to
    ///                     detail::less.
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each pair of elements as a
    ///                     projection operation before the actual predicate
    ///                     \a comp is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each pair of elements as a
    ///                     projection operation after the actual predicate
    ///                     \a comp is invoked.
    ///
    /// The assignments in the parallel \a partial_sort_copy algorithm invoked
    /// without an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a partial_sort_copy algorithm returns a
    ///           returns \a partial_sort_copy_result<InIter, RandIter>.
    ///           The algorithm returns {last, result_first + N}.
    ///
    template <typename InIter, typename Sent1, typename RandIter,
        typename Sent2, typename Comp = ranges::less,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    partial_sort_copy_result<InIter, RandIter> partial_sort_copy(InIter first,
        Sent1 last, RandIter r_first, Sent2 r_last, Comp&& comp = Comp(),
        Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts some of the elements in the range [first, last) in ascending
    /// order, storing the result in the range [r_first, r_last). At most
    /// r_last - r_first of the elements are placed sorted to the range
    /// [r_first, r_first + n) where n is the number of elements to sort
    /// (n = min(last - first, r_last - r_first)).
    ///
    /// \note   Complexity: O(N log(min(D,N))), where N =
    ///         std::distance(first, last) and D = std::distance(r_first,
    ///         r_last) comparisons.
    ///
    /// \tparam Rng1        The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a input iterator.
    /// \tparam Rng2        The type of the destination range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a random iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Comp defaults to detail::less.
    /// \tparam Proj1       The type of an optional projection function for the
    ///                     input range. This defaults to
    ///                     \a hpx::identity.
    /// \tparam Proj2       The type of an optional projection function for the
    ///                     output range. This defaults to
    ///                     \a hpx::identity.
    ///
    /// \param rng1         Refers to the source range.
    /// \param rng2         Refers to the destination range.
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that
    ///                     comp will not apply any non-constant function
    ///                     through the dereferenced iterator. This defaults to
    ///                     detail::less.
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each pair of elements as a
    ///                     projection operation before the actual predicate
    ///                     \a comp is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each pair of elements as a
    ///                     projection operation after the actual predicate
    ///                     \a comp is invoked.
    ///
    /// The assignments in the parallel \a partial_sort_copy algorithm invoked
    /// without an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a partial_sort_copy algorithm returns \a
    ///           partial_sort_copy_result<range_iterator_t<Rng1>,
    ///           range_iterator_t<Rng2>>.
    ///           The algorithm returns {last, result_first + N}.
    ///
    template <typename Rng1, typename Rng2,
        typename Comp = ranges::less,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    partial_sort_copy_result<std::ranges::iterator_t<Rng1>,
        std::ranges::iterator_t<Rng2>>
    partial_sort_copy(Rng1&& rng1, Rng2&& rng2, Comp&& comp = Comp(),
        Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    // clang-format on

    /// \brief Execution-policy overload of \c partial_sort_copy.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I1,
        std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
        std::sized_sentinel_for<I2> S2, typename Comp = std::ranges::less,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_copyable<I1, I2> && std::sortable<I2, Comp, Proj2> &&
        std::indirect_strict_weak_order<Comp, std::projected<I1, Proj1>,
            std::projected<I2, Proj2>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        partial_sort_copy_result<I1, I2>>
    partial_sort_copy(ExPolicy&& policy, I1 first1, S1 last1, I2 first2,
        S2 last2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {});

    /// \brief Execution-policy overload of \c partial_sort_copy.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R1,
        std::ranges::random_access_range R2, typename Comp = std::ranges::less,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R1> && std::ranges::sized_range<R2> &&
        std::indirectly_copyable<std::ranges::iterator_t<R1>,
            std::ranges::iterator_t<R2>> &&
        std::sortable<std::ranges::iterator_t<R2>, Comp, Proj2> &&
        std::indirect_strict_weak_order<Comp,
            std::projected<std::ranges::iterator_t<R1>, Proj1>,
            std::projected<std::ranges::iterator_t<R2>, Proj2>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        partial_sort_copy_result<std::ranges::borrowed_iterator_t<R1>,
            std::ranges::borrowed_iterator_t<R2>>>
    partial_sort_copy(ExPolicy&& policy, R1&& rng1, R2&& rng2, Comp comp = {},
        Proj1 proj1 = {}, Proj2 proj2 = {});
}}    // namespace hpx::ranges

#else

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/partial_sort_copy.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using partial_sort_copy_result = parallel::util::in_out_result<I, O>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::partial_sort_copy
    HPX_CXX_CORE_EXPORT inline constexpr struct partial_sort_copy_t final
      : hpx::detail::tag_dispatch<partial_sort_copy_t,
            hpx::detail::tag_parallel_algorithm<partial_sort_copy_t>>
    {
        template <typename InIter, typename Sent1, typename RandIter,
            typename Sent2, typename Comp = ranges::less,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<InIter> &&
                std::sentinel_for<Sent1, InIter> &&
                hpx::traits::is_iterator_v<RandIter> &&
                std::sentinel_for<Sent2, RandIter> &&
                parallel::traits::is_projected_v<Proj1, InIter> &&
                parallel::traits::is_projected_v<Proj2, RandIter> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Comp,
                    parallel::traits::projected<Proj1, InIter>,
                    parallel::traits::projected<Proj1, InIter>
                >
            )
        // clang-format on
        static partial_sort_copy_result<InIter, RandIter> invoke_default(
            InIter first, Sent1 last, RandIter r_first, Sent2 r_last,
            Comp comp = Comp(), Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            static_assert(
                std::input_iterator<InIter>, "Requires an input iterator.");

            static_assert(std::random_access_iterator<RandIter>,
                "Requires a random access iterator.");

            using result_type = partial_sort_copy_result<InIter, RandIter>;

            return hpx::parallel::detail::partial_sort_copy<result_type>().call(
                hpx::execution::seq, first, last, r_first, r_last,
                HPX_MOVE(comp), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename ExPolicy, std::random_access_iterator I1,
            std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, typename Comp = std::ranges::less,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_copyable<I1, I2> &&
            std::sortable<I2, Comp, Proj2> &&
            std::indirect_strict_weak_order<Comp, std::projected<I1, Proj1>,
                std::projected<I2, Proj2>>
        static decltype(auto) invoke_default(ExPolicy&& policy, I1 first1,
            S1 last1, I2 first2, S2 last2, Comp comp = {}, Proj1 proj1 = {},
            Proj2 proj2 = {})
        {
            return parallel::detail::range_partial_sort_copy<I1, I2>().call(
                HPX_FORWARD(ExPolicy, policy), first1,
                first1 + (last1 - first1), first2, first2 + (last2 - first2),
                HPX_MOVE(comp), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename Rng1, typename Rng2, typename Comp = ranges::less,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng1> &&
                std::ranges::range<Rng2> &&
                parallel::traits::is_projected_range_v<Proj1, Rng1> &&
                parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Comp,
                    parallel::traits::projected_range<Proj1, Rng1>,
                    parallel::traits::projected_range<Proj1, Rng1>
                >
            )
        // clang-format on
        static partial_sort_copy_result<std::ranges::iterator_t<Rng1>,
            std::ranges::iterator_t<Rng2>>
        invoke_default(Rng1&& rng1, Rng2&& rng2, Comp comp = Comp(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            using iterator_type1 = std::ranges::iterator_t<Rng1>;
            using iterator_type2 = std::ranges::iterator_t<Rng2>;
            using result_type =
                partial_sort_copy_result<iterator_type1, iterator_type2>;

            static_assert(std::forward_iterator<iterator_type1>,
                "Requires a forward iterator.");

            static_assert(std::random_access_iterator<iterator_type2>,
                "Requires a random access iterator.");

            return hpx::parallel::detail::partial_sort_copy<result_type>().call(
                hpx::execution::seq, hpx::util::begin(rng1),
                hpx::util::end(rng1), hpx::util::begin(rng2),
                hpx::util::end(rng2), HPX_MOVE(comp), HPX_MOVE(proj1),
                HPX_MOVE(proj2));
        }

        template <typename ExPolicy, std::ranges::random_access_range R1,
            std::ranges::random_access_range R2,
            typename Comp = std::ranges::less, typename Proj1 = hpx::identity,
            typename Proj2 = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R1> && std::ranges::sized_range<R2> &&
            std::indirectly_copyable<std::ranges::iterator_t<R1>,
                std::ranges::iterator_t<R2>> &&
            std::sortable<std::ranges::iterator_t<R2>, Comp, Proj2> &&
            std::indirect_strict_weak_order<Comp,
                std::projected<std::ranges::iterator_t<R1>, Proj1>,
                std::projected<std::ranges::iterator_t<R2>, Proj2>>
        static decltype(auto) invoke_default(ExPolicy&& policy, R1&& rng1,
            R2&& rng2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})
        {
            using I1 = std::ranges::iterator_t<R1>;
            using I2 = std::ranges::iterator_t<R2>;
            auto first1 = std::ranges::begin(rng1);
            auto first2 = std::ranges::begin(rng2);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first1,
                    first1 + std::ranges::distance(rng1), first2,
                    first2 + std::ranges::distance(rng2), HPX_MOVE(comp),
                    HPX_MOVE(proj1), HPX_MOVE(proj2)),
                [](partial_sort_copy_result<I1, I2> result)
                    -> partial_sort_copy_result<
                        std::ranges::borrowed_iterator_t<R1>,
                        std::ranges::borrowed_iterator_t<R2>> {
                    return {result.in, result.out};
                });
        }
    } partial_sort_copy{};
}    // namespace hpx::ranges

#endif
