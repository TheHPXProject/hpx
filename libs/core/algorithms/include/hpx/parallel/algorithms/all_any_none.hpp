//  Copyright (c) 2022 Bhumit Attarde
//  Copyright (c) 2014-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/all_any_none.hpp
/// \page hpx::all_of, hpx::any_of, hpx::none_of
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    ///  Checks if unary predicate \a f returns true for no elements in the
    ///  range [first, last).
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         predicate \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a none_of requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed
    ///                     to it. The type \a Type must be such that an object
    ///                     of type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a none_of algorithm returns a \a hpx::future<bool> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a bool
    ///           otherwise.
    ///           The \a none_of algorithm returns true if the unary predicate
    ///           \a f returns true for no elements in the range, false
    ///           otherwise. It returns true if the range is empty.
    ///
    template <typename ExPolicy, typename FwdIter, typename F>
    util::detail::algorithm_result_t<ExPolicy, bool>
    none_of(ExPolicy&& policy, FwdIter first, FwdIter last, F&& f);

    ///  Checks if unary predicate \a f returns true for no elements in the
    ///  range [first, last).
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         predicate \a f
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a none_of requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed
    ///                     to it. The type \a Type must be such that an object
    ///                     of type \a InIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// \returns  The \a none_of algorithm returns a \a bool .
    ///           The \a none_of algorithm returns true if the unary predicate
    ///           \a f returns true for no elements in the range, false
    ///           otherwise. It returns true if the range is empty.
    ///
    template <typename InIter, typename F>
    bool none_of(InIter first, InIter last, F&& f);

    ///  Checks if unary predicate \a f returns true for at least one element
    ///  in the range [first, last).
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         predicate \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a any_of requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed
    ///                     to it. The type \a Type must be such that an object
    ///                     of type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a any_of algorithm returns a \a hpx::future<bool> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a bool otherwise.
    ///           The \a any_of algorithm returns true if the unary predicate
    ///           \a f returns true for at least one element in the range,
    ///           false otherwise. It returns false if the range is empty.
    ///
    template <typename ExPolicy, typename FwdIter, typename F>
    util::detail::algorithm_result_t<ExPolicy, bool>
    any_of(ExPolicy&& policy, FwdIter first, FwdIter last, F&& f);

    ///  Checks if unary predicate \a f returns true for at least one element
    ///  in the range [first, last).
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         predicate \a f
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a any_of requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed
    ///                     to it. The type \a Type must be such that an object
    ///                     of type \a InIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// \returns  The \a any_of algorithm returns a \a bool .
    ///           The \a any_of algorithm returns true if the unary predicate
    ///           \a f returns true for at least one element in the range,
    ///           false otherwise. It returns false if the range is empty.
    ///
    template <typename InIter, typename F>
    bool any_of(InIter first, InIter last, F&& f);

    /// Checks if unary predicate \a f returns true for all elements in the
    /// range [first, last).
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         predicate \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a all_of requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed
    ///                     to it. The type \a Type must be such that an object
    ///                     of type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a all_of algorithm returns a \a hpx::future<bool> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a bool otherwise.
    ///           The \a all_of algorithm returns true if the unary predicate
    ///           \a f returns true for all elements in the range, false
    ///           otherwise. It returns true if the range is empty.
    ///
    template <typename ExPolicy, typename FwdIter, typename F>
    util::detail::algorithm_result_t<ExPolicy, bool>
    all_of(ExPolicy&& policy, FwdIter first, FwdIter last, F&& f);

    /// Checks if unary predicate \a f returns true for all elements in the
    /// range [first, last).
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         predicate \a f
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a all_of requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed
    ///                     to it. The type \a Type must be such that an object
    ///                     of type \a InIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// \returns  The \a all_of algorithm returns a \a bool .
    ///           The \a all_of algorithm returns true if the unary predicate
    ///           \a f returns true for all elements in the range, false
    ///           otherwise. It returns true if the range is empty.
    ///
    template <typename ExPolicy, typename InIter, typename F>
    bool all_of(InIter first, InIter last, F&& f);

    // clang-format on
}    // namespace hpx
#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/find.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/invoke_projected.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // none_of
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT struct none_of : public algorithm<none_of, bool>
        {
            constexpr none_of() noexcept
              : algorithm<none_of, bool>("none_of")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename F, typename Proj>
            static constexpr bool sequential(
                ExPolicy, Iter first, Sent last, F&& f, Proj&& proj)
            {
                return detail::sequential_find_if<ExPolicy>(first, last,
                           util::invoke_projected<F, Proj>(HPX_FORWARD(F, f),
                               HPX_FORWARD(Proj, proj))) == last;
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename F, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter first,
                Sent last, F&& op, Proj&& proj)
            {
                using result = util::detail::algorithm_result<ExPolicy, bool>;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return result::get(true);
                    }
                }

                using policy_type = std::decay_t<ExPolicy>;
                using intermediate_result_t = std::uint8_t;

                util::cancellation_token<> tok;
                auto f1 = [op = HPX_FORWARD(F, op), tok,
                              proj = HPX_FORWARD(Proj, proj)](
                              FwdIter part_begin,
                              std::size_t part_count) mutable
                    -> intermediate_result_t {
                    detail::sequential_find_if<policy_type>(part_begin,
                        part_count, tok, HPX_FORWARD(F, op),
                        HPX_FORWARD(Proj, proj));

                    return !tok.was_cancelled();
                };

                auto f2 = [](auto&& results) {
                    return detail::sequential_find_if_not<
                               hpx::execution::sequenced_policy>(
                               hpx::util::begin(results),
                               hpx::util::end(results),
                               hpx::functional::unwrap{}) ==
                        hpx::util::end(results);
                };

                return util::partitioner<policy_type, bool,
                    intermediate_result_t>::call(HPX_FORWARD(decltype(policy),
                                                     policy),
                    first, detail::distance(first, last), HPX_MOVE(f1),
                    HPX_MOVE(f2));
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // any_of
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT struct any_of : public algorithm<any_of, bool>
        {
            constexpr any_of() noexcept
              : algorithm<any_of, bool>("any_of")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename F, typename Proj>
            static bool sequential(
                ExPolicy, Iter first, Sent last, F&& f, Proj&& proj)
            {
                return detail::sequential_find_if<ExPolicy>(first, last,
                           util::invoke_projected<F, Proj>(HPX_FORWARD(F, f),
                               HPX_FORWARD(Proj, proj))) != last;
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename F, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter first,
                Sent last, F&& op, Proj&& proj)
            {
                using result = util::detail::algorithm_result<ExPolicy, bool>;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return result::get(true);
                    }
                }

                using policy_type = std::decay_t<ExPolicy>;
                using intermediate_result_t = std::uint8_t;

                util::cancellation_token<> tok;
                auto f1 = [op = HPX_FORWARD(F, op), tok,
                              proj = HPX_FORWARD(Proj, proj)](
                              FwdIter part_begin,
                              std::size_t part_count) mutable
                    -> intermediate_result_t {
                    detail::sequential_find_if<policy_type>(part_begin,
                        part_count, tok, HPX_FORWARD(F, op),
                        HPX_FORWARD(Proj, proj));

                    return tok.was_cancelled();
                };

                auto f2 = [](auto&& results) {
                    return detail::sequential_find_if<
                               hpx::execution::sequenced_policy>(
                               hpx::util::begin(results),
                               hpx::util::end(results),
                               hpx::functional::unwrap{}) !=
                        hpx::util::end(results);
                };

                return util::partitioner<policy_type, bool,
                    intermediate_result_t>::call(HPX_FORWARD(decltype(policy),
                                                     policy),
                    first, detail::distance(first, last), HPX_MOVE(f1),
                    HPX_MOVE(f2));
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // all_of
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT struct all_of : public algorithm<all_of, bool>
        {
            constexpr all_of() noexcept
              : algorithm("all_of")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename F, typename Proj>
            static bool sequential(
                ExPolicy, Iter first, Sent last, F&& f, Proj&& proj)
            {
                return detail::sequential_find_if_not<ExPolicy>(first, last,
                           HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj)) == last;
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename F, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter first,
                Sent last, F&& op, Proj&& proj)
            {
                using result = util::detail::algorithm_result<ExPolicy, bool>;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return result::get(true);
                    }
                }

                using policy_type = std::decay_t<ExPolicy>;
                using intermediate_result_t = std::uint8_t;

                util::cancellation_token<> tok;
                auto f1 = [op = HPX_FORWARD(F, op), tok,
                              proj = HPX_FORWARD(Proj, proj)](
                              FwdIter part_begin,
                              std::size_t part_count) mutable
                    -> intermediate_result_t {
                    detail::sequential_find_if_not<policy_type>(part_begin,
                        part_count, tok, HPX_FORWARD(F, op),
                        HPX_FORWARD(Proj, proj));

                    return !tok.was_cancelled();
                };

                auto f2 = [](auto&& results) {
                    return detail::sequential_find_if_not<
                               hpx::execution::sequenced_policy>(
                               hpx::util::begin(results),
                               hpx::util::end(results),
                               hpx::functional::unwrap{}) ==
                        hpx::util::end(results);
                };

                return util::partitioner<policy_type, bool,
                    intermediate_result_t>::call(HPX_FORWARD(decltype(policy),
                                                     policy),
                    first, detail::distance(first, last), HPX_MOVE(f1),
                    HPX_MOVE(f2));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::none_of
    HPX_CXX_CORE_EXPORT inline constexpr struct none_of_t final
      : hpx::detail::tag_parallel_algorithm<none_of_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter, typename F>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(
            none_of_t, ExPolicy&& policy, FwdIter first, FwdIter last, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::none_of().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                hpx::identity_v);
        }

        template <typename InIter, typename F>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter>
            )
        // clang-format on
        friend bool tag_fallback_invoke(
            none_of_t, InIter first, InIter last, F f)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");

            return hpx::parallel::detail::none_of().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), hpx::identity_v);
        }
    } none_of{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::any_of
    HPX_CXX_CORE_EXPORT inline constexpr struct any_of_t final
      : hpx::detail::tag_parallel_algorithm<any_of_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter, typename F>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(
            any_of_t, ExPolicy&& policy, FwdIter first, FwdIter last, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::any_of().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                hpx::identity_v);
        }

        template <typename InIter, typename F>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter>
            )
        // clang-format on
        friend bool tag_fallback_invoke(
            any_of_t, InIter first, InIter last, F f)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");

            return hpx::parallel::detail::any_of().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), hpx::identity_v);
        }
    } any_of{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::all_of
    HPX_CXX_CORE_EXPORT inline constexpr struct all_of_t final
      : hpx::detail::tag_parallel_algorithm<all_of_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter, typename F>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(
            all_of_t, ExPolicy&& policy, FwdIter first, FwdIter last, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::all_of().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                hpx::identity_v);
        }

        template <typename InIter, typename F>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter>
            )
        // clang-format on
        friend bool tag_fallback_invoke(
            all_of_t, InIter first, InIter last, F f)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");

            return hpx::parallel::detail::all_of().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), hpx::identity_v);
        }
    } all_of{};
}    // namespace hpx

#endif    // DOXYGEN
