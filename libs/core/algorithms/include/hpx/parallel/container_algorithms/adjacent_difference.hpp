//  Copyright (c) 2022 Dimitra Karatza
//  Copyright (c) 2021 Karame M.Shokooh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/adjacent_difference.hpp
/// \page hpx::ranges::adjacent_difference
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    /// Searches the range [first, last) for two consecutive identical elements.
    ///
    /// \note   Complexity: Exactly the smaller of (result - first) + 1 and
    ///                     (last - first) - 1 application of the predicate
    ///                     where \a result is the value returned
    ///
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// \returns  The \a adjacent_difference algorithm returns an iterator to the
    ///           first of the identical elements. If no such elements are
    ///           found, \a last is returned.
    ///
    template <typename FwdIter1, typename FwdIter2, typename Sent>
    FwdIter2 adjacent_difference(
        FwdIter1 first, Sent last, FwdIter2 dest);

    /// Searches the \a rng for two consecutive identical elements.
    ///
    /// \note   Complexity: Exactly the smaller of (result - first) + 1 and
    ///                     (last - first) - 1 application of the predicate
    ///                     where \a result is the value returned
    ///
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// \returns  The \a adjacent_difference algorithm returns an iterator to the
    ///           first of the identical elements.
    ///
    template <typename Rng, typename FwdIter2>
    FwdIter2 adjacent_difference(Rng&& rng, FwdIter2 dest);

    /// Searches the range [first, last) for two consecutive identical elements.
    ///
    /// \note   Complexity: Exactly the smaller of (result - first) + 1 and
    ///                     (last - first) - 1 application of the predicate
    ///                     where \a result is the value returned
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// \returns  The \a adjacent_difference algorithm returns an iterator to the
    ///           first of the identical elements. If no such elements are
    ///           found, \a last is returned.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Sent,
            typename FwdIter2>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            FwdIter2>
    adjacent_difference(ExPolicy&& policy, FwdIter1 first, Sent last, FwdIter2 dest);

    /// Searches the \a rng for two consecutive identical elements.
    ///
    /// \note   Complexity: Exactly the smaller of (result - first) + 1 and
    ///                     (last - first) - 1 application of the predicate
    ///                     where \a result is the value returned
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// \returns  The \a adjacent_difference algorithm returns an iterator to the
    ///           first of the identical elements.
    ///
    template <typename ExPolicy, typename Rng, typename FwdIter2>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            FwdIter2>
    adjacent_difference(ExPolicy&& policy, Rng&& rng, FwdIter2 dest);

    /// Searches the range [first, last) for two consecutive identical elements.
    ///
    /// \note   Complexity: Exactly the smaller of (result - first) + 1 and
    ///                     (last - first) - 1 application of the predicate
    ///                     where \a result is the value returned
    ///
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam Op          The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_difference requires \a Op
    ///                     to meet the requirements of \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Binary operation function object that will be applied.
    ///                     The signature of the function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &.
    ///                     The types \a Type1 and \a Type2 must be such that an
    ///                     object of type \a iterator_traits<InputIt>::value_type
    ///                     can be implicitly converted to both of them. The type
    ///                     \a Ret must be such that an object of type \a OutputIt
    ///                     can be dereferenced and assigned a value of type \a Ret.
    ///
    /// \returns  The \a adjacent_difference algorithm returns an iterator to the
    ///           first of the identical elements. If no such elements are
    ///           found, \a last is returned.
    ///
    template <typename FwdIter1, typename Sent, typename FwdIter2,
            typename Op>
    FwdIter2 adjacent_difference(FwdIter1 first, Sent last, FwdIter2 dest, Op&& op);

    /// Searches the \a rng for two consecutive identical elements.
    ///
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Op          The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_difference requires \a Op
    ///                     to meet the requirements of \a CopyConstructible.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Binary operation function object that will be applied.
    ///                     The signature of the function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &.
    ///                     The types \a Type1 and \a Type2 must be such that an
    ///                     object of type \a iterator_traits<InputIt>::value_type
    ///                     can be implicitly converted to both of them. The type
    ///                     \a Ret must be such that an object of type \a OutputIt
    ///                     can be dereferenced and assigned a value of type \a Ret.?
    ///
    /// \returns  The \a adjacent_difference algorithm returns an iterator to the
    ///           first of the identical elements.
    ///
    template <typename Rng, typename FwdIter2, typename Op>
    FwdIter2 adjacent_difference(Rng&& rng, FwdIter2 dest, Op&& op);

    /// Searches the range [first, last) for two consecutive identical elements.
    ///
    /// \note   Complexity: Exactly the smaller of (result - first) + 1 and
    ///                     (last - first) - 1 application of the predicate
    ///                     where \a result is the value returned
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam Op          The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_difference requires \a Op
    ///                     to meet the requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Binary operation function object that will be applied.
    ///                     The signature of the function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &.
    ///                     The types \a Type1 and \a Type2 must be such that an
    ///                     object of type \a iterator_traits<InputIt>::value_type
    ///                     can be implicitly converted to both of them. The type
    ///                     \a Ret must be such that an object of type \a OutputIt
    ///                     can be dereferenced and assigned a value of type \a Ret.?
    ///
    /// \returns  The \a adjacent_difference algorithm returns an iterator to the
    ///           first of the identical elements. If no such elements are
    ///           found, \a last is returned.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Sent,
            typename FwdIter2, typename Op>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            FwdIter2>
    adjacent_difference(ExPolicy&& policy, FwdIter1 first, Sent last, FwdIter2 dest,
            Op&& op);

    /// Searches the \a rng for two consecutive identical elements.
    ///
    /// \note   Complexity: Exactly the smaller of (result - first) + 1 and
    ///                     (last - first) - 1 application of the predicate
    ///                     where \a result is the value returned
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Op          The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_difference requires \a Op
    ///                     to meet the requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Binary operation function object that will be applied.
    ///                     The signature of the function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &.
    ///                     The types \a Type1 and \a Type2 must be such that an
    ///                     object of type \a iterator_traits<InputIt>::value_type
    ///                     can be implicitly converted to both of them. The type
    ///                     \a Ret must be such that an object of type \a OutputIt
    ///                     can be dereferenced and assigned a value of type \a Ret.
    ///
    /// \returns  The \a adjacent_difference algorithm returns an iterator to the
    ///           first of the identical elements.
    ///
    template <typename ExPolicy, typename Rng, typename FwdIter2,
            typename Op>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            FwdIter2>
    adjacent_difference(ExPolicy&& policy, Rng&& rng, FwdIter2 dest, Op&& op);
    // clang-format on
}}    // namespace hpx::ranges

#else

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/adjacent_difference.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>

#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT inline constexpr struct adjacent_difference_t final
      : hpx::detail::tag_dispatch<adjacent_difference_t,
            hpx::detail::tag_parallel_algorithm<adjacent_difference_t>>
    {
        template <typename FwdIter1, typename FwdIter2, typename Sent>
        // clang-format off
             requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                std::sentinel_for<Sent, FwdIter1>
            )
        // clang-format on
        static FwdIter2 invoke_default(FwdIter1 first, Sent last, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                hpx::execution::seq, first, last, dest, std::minus<>());
        }

        template <typename Rng, typename FwdIter2>
        // clang-format off
            requires(std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<FwdIter2>)
        // clang-format on
        static FwdIter2 invoke_default(Rng&& rng, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                dest, std::minus<>());
        }

        template <typename ExPolicy, typename FwdIter1, typename Sent,
            typename FwdIter2>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                std::sentinel_for<Sent, FwdIter1>
            )
        // clang-format on
        static decltype(auto) invoke_default(
            ExPolicy&& policy, FwdIter1 first, Sent last, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, dest,
                std::minus<>());
        }

        template <typename ExPolicy, typename Rng, typename FwdIter2>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        static decltype(auto) invoke_default(
            ExPolicy&& policy, Rng&& rng, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::end(rng), dest, std::minus<>());
        }

        template <typename FwdIter1, typename Sent, typename FwdIter2,
            typename Op>
        // clang-format off
            requires(hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                std::sentinel_for<Sent, FwdIter1>)
        // clang-format on
        static FwdIter2 invoke_default(
            FwdIter1 first, Sent last, FwdIter2 dest, Op op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                hpx::execution::sequenced_policy{}, first, last, dest,
                HPX_MOVE(op));
        }

        template <typename Rng, typename FwdIter2, typename Op>
        // clang-format off
             requires (
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        static FwdIter2 invoke_default(Rng&& rng, FwdIter2 dest, Op op)
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                dest, HPX_MOVE(op));
        }

        template <typename ExPolicy, typename FwdIter1, typename Sent,
            typename FwdIter2, typename Op>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                std::sentinel_for<Sent, FwdIter1>
            )
        // clang-format on
        static decltype(auto) invoke_default(
            ExPolicy&& policy, FwdIter1 first, Sent last, FwdIter2 dest, Op op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, dest, HPX_MOVE(op));
        }

        template <typename ExPolicy, typename Rng, typename FwdIter2,
            typename Op>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        static decltype(auto) invoke_default(
            ExPolicy&& policy, Rng&& rng, FwdIter2 dest, Op op)
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::end(rng), dest, HPX_MOVE(op));
        }
    } adjacent_difference{};
}    // namespace hpx::ranges

#endif
