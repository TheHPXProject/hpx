//  Copyright (c) 2017 Taeguk Kwon
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/unique.hpp
/// \page hpx::ranges::unique, hpx::ranges::unique_copy
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Eliminates all but the first element from every consecutive group of
    /// equivalent elements from the range [first, last) and returns a
    /// past-the-end iterator for the new logical end of the range.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first - 1 applications of
    ///         the predicate \a pred and no more than twice as many
    ///         applications of the projection \a proj.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a unique requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is an
    ///                     binary predicate which returns \a true for the
    ///                     required elements. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be
    ///                     such that objects of types \a FwdIter can be
    ///                     dereferenced and then implicitly converted to
    ///                     both \a Type1 and \a Type2
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a unique algorithm invoked without
    /// an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a unique algorithm returns \a subrange_t<FwdIter, Sent>.
    ///           The \a unique algorithm returns an object {ret, last},
    ///           where ret is a past-the-end iterator for a new
    ///           subrange.
    ///
    template <typename FwdIter, typename Sent,
        typename Pred = ranges::equal_to,
        typename Proj = hpx::identity>
    subrange_t<FwdIter, Sent> unique(FwdIter first, Sent last,
        Pred&& pred = Pred(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Eliminates all but the first element from every consecutive group of
    /// equivalent elements from the range \a rng and returns a
    /// past-the-end iterator for the new logical end of the range.
    ///
    /// \note   Complexity: Performs not more than N assignments,
    ///         exactly N - 1 applications of the predicate \a pred and
    ///         no more than twice as many applications of the projection
    ///         \a proj, where N = std::distance(begin(rng), end(rng)).
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a unique requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is an
    ///                     binary predicate which returns \a true for the
    ///                     required elements. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a unique algorithm invoked without
    /// an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a unique algorithm returns
    ///           \a subrange_t<std::ranges::iterator_t<Rng>,
    ///           std::ranges::iterator_t<Rng>>.
    ///           The \a unique algorithm returns an object {ret, last},
    ///           where ret is a past-the-end iterator for a new
    ///           subrange.
    ///
    template <typename Rng,
        typename Pred = ranges::equal_to,
        typename Proj = hpx::identity>
    subrange_t<std::ranges::iterator_t<Rng>,
        std::ranges::iterator_t<Rng>>
    unique(Rng&& rng, Pred&& pred = Pred(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Copies the elements from the range [first, last),
    /// to another range beginning at \a dest in such a way that
    /// there are no consecutive equal elements. Only the first element of
    /// each group of equal elements is copied.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first - 1 applications of
    ///         the predicate \a pred and no more than twice as many
    ///         applications of the projection \a proj
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a unique_copy requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is an
    ///                     binary predicate which returns \a true for the
    ///                     required elements. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a InIter1 can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a unique_copy algorithm invoked
    /// without an execution policy object  will execute in sequential
    /// order in the calling thread.
    ///
    /// \returns  The \a unique_copy algorithm returns a
    ///           returns unique_copy_result<InIter, O>.
    ///           The \a unique_copy algorithm returns an in_out_result with
    ///           the source iterator to one past the last element and out
    ///           containing the destination iterator to the end of the
    ///           \a dest range.
    ///
    template <typename InIter, typename Sent, typename O,
        typename Pred = ranges::equal_to,
        typename Proj = hpx::identity>
    unique_copy_result<InIter, O> unique_copy(InIter first,
        Sent last, O dest, Pred&& pred = Pred(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Copies the elements from the range [first, last),
    /// to another range beginning at \a dest in such a way that
    /// there are no consecutive equal elements. Only the first element of
    /// each group of equal elements is copied.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first - 1 applications of
    ///         the predicate \a pred and no more than twice as many
    ///         applications of the projection \a proj
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter1.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a unique_copy requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is an
    ///                     binary predicate which returns \a true for the
    ///                     required elements. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a unique_copy algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a unique_copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a unique_copy algorithm returns returns a hpx::future<
    ///           unique_copy_result<FwdIter, O>> if the
    ///           execution policy is of type \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a
    ///           unique_copy_result<FwdIter, O> otherwise.
    ///           The \a unique_copy algorithm returns an in_out_result with
    ///           the source iterator to one past the last element and out
    ///           containing the destination iterator to the end of the
    ///           \a dest range.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent,
        typename O,
        typename Pred = ranges::equal_to,
        typename Proj = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        unique_copy_result<FwdIter, O>>::type
    unique_copy(ExPolicy&& policy, FwdIter first, Sent last,
        O dest, Pred&& pred = Pred(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Copies the elements from the range \a rng,
    /// to another range beginning at \a dest in such a way that
    /// there are no consecutive equal elements. Only the first element of
    /// each group of equal elements is copied.
    ///
    /// \note   Complexity: Performs not more than N assignments,
    ///         exactly N - 1 applications of the predicate \a pred,
    ///         where N = std::distance(begin(rng), end(rng)).
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a unique_copy requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by the range \a rng. This is an
    ///                     binary predicate which returns \a true for the
    ///                     required elements. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a unique_copy algorithm invoked
    /// without an execution policy object  will execute in sequential
    /// order in the calling thread.
    ///
    /// \returns  The \a unique_copy algorithm returns \a
    ///           unique_copy_result<
    ///           std::ranges::iterator_t<Rng>, O>.
    ///           The \a unique_copy algorithm returns the pair of
    ///           the source iterator to \a last, and
    ///           the destination iterator to the end of the \a dest range.
    ///
    template <typename Rng, typename O,
        typename Pred = ranges::equal_to,
        typename Proj = hpx::identity>
    unique_copy_result<std::ranges::iterator_t<Rng>, O>
    unique_copy(Rng&& rng, O dest, Pred&& pred = Pred(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Copies the elements from the range \a rng,
    /// to another range beginning at \a dest in such a way that
    /// there are no consecutive equal elements. Only the first element of
    /// each group of equal elements is copied.
    ///
    /// \note   Complexity: Performs not more than N assignments,
    ///         exactly N - 1 applications of the predicate \a pred,
    ///         where N = std::distance(begin(rng), end(rng)).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a unique_copy requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by the range \a rng. This is an
    ///                     binary predicate which returns \a true for the
    ///                     required elements. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a unique_copy algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a unique_copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a unique_copy algorithm returns a
    ///           \a hpx::future<unique_copy_result<
    ///           std::ranges::iterator_t<Rng>, O>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a unique_copy_result<
    ///           std::ranges::iterator_t<Rng>, O>
    ///           otherwise.
    ///           The \a unique_copy algorithm returns the pair of
    ///           the source iterator to \a last, and
    ///           the destination iterator to the end of the \a dest range.
    ///
    template <typename ExPolicy, typename Rng, typename O,
        typename Pred = ranges::equal_to,
        typename Proj = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        unique_copy_result<std::ranges::iterator_t<Rng>, O>>
    unique_copy(ExPolicy&& policy, Rng&& rng, O dest,
        Pred&& pred = Pred(), Proj&& proj = Proj());

    // clang-format on

    /// \brief Execution-policy overload of \c unique.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename Pred = std::ranges::equal_to,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> && std::permutable<I> &&
        std::indirect_equivalence_relation<Pred, std::projected<I, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I>>
    unique(ExPolicy&& policy, I first, S last, Pred pred = {}, Proj proj = {});

    /// \brief Execution-policy overload of \c unique.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        typename Pred = std::ranges::equal_to, typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::permutable<std::ranges::iterator_t<R>> &&
        std::indirect_equivalence_relation<Pred,
            std::projected<std::ranges::iterator_t<R>, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R>>
    unique(ExPolicy&& policy, R&& rng, Pred pred = {}, Proj proj = {});

    /// \brief Execution-policy overload of \c unique_copy.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// The operation is bounded by the supplied output range(s). Returned
    /// input positions identify where processing can resume.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, std::random_access_iterator O,
        std::sized_sentinel_for<O> OutS, typename Pred = std::ranges::equal_to,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_copyable<I, O> &&
        std::indirect_equivalence_relation<Pred, std::projected<I, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        unique_copy_result<I, O>>
    unique_copy(ExPolicy&& policy, I first, S last, O dest, OutS dest_last,
        Pred pred = {}, Proj proj = {});

    /// \brief Execution-policy overload of \c unique_copy.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// The operation is bounded by the supplied output range(s). Returned
    /// input positions identify where processing can resume.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        std::ranges::random_access_range OutR,
        typename Pred = std::ranges::equal_to, typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> && std::ranges::sized_range<OutR> &&
        std::indirectly_copyable<std::ranges::iterator_t<R>,
            std::ranges::iterator_t<OutR>> &&
        std::indirect_equivalence_relation<Pred,
            std::projected<std::ranges::iterator_t<R>, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        unique_copy_result<std::ranges::borrowed_iterator_t<R>,
            std::ranges::borrowed_iterator_t<OutR>>>
    unique_copy(ExPolicy&& policy, R&& rng, OutR&& output, Pred pred = {},
        Proj proj = {});
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/bounded_copy.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/unique.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename I, typename S>
    using subrange_t = hpx::util::iterator_range<I, S>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::unique
    HPX_CXX_CORE_EXPORT inline constexpr struct unique_t final
      : hpx::detail::tag_dispatch<unique_t,
            hpx::detail::tag_parallel_algorithm<unique_t>>
    {
        template <typename FwdIter, typename Sent,
            typename Pred = ranges::equal_to, typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                parallel::traits::is_projected_v<Proj, FwdIter> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    parallel::traits::projected<Proj, FwdIter>,
                    parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        static subrange_t<FwdIter, Sent> invoke_default(
            FwdIter first, Sent last, Pred pred = Pred(), Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::util::make_subrange<FwdIter, Sent>(
                hpx::parallel::detail::unique<FwdIter>().call(
                    hpx::execution::seq, first, last, HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                last);
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Pred = std::ranges::equal_to,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::permutable<I> &&
            std::indirect_equivalence_relation<Pred, std::projected<I, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, Pred pred = {}, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::util::detail::convert_to_result(
                parallel::detail::unique<I>().call(
                    HPX_FORWARD(ExPolicy, policy), first, end, HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                [end](I position) -> std::ranges::subrange<I> {
                    return {position, end};
                });
        }

        template <typename Rng, typename Pred = ranges::equal_to,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static subrange_t<std::ranges::iterator_t<Rng>,
            std::ranges::sentinel_t<Rng>>
        invoke_default(Rng&& rng, Pred pred = Pred(), Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least input iterator.");

            return hpx::parallel::util::make_subrange<
                std::ranges::iterator_t<Rng>, std::ranges::sentinel_t<Rng>>(
                hpx::parallel::detail::unique<std::ranges::iterator_t<Rng>>()
                    .call(hpx::execution::seq, hpx::util::begin(rng),
                        hpx::util::end(rng), HPX_MOVE(pred), HPX_MOVE(proj)),
                hpx::util::end(rng));
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Pred = std::ranges::equal_to,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::permutable<std::ranges::iterator_t<R>> &&
            std::indirect_equivalence_relation<Pred,
                std::projected<std::ranges::iterator_t<R>, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, Pred pred = {}, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                [](std::ranges::subrange<I> result)
                    -> std::ranges::borrowed_subrange_t<R> { return result; });
        }
    } unique{};

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using unique_copy_result = parallel::util::in_out_result<I, O>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::unique_copy
    HPX_CXX_CORE_EXPORT inline constexpr struct unique_copy_t final
      : hpx::detail::tag_dispatch<unique_copy_t,
            hpx::detail::tag_parallel_algorithm<unique_copy_t>>
    {
        template <typename InIter, typename Sent, typename O,
            typename Pred = ranges::equal_to, typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<InIter> &&
                std::sentinel_for<Sent, InIter> &&
                parallel::traits::is_projected_v<Proj, InIter> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    parallel::traits::projected<Proj, InIter>,
                    parallel::traits::projected<Proj, InIter>
                >
            )
        // clang-format on
        static unique_copy_result<InIter, O> invoke_default(InIter first,
            Sent last, O dest, Pred pred = Pred(), Proj proj = Proj())
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            using result_type = unique_copy_result<InIter, O>;

            return hpx::parallel::detail::unique_copy<result_type>().call(
                hpx::execution::seq, first, last, dest, HPX_MOVE(pred),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename FwdIter, typename Sent,
            typename O, typename Pred = ranges::equal_to,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                parallel::traits::is_projected_v<Proj, FwdIter> &&
                parallel::traits::is_indirect_callable_v<
                    ExPolicy, Pred,
                    parallel::traits::projected<Proj, FwdIter>,
                    parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            unique_copy_result<FwdIter, O>>
        invoke_default(ExPolicy&& policy, FwdIter first, Sent last, O dest,
            Pred pred = Pred(), Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            using result_type = unique_copy_result<FwdIter, O>;

            return hpx::parallel::detail::unique_copy<result_type>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, dest,
                HPX_MOVE(pred), HPX_MOVE(proj));
        }

        template <typename Rng, typename O, typename Pred = ranges::equal_to,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                std::input_or_output_iterator<O> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static unique_copy_result<std::ranges::iterator_t<Rng>, O>
        invoke_default(
            Rng&& rng, O dest, Pred pred = Pred(), Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::input_iterator<iterator_type>,
                "Requires at least input iterator.");

            using result_type = unique_copy_result<iterator_type, O>;

            return hpx::parallel::detail::unique_copy<result_type>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                dest, HPX_MOVE(pred), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng, typename O,
            typename Pred = ranges::equal_to, typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                std::input_or_output_iterator<O> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, Pred,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            unique_copy_result<std::ranges::iterator_t<Rng>, O>>
        invoke_default(ExPolicy&& policy, Rng&& rng, O dest, Pred pred = Pred(),
            Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least input iterator.");

            using result_type = unique_copy_result<iterator_type, O>;

            return hpx::parallel::detail::unique_copy<result_type>().call(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::end(rng), dest, HPX_MOVE(pred), HPX_MOVE(proj));
        }

        /// \brief Copy selected elements into a bounded destination.
        /// \returns The first selected input that did not fit and the output
        /// end position, or their future. If all selected elements fit, the
        /// input position is last.
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS,
            typename Pred = std::ranges::equal_to,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_copyable<I, O> &&
            std::indirect_equivalence_relation<Pred, std::projected<I, Proj>>
        static decltype(auto) invoke_default(ExPolicy&& policy, I first, S last,
            O dest, OutS dest_last, Pred pred = {}, Proj proj = {})
        {
            auto select = [first, pred = HPX_MOVE(pred), proj = HPX_MOVE(proj)](
                              I current) mutable {
                return current == first ||
                    !HPX_INVOKE(pred, HPX_INVOKE(proj, *(current - 1)),
                        HPX_INVOKE(proj, *current));
            };
            return parallel::detail::bounded_copy_selected<I, O>().call(
                HPX_FORWARD(ExPolicy, policy), first, first + (last - first),
                dest, dest + (dest_last - dest), HPX_MOVE(select));
        }

        /// \brief Copy selected elements between bounded ranges.
        /// \returns Borrowed resume positions, or their future.
        template <typename ExPolicy, std::ranges::random_access_range R,
            std::ranges::random_access_range OutR,
            typename Pred = std::ranges::equal_to,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> && std::ranges::sized_range<OutR> &&
            std::indirectly_copyable<std::ranges::iterator_t<R>,
                std::ranges::iterator_t<OutR>> &&
            std::indirect_equivalence_relation<Pred,
                std::projected<std::ranges::iterator_t<R>, Proj>>
        static decltype(auto) invoke_default(ExPolicy&& policy, R&& rng,
            OutR&& output, Pred pred = {}, Proj proj = {})
        {
            using iterator_result =
                unique_copy_result<std::ranges::iterator_t<R>,
                    std::ranges::iterator_t<OutR>>;
            using result_type =
                unique_copy_result<std::ranges::borrowed_iterator_t<R>,
                    std::ranges::borrowed_iterator_t<OutR>>;
            auto first = std::ranges::begin(rng);
            auto dest = std::ranges::begin(output);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), dest,
                    dest + std::ranges::distance(output), HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                [](iterator_result result) -> result_type {
                    return {result.in, result.out};
                });
        }
    } unique_copy{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
