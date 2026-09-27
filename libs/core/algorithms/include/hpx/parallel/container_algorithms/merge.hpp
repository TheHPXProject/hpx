//  Copyright (c) 2017 Taeguk Kwon
//  Copyright (c) 2022 Dimitra Karatza
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/merge.hpp
/// \page hpx::ranges::merge, hpx::ranges::inplace_merge
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
    // clang-format off

    /// Merges two sorted ranges [first1, last1) and [first2, last2)
    /// into one sorted range beginning at \a dest. The order of
    /// equivalent elements in the each of original two ranges is preserved.
    /// For equivalent elements in the original two ranges, the elements from
    /// the first range precede the elements from the second range.
    /// The destination range cannot overlap with either of the input ranges.
    ///
    /// \note   Complexity: Performs
    ///         O(std::distance(first1, last1) + std::distance(first2, last2))
    ///         applications of the comparison \a comp and the each projection.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng1        The type of the first source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an random access iterator.
    /// \tparam Rng2        The type of the second source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an random access iterator.
    /// \tparam Iter3       The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a merge requires \a Comp to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::less<>
    /// \tparam Proj1       The type of an optional projection function to be
    ///                     used for elements of the first range. This defaults
    ///                     to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function to be
    ///                     used for elements of the second range. This defaults
    ///                     to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng1         Refers to the first range of elements the algorithm
    ///                     will be applied to.
    /// \param rng2         Refers to the second range of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param comp         \a comp is a callable object which returns true if
    ///                     the first argument is less than the second,
    ///                     and false otherwise. The signature of this
    ///                     comparison should be equivalent to:
    ///                     \code
    ///                     bool comp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such that
    ///                     objects of types \a Iter1 and \a Iter2 can be
    ///                     dereferenced and then implicitly converted to
    ///                     both \a Type1 and \a Type2
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     first range as a projection operation before the
    ///                     actual comparison \a comp is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     second range as a projection operation before the
    ///                     actual comparison \a comp is invoked.
    ///
    /// The assignments in the parallel \a merge algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a merge algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a merge algorithm returns a
    /// \a hpx::future<merge_result<Iter1, Iter2, Iter3>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns
    ///           \a merge_result<Iter1, Iter2, Iter3> otherwise.
    ///           The \a merge algorithm returns the tuple of
    ///           the source iterator \a last1,
    ///           the source iterator \a last2,
    ///           the destination iterator to the end of the \a dest range.
    ///
    template <typename ExPolicy, typename Rng1, typename Rng2,
        typename Iter3, typename Comp = hpx::ranges::less,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        hpx::ranges::merge_result<
            std::ranges::iterator_t<Rng1>,
            std::ranges::iterator_t<Rng2>, Iter3>>
    merge(ExPolicy&& policy, Rng1&& rng1, Rng2&& rng2, Iter3 dest,
        Comp&& comp = Comp(), Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    /// Merges two sorted ranges [first1, last1) and [first2, last2)
    /// into one sorted range beginning at \a dest. The order of
    /// equivalent elements in the each of original two ranges is preserved.
    /// For equivalent elements in the original two ranges, the elements from
    /// the first range precede the elements from the second range.
    /// The destination range cannot overlap with either of the input ranges.
    ///
    /// \note   Complexity: Performs
    ///         O(std::distance(first1, last1) + std::distance(first2, last2))
    ///         applications of the comparison \a comp and the each projection.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Iter1       The type of the source iterators used (deduced)
    ///                     representing the first sequence.
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Sent1       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter1.
    /// \tparam Iter2       The type of the source iterators used (deduced)
    ///                     representing the second sequence.
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Sent2       The type of the end source iterators used (deduced)
    ///                     representing the second sequence.
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter2.
    /// \tparam Iter3       The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a merge requires \a Comp to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::less<>
    /// \tparam Proj1       The type of an optional projection function to be
    ///                     used for elements of the first range. This defaults
    ///                     to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function to be
    ///                     used for elements of the second range. This defaults
    ///                     to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last1        Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
    /// \param last2        Refers to the end of the sequence of elements of
    ///                     the second range the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param comp         \a comp is a callable object which returns true if
    ///                     the first argument is less than the second,
    ///                     and false otherwise. The signature of this
    ///                     comparison should be equivalent to:
    ///                     \code
    ///                     bool comp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such that
    ///                     objects of types \a Iter1 and \a Iter2 can be
    ///                     dereferenced and then implicitly converted to
    ///                     both \a Type1 and \a Type2
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     first range as a projection operation before the
    ///                     actual comparison \a comp is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     second range as a projection operation before the
    ///                     actual comparison \a comp is invoked.
    ///
    /// The assignments in the parallel \a merge algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a merge algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a merge algorithm returns a
    /// \a hpx::future<merge_result<Iter1, Iter2, Iter3>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns
    ///           \a merge_result<Iter1, Iter2, Iter3> otherwise.
    ///           The \a merge algorithm returns the tuple of
    ///           the source iterator \a last1,
    ///           the source iterator \a last2,
    ///           the destination iterator to the end of the \a dest range.
    ///
    template <typename ExPolicy, typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Iter3,
        typename Comp = hpx::ranges::less,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        hpx::ranges::merge_result<Iter1, Iter2, Iter3>>::type
    merge(ExPolicy&& policy, Iter1 first1, Sent1 last1, Iter2 first2,
        Sent2 last2, Iter3 dest, Comp&& comp = Comp(), Proj1&& proj1 = Proj1(),
        Proj2&& proj2 = Proj2());

    /// Merges two sorted ranges [first1, last1) and [first2, last2)
    /// into one sorted range beginning at \a dest. The order of
    /// equivalent elements in the each of original two ranges is preserved.
    /// For equivalent elements in the original two ranges, the elements from
    /// the first range precede the elements from the second range.
    /// The destination range cannot overlap with either of the input ranges.
    ///
    /// \note   Complexity: Performs
    ///         O(std::distance(first1, last1) + std::distance(first2, last2))
    ///         applications of the comparison \a comp and the each projection.
    ///
    /// \tparam Rng1        The type of the first source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an random access iterator.
    /// \tparam Rng2        The type of the second source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an random access iterator.
    /// \tparam Iter3       The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a merge requires \a Comp to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::less<>
    /// \tparam Proj1       The type of an optional projection function to be
    ///                     used for elements of the first range. This defaults
    ///                     to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function to be
    ///                     used for elements of the second range. This defaults
    ///                     to \a hpx::identity
    ///
    /// \param rng1         Refers to the first range of elements the algorithm
    ///                     will be applied to.
    /// \param rng2         Refers to the second range of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param comp         \a comp is a callable object which returns true if
    ///                     the first argument is less than the second,
    ///                     and false otherwise. The signature of this
    ///                     comparison should be equivalent to:
    ///                     \code
    ///                     bool comp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such that
    ///                     objects of types \a Iter1 and \a Iter2 can be
    ///                     dereferenced and then implicitly converted to
    ///                     both \a Type1 and \a Type2
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     first range as a projection operation before the
    ///                     actual comparison \a comp is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     second range as a projection operation before the
    ///                     actual comparison \a comp is invoked.
    ///
    /// \returns  The \a merge algorithm returns
    ///           \a merge_result<Iter1, Iter2, Iter3>.
    ///           The \a merge algorithm returns the tuple of
    ///           the source iterator \a last1,
    ///           the source iterator \a last2,
    ///           the destination iterator to the end of the \a dest range.
    ///
    template <typename Rng1, typename Rng2,
        typename Iter3, typename Comp = hpx::ranges::less,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    hpx::ranges::merge_result<
        std::ranges::iterator_t<Rng1>,
        std::ranges::iterator_t<Rng2>, Iter3>
    merge(Rng1&& rng1, Rng2&& rng2, Iter3 dest, Comp&& comp = Comp(),
        Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    /// Merges two sorted ranges [first1, last1) and [first2, last2)
    /// into one sorted range beginning at \a dest. The order of
    /// equivalent elements in the each of original two ranges is preserved.
    /// For equivalent elements in the original two ranges, the elements from
    /// the first range precede the elements from the second range.
    /// The destination range cannot overlap with either of the input ranges.
    ///
    /// \note   Complexity: Performs
    ///         O(std::distance(first1, last1) + std::distance(first2, last2))
    ///         applications of the comparison \a comp and the each projection.
    ///
    /// \tparam Iter1       The type of the source iterators used (deduced)
    ///                     representing the first sequence.
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Sent1       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter1.
    /// \tparam Iter2       The type of the source iterators used (deduced)
    ///                     representing the second sequence.
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Sent2       The type of the end source iterators used (deduced)
    ///                     representing the second sequence.
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter2.
    /// \tparam Iter3       The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a merge requires \a Comp to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::less<>
    /// \tparam Proj1       The type of an optional projection function to be
    ///                     used for elements of the first range. This defaults
    ///                     to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function to be
    ///                     used for elements of the second range. This defaults
    ///                     to \a hpx::identity
    ///
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last1        Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
    /// \param last2        Refers to the end of the sequence of elements of
    ///                     the second range the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param comp         \a comp is a callable object which returns true if
    ///                     the first argument is less than the second,
    ///                     and false otherwise. The signature of this
    ///                     comparison should be equivalent to:
    ///                     \code
    ///                     bool comp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such that
    ///                     objects of types \a Iter1 and \a Iter2 can be
    ///                     dereferenced and then implicitly converted to
    ///                     both \a Type1 and \a Type2
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     first range as a projection operation before the
    ///                     actual comparison \a comp is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     second range as a projection operation before the
    ///                     actual comparison \a comp is invoked.
    ///
    /// \returns  The \a merge algorithm returns
    ///           \a merge_result<Iter1, Iter2, Iter3>.
    ///           The \a merge algorithm returns the tuple of
    ///           the source iterator \a last1,
    ///           the source iterator \a last2,
    ///           the destination iterator to the end of the \a dest range.
    ///
    template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Iter3,
        typename Comp = hpx::ranges::less,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    hpx::ranges::merge_result<Iter1, Iter2, Iter3>
    merge(Iter1 first1, Sent1 last1, Iter2 first2,
        Sent2 last2, Iter3 dest, Comp&& comp = Comp(), Proj1&& proj1 = Proj1(),
        Proj2&& proj2 = Proj2());

    /// Merges two consecutive sorted ranges [first, middle) and
    /// [middle, last) into one sorted range [first, last). The order of
    /// equivalent elements in the each of original two ranges is preserved.
    /// For equivalent elements in the original two ranges, the elements from
    /// the first range precede the elements from the second range.
    ///
    /// \note   Complexity: Performs O(std::distance(first, last))
    ///         applications of the comparison \a comp and the each projection.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an random access iterator.
    /// \tparam Iter        The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a inplace_merge requires \a Comp
    ///                     to meet the requirements of \a CopyConstructible.
    ///                     This defaults to std::less<>
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the range of elements the algorithm
    ///                     will be applied to.
    /// \param middle       Refers to the end of the first sorted range and
    ///                     the beginning of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param comp         \a comp is a callable object which returns true if
    ///                     the first argument is less than the second,
    ///                     and false otherwise. The signature of this
    ///                     comparison should be equivalent to:
    ///                     \code
    ///                     bool comp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be
    ///                     such that objects of types \a Iter can be
    ///                     dereferenced and then implicitly converted to both
    ///                     \a Type1 and \a Type2
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a inplace_merge algorithm returns \a Iter.
    ///           The \a inplace_merge algorithm returns
    ///           the source iterator \a last
    ///
    template <typename Rng, typename Iter,
        typename Comp = hpx::ranges::less,
        typename Proj = hpx::identity>
    Iter inplace_merge(Rng&& rng, Iter middle, Comp&& comp = Comp(),
        Proj&& proj = Proj());

    /// Merges two consecutive sorted ranges [first, middle) and
    /// [middle, last) into one sorted range [first, last). The order of
    /// equivalent elements in the each of original two ranges is preserved.
    /// For equivalent elements in the original two ranges, the elements from
    /// the first range precede the elements from the second range.
    ///
    /// \note   Complexity: Performs O(std::distance(first, last))
    ///         applications of the comparison \a comp and the each projection.
    ///
    /// \tparam Iter        The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Sent       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter1.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a inplace_merge requires \a Comp
    ///                     to meet the requirements of \a CopyConstructible.
    ///                     This defaults to std::less<>
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the first sorted range
    ///                     the algorithm will be applied to.
    /// \param middle       Refers to the end of the first sorted range and
    ///                     the beginning of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param comp         \a comp is a callable object which returns true if
    ///                     the first argument is less than the second,
    ///                     and false otherwise. The signature of this
    ///                     comparison should be equivalent to:
    ///                     \code
    ///                     bool comp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be
    ///                     such that objects of types \a Iter can be
    ///                     dereferenced and then implicitly converted to both
    ///                     \a Type1 and \a Type2
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a inplace_merge algorithm \a Iter.
    ///           The \a inplace_merge algorithm returns
    ///           the source iterator \a last
    ///
    template <typename Iter, typename Sent,
        typename Comp = hpx::ranges::less,
        typename Proj = hpx::identity>
    Iter inplace_merge(Iter first, Iter middle, Sent last, Comp&& comp = Comp(),
        Proj&& proj = Proj());

    // clang-format on

    /// \brief Execution-policy overload of \c merge.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// The operation is bounded by the supplied output range(s). Returned
    /// input positions identify where processing can resume.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I1,
        std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
        std::sized_sentinel_for<I2> S2, std::random_access_iterator O,
        std::sized_sentinel_for<O> OutS, typename Comp = std::ranges::less,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::mergeable<I1, I2, O, Comp, Proj1, Proj2>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        merge_result<I1, I2, O>>
    merge(ExPolicy&& policy, I1 first1, S1 last1, I2 first2, S2 last2, O dest,
        OutS dest_last, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {});

    /// \brief Execution-policy overload of \c merge.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// The operation is bounded by the supplied output range(s). Returned
    /// input positions identify where processing can resume.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R1,
        std::ranges::random_access_range R2,
        std::ranges::random_access_range OutR,
        typename Comp = std::ranges::less, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R1> && std::ranges::sized_range<R2> &&
        std::ranges::sized_range<OutR> &&
        std::mergeable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>,
            std::ranges::iterator_t<OutR>, Comp, Proj1, Proj2>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        merge_result<std::ranges::borrowed_iterator_t<R1>,
            std::ranges::borrowed_iterator_t<R2>,
            std::ranges::borrowed_iterator_t<OutR>>>
    merge(ExPolicy&& policy, R1&& rng1, R2&& rng2, OutR&& output,
        Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {});

    /// \brief Execution-policy overload of \c inplace_merge.
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
    inplace_merge(ExPolicy&& policy, R&& rng, std::ranges::iterator_t<R> middle,
        Comp comp = {}, Proj proj = {});

    /// \brief Execution-policy overload of \c inplace_merge.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename Comp = std::ranges::less,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::sortable<I, Comp, Proj>
    parallel::util::detail::algorithm_result_t<ExPolicy, I> inplace_merge(
        ExPolicy&& policy, I first, I middle, S last, Comp comp = {},
        Proj proj = {});
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/bounded_merge.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/merge.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT template <typename I1, typename I2, typename O>
    using merge_result = parallel::util::in_in_out_result<I1, I2, O>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::merge
    HPX_CXX_CORE_EXPORT inline constexpr struct merge_t final
      : hpx::detail::tag_dispatch<merge_t,
            hpx::detail::tag_parallel_algorithm<merge_t>>
    {
        template <typename ExPolicy, typename Rng1, typename Rng2,
            typename Iter3, typename Comp = hpx::ranges::less,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng1> &&
                hpx::parallel::traits::is_projected_range_v<Proj1, Rng1> &&
                std::ranges::range<Rng2> &&
                hpx::parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                hpx::traits::is_iterator_v<Iter3> &&
                hpx::parallel::traits::is_indirect_callable_v<ExPolicy, Comp,
                    hpx::parallel::traits::projected_range<Proj1, Rng1>,
                    hpx::parallel::traits::projected_range<Proj2, Rng2>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            hpx::ranges::merge_result<std::ranges::iterator_t<Rng1>,
                std::ranges::iterator_t<Rng2>, Iter3>>
        invoke_default(ExPolicy&& policy, Rng1&& rng1, Rng2&& rng2, Iter3 dest,
            Comp comp = Comp(), Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            using iterator_type1 = std::ranges::iterator_t<Rng1>;
            using iterator_type2 = std::ranges::iterator_t<Rng2>;

            static_assert(std::random_access_iterator<iterator_type1>,
                "Required at least random access iterator.");
            static_assert(std::random_access_iterator<iterator_type2>,
                "Requires at least random access iterator.");
            static_assert(std::random_access_iterator<Iter3>,
                "Requires at least random access iterator.");

            using result_type = hpx::ranges::merge_result<iterator_type1,
                iterator_type2, Iter3>;

            return hpx::parallel::detail::merge<result_type>().call(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng1),
                hpx::util::end(rng1), hpx::util::begin(rng2),
                hpx::util::end(rng2), dest, HPX_MOVE(comp), HPX_MOVE(proj1),
                HPX_MOVE(proj2));
        }

        template <typename ExPolicy, typename Iter1, typename Sent1,
            typename Iter2, typename Sent2, typename Iter3,
            typename Comp = hpx::ranges::less, typename Proj1 = hpx::identity,
            typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::sentinel_for<Sent1, Iter1> &&
                hpx::parallel::traits::is_projected_v<Proj1, Iter1> &&
                std::sentinel_for<Sent2, Iter2> &&
                hpx::parallel::traits::is_projected_v<Proj2, Iter2> &&
                hpx::traits::is_iterator_v<Iter3> &&
                hpx::parallel::traits::is_indirect_callable_v<ExPolicy, Comp,
                    hpx::parallel::traits::projected<Proj1, Iter1>,
                    hpx::parallel::traits::projected<Proj2, Iter2>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            hpx::ranges::merge_result<Iter1, Iter2, Iter3>>
        invoke_default(ExPolicy&& policy, Iter1 first1, Sent1 last1,
            Iter2 first2, Sent2 last2, Iter3 dest, Comp comp = Comp(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            static_assert(std::random_access_iterator<Iter1>,
                "Required at least random access iterator.");
            static_assert(std::random_access_iterator<Iter2>,
                "Requires at least random access iterator.");
            static_assert(std::random_access_iterator<Iter3>,
                "Requires at least random access iterator.");

            using result_type = hpx::ranges::merge_result<Iter1, Iter2, Iter3>;

            return hpx::parallel::detail::merge<result_type>().call(
                HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2,
                dest, HPX_MOVE(comp), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename Rng1, typename Rng2, typename Iter3,
            typename Comp = hpx::ranges::less, typename Proj1 = hpx::identity,
            typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng1> &&
                hpx::parallel::traits::is_projected_range_v<Proj1, Rng1> &&
                std::ranges::range<Rng2> &&
                hpx::parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                hpx::traits::is_iterator_v<Iter3> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Comp,
                    hpx::parallel::traits::projected_range<Proj1, Rng1>,
                    hpx::parallel::traits::projected_range<Proj2, Rng2>
                >
            )
        // clang-format on
        static hpx::ranges::merge_result<std::ranges::iterator_t<Rng1>,
            std::ranges::iterator_t<Rng2>, Iter3>
        invoke_default(Rng1&& rng1, Rng2&& rng2, Iter3 dest, Comp comp = Comp(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            using iterator_type1 = std::ranges::iterator_t<Rng1>;
            using iterator_type2 = std::ranges::iterator_t<Rng2>;

            static_assert(std::random_access_iterator<iterator_type1>,
                "Required at least random access iterator.");
            static_assert(std::random_access_iterator<iterator_type2>,
                "Requires at least random access iterator.");
            static_assert(std::random_access_iterator<Iter3>,
                "Requires at least random access iterator.");

            using result_type = hpx::ranges::merge_result<iterator_type1,
                iterator_type2, Iter3>;

            return hpx::parallel::detail::merge<result_type>().call(
                hpx::execution::seq, hpx::util::begin(rng1),
                hpx::util::end(rng1), hpx::util::begin(rng2),
                hpx::util::end(rng2), dest, HPX_MOVE(comp), HPX_MOVE(proj1),
                HPX_MOVE(proj2));
        }

        template <typename Iter1, typename Sent1, typename Iter2,
            typename Sent2, typename Iter3, typename Comp = hpx::ranges::less,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                std::sentinel_for<Sent1, Iter1> &&
                hpx::parallel::traits::is_projected_v<Proj1, Iter1> &&
                std::sentinel_for<Sent2, Iter2> &&
                hpx::parallel::traits::is_projected_v<Proj2, Iter2> &&
                hpx::traits::is_iterator_v<Iter3> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Comp,
                    hpx::parallel::traits::projected<Proj1, Iter1>,
                    hpx::parallel::traits::projected<Proj2, Iter2>
                >
            )
        // clang-format on
        static hpx::ranges::merge_result<Iter1, Iter2, Iter3> invoke_default(
            Iter1 first1, Sent1 last1, Iter2 first2, Sent2 last2, Iter3 dest,
            Comp comp = Comp(), Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            static_assert(std::random_access_iterator<Iter1>,
                "Required at least random access iterator.");
            static_assert(std::random_access_iterator<Iter2>,
                "Requires at least random access iterator.");
            static_assert(std::random_access_iterator<Iter3>,
                "Requires at least random access iterator.");

            using result_type = hpx::ranges::merge_result<Iter1, Iter2, Iter3>;

            return hpx::parallel::detail::merge<result_type>().call(
                hpx::execution::seq, first1, last1, first2, last2, dest,
                HPX_MOVE(comp), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        /// \brief Produce the bounded prefix of an ordered operation.
        /// \returns Input and output resume positions, or their future.
        template <typename ExPolicy, std::random_access_iterator I1,
            std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2, std::random_access_iterator O,
            std::sized_sentinel_for<O> OutS, typename Comp = std::ranges::less,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::mergeable<I1, I2, O, Comp, Proj1, Proj2>
        static decltype(auto) invoke_default(ExPolicy&& policy, I1 first1,
            S1 last1, I2 first2, S2 last2, O dest, OutS dest_last,
            Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {})
        {
            return parallel::detail::bounded_merge<I1, I2, O>().call(
                HPX_FORWARD(ExPolicy, policy), first1,
                first1 + (last1 - first1), first2, first2 + (last2 - first2),
                dest, dest + (dest_last - dest), HPX_MOVE(comp),
                HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        /// \brief Produce the bounded prefix of an ordered operation.
        /// \returns Input and output resume positions, or their future.
        template <typename ExPolicy, std::ranges::random_access_range R1,
            std::ranges::random_access_range R2,
            std::ranges::random_access_range OutR,
            typename Comp = std::ranges::less, typename Proj1 = hpx::identity,
            typename Proj2 = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R1> && std::ranges::sized_range<R2> &&
            std::ranges::sized_range<OutR> &&
            std::mergeable<std::ranges::iterator_t<R1>,
                std::ranges::iterator_t<R2>, std::ranges::iterator_t<OutR>,
                Comp, Proj1, Proj2>
        static decltype(auto) invoke_default(ExPolicy&& policy, R1&& rng1,
            R2&& rng2, OutR&& output, Comp comp = {}, Proj1 proj1 = {},
            Proj2 proj2 = {})
        {
            using iterator_result = merge_result<std::ranges::iterator_t<R1>,
                std::ranges::iterator_t<R2>, std::ranges::iterator_t<OutR>>;
            using result_type =
                merge_result<std::ranges::borrowed_iterator_t<R1>,
                    std::ranges::borrowed_iterator_t<R2>,
                    std::ranges::borrowed_iterator_t<OutR>>;
            auto first1 = std::ranges::begin(rng1);
            auto first2 = std::ranges::begin(rng2);
            auto dest = std::ranges::begin(output);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first1,
                    first1 + std::ranges::distance(rng1), first2,
                    first2 + std::ranges::distance(rng2), dest,
                    dest + std::ranges::distance(output), HPX_MOVE(comp),
                    HPX_MOVE(proj1), HPX_MOVE(proj2)),
                [](iterator_result result) -> result_type {
                    return {result.in1, result.in2, result.out};
                });
        }
    } merge{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::inplace_merge
    HPX_CXX_CORE_EXPORT inline constexpr struct inplace_merge_t final
      : hpx::detail::tag_dispatch<inplace_merge_t,
            hpx::detail::tag_parallel_algorithm<inplace_merge_t>>
    {
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

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Comp = std::ranges::less,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::sortable<I, Comp, Proj>
        static decltype(auto) invoke_default(ExPolicy&& policy, I first,
            I middle, S last, Comp comp = {}, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::detail::inplace_merge<I>().call(
                HPX_FORWARD(ExPolicy, policy), first, middle, end,
                HPX_MOVE(comp), HPX_MOVE(proj));
        }

        template <typename Rng, typename Iter,
            typename Comp = hpx::ranges::less, typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::traits::is_iterator_v<Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Comp,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static Iter invoke_default(
            Rng&& rng, Iter middle, Comp comp = Comp(), Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::random_access_iterator<iterator_type>,
                "Required at least random access iterator.");
            static_assert(std::random_access_iterator<Iter>,
                "Required at least random access iterator.");

            return hpx::parallel::detail::inplace_merge<Iter>().call(
                hpx::execution::seq, hpx::util::begin(rng), middle,
                hpx::util::end(rng), HPX_MOVE(comp), HPX_MOVE(proj));
        }

        template <typename Iter, typename Sent,
            typename Comp = hpx::ranges::less, typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::sentinel_for<Sent, Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Comp,
                    hpx::parallel::traits::projected<Proj, Iter>,
                    hpx::parallel::traits::projected<Proj, Iter>
                >
            )
        // clang-format on
        static Iter invoke_default(Iter first, Iter middle, Sent last,
            Comp comp = Comp(), Proj proj = Proj())
        {
            static_assert(std::random_access_iterator<Iter>,
                "Required at least random access iterator.");

            return hpx::parallel::detail::inplace_merge<Iter>().call(
                hpx::execution::seq, first, middle, last, HPX_MOVE(comp),
                HPX_MOVE(proj));
        }
    } inplace_merge{};
}    // namespace hpx::ranges

#endif    //DOXYGEN
