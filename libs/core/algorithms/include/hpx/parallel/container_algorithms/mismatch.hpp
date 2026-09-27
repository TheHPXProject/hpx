//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2022 Dimitra Karatza
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/mismatch.hpp
/// \page hpx::ranges::mismatch
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
    // clang-format off

    /// Returns true if the range [first1, last1) is mismatch to the range
    /// [first2, last2), and false otherwise.
    ///
    /// \note   Complexity: At most min(last1 - first1, last2 - first2)
    ///         applications of the predicate \a f. If \a FwdIter1
    ///         and \a FwdIter2 meet the requirements of \a RandomAccessIterator
    ///         and (last1 - first1) != (last2 - first2) then no applications
    ///         of the predicate \a f are made.
    ///
    /// \tparam Iter1       The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent1       The type of the source iterators used for the end of
    ///                     the first range (deduced).
    /// \tparam Iter2       The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent2       The type of the source iterators used for the end of
    ///                     the second range (deduced).
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a mismatch requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function applied
    ///                     to the first range. This
    ///                     defaults to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function applied
    ///                     to the second range. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last1        Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
    /// \param last2        Refers to the end of the sequence of elements of
    ///                     the second range the algorithm will be applied to.
    /// \param op           The binary predicate which returns true if the
    ///                     elements should be treated as mismatch. The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     first range as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     second range as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \note     The two ranges are considered mismatch if, for every iterator
    ///           i in the range [first1,last1), *i mismatchs *(first2 + (i - first1)).
    ///           This overload of mismatch uses operator== to determine if two
    ///           elements are mismatch.
    ///
    /// \returns  The \a mismatch algorithm returns \a bool.
    ///           The \a mismatch algorithm returns true if the elements in the
    ///           two ranges are mismatch, otherwise it returns false.
    ///           If the length of the range [first1, last1) does not mismatch
    ///           the length of the range [first2, last2), it returns false.
    ///
    template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Pred = equal_to,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    mismatch_result<Iter1, Iter2> mismatch(Iter1 first1, Sent1 last1,
        Iter2 first2, Sent2 last2, Pred&& op = Pred(),
        Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    /// Returns std::pair with iterators to the first two non-equivalent
    /// elements.
    ///
    /// \note   Complexity: At most \a last1 - \a first1 applications of the
    ///         predicate \a f.
    ///
    /// \tparam Rng1        The type of the first source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Rng2        The type of the second source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a mismatch requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function applied
    ///                     to the first range. This
    ///                     defaults to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function applied
    ///                     to the second range. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng1         Refers to the first sequence of elements the
    ///                     algorithm will be applied to.
    /// \param rng2         Refers to the second sequence of elements the
    ///                     algorithm will be applied to.
    /// \param op           The binary predicate which returns true if the
    ///                     elements should be treated as mismatch. The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     first range as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     second range as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a mismatch algorithm returns \a std::pair<FwdIter1, FwdIter2>.
    ///           The \a mismatch algorithm returns the first mismatching pair
    ///           of elements from two ranges: one defined by [first1, last1)
    ///           and another defined by [first2, last2).
    ///
    template <typename Rng1, typename Rng2, typename Pred = equal_to,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    mismatch_result<
        typename hpx::traits::range_traits<Rng1>::iterator_type,
        typename hpx::traits::range_traits<Rng2>::iterator_type>
    mismatch(Rng1&& rng1, Rng2&& rng2, Pred&& op = Pred(),
        Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());
    // clang-format on

    /// \brief Execution-policy overload of \c mismatch.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I1,
        std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
        std::sized_sentinel_for<I2> S2, typename Pred = std::ranges::equal_to,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        mismatch_result<I1, I2>>
    mismatch(ExPolicy&& policy, I1 first1, S1 last1, I2 first2, S2 last2,
        Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {});

    /// \brief Execution-policy overload of \c mismatch.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R1,
        std::ranges::random_access_range R2,
        typename Pred = std::ranges::equal_to, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R1> && std::ranges::sized_range<R2> &&
        std::indirectly_comparable<std::ranges::iterator_t<R1>,
            std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        mismatch_result<std::ranges::borrowed_iterator_t<R1>,
            std::ranges::borrowed_iterator_t<R2>>>
    mismatch(ExPolicy&& policy, R1&& rng1, R2&& rng2, Pred pred = {},
        Proj1 proj1 = {}, Proj2 proj2 = {});
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/mismatch.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
    using mismatch_result = hpx::parallel::util::in_in_result<Iter1, Iter2>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::mismatch
    HPX_CXX_CORE_EXPORT inline constexpr struct mismatch_t final
      : hpx::detail::tag_dispatch<mismatch_t,
            hpx::detail::tag_parallel_algorithm<mismatch_t>>
    {
        template <typename ExPolicy, std::random_access_iterator I1,
            std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2,
            typename Pred = std::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
        static decltype(auto) invoke_default(ExPolicy&& policy, I1 first1,
            S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {},
            Proj2 proj2 = {})
        {
            auto end1 = first1 + (last1 - first1);
            auto end2 = first2 + (last2 - first2);
            return parallel::detail::mismatch_binary<mismatch_result<I1, I2>>()
                .call(HPX_FORWARD(ExPolicy, policy), first1, end1, first2, end2,
                    HPX_MOVE(pred), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename ExPolicy, std::ranges::random_access_range R1,
            std::ranges::random_access_range R2,
            typename Pred = std::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R1> && std::ranges::sized_range<R2> &&
            std::indirectly_comparable<std::ranges::iterator_t<R1>,
                std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
        static decltype(auto) invoke_default(ExPolicy&& policy, R1&& rng1,
            R2&& rng2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {})
        {
            using I1 = std::ranges::iterator_t<R1>;
            using I2 = std::ranges::iterator_t<R2>;
            auto first1 = std::ranges::begin(rng1);
            auto first2 = std::ranges::begin(rng2);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first1,
                    first1 + std::ranges::distance(rng1), first2,
                    first2 + std::ranges::distance(rng2), HPX_MOVE(pred),
                    HPX_MOVE(proj1), HPX_MOVE(proj2)),
                [](mismatch_result<I1, I2> result)
                    -> mismatch_result<std::ranges::borrowed_iterator_t<R1>,
                        std::ranges::borrowed_iterator_t<R2>> {
                    return {result.in1, result.in2};
                });
        }

        template <typename Iter1, typename Sent1, typename Iter2,
            typename Sent2, typename Pred = equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                std::sentinel_for<Sent1, Iter1> &&
                std::sentinel_for<Sent2, Iter2> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj1, Iter1>,
                    hpx::parallel::traits::projected<Proj2, Iter2>
                >
            )
        // clang-format on
        static mismatch_result<Iter1, Iter2> invoke_default(Iter1 first1,
            Sent1 last1, Iter2 first2, Sent2 last2, Pred op = Pred(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            static_assert(std::forward_iterator<Iter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<Iter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::mismatch_binary<
                mismatch_result<Iter1, Iter2>>()
                .call(hpx::execution::seq, first1, last1, first2, last2,
                    HPX_MOVE(op), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename Rng1, typename Rng2, typename Pred = equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::parallel::traits::is_projected_range_v<Proj1, Rng1> &&
                hpx::parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj1,
                        typename hpx::traits::range_traits<Rng1>::iterator_type>,
                    hpx::parallel::traits::projected<Proj2,
                        typename hpx::traits::range_traits<Rng2>::iterator_type>
                >
            )
        // clang-format on
        static mismatch_result<
            typename hpx::traits::range_traits<Rng1>::iterator_type,
            typename hpx::traits::range_traits<Rng2>::iterator_type>
        invoke_default(Rng1&& rng1, Rng2&& rng2, Pred op = Pred(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            static_assert(
                std::forward_iterator<
                    typename hpx::traits::range_traits<Rng1>::iterator_type>,
                "Requires at least forward iterator.");
            static_assert(
                std::forward_iterator<
                    typename hpx::traits::range_traits<Rng2>::iterator_type>,
                "Requires at least forward iterator.");

            using result_type = mismatch_result<
                typename hpx::traits::range_traits<Rng1>::iterator_type,
                typename hpx::traits::range_traits<Rng2>::iterator_type>;

            return hpx::parallel::detail::mismatch_binary<result_type>().call(
                hpx::execution::seq, hpx::util::begin(rng1),
                hpx::util::end(rng1), hpx::util::begin(rng2),
                hpx::util::end(rng2), HPX_MOVE(op), HPX_MOVE(proj1),
                HPX_MOVE(proj2));
        }
    } mismatch{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
