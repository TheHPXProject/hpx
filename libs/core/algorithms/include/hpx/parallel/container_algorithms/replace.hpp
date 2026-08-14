//  Copyright (c) 2015-2023 Hartmut Kaiser
//  Copyright (c) 2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/replace.hpp
/// \page hpx::ranges::replace, hpx::ranges::replace_if, hpx::ranges::replace_copy, hpx::ranges::replace_copy_if
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {

    ///////////////////////////////////////////////////////////////////////////
    /// Replaces all elements satisfying specific criteria (for which predicate
    /// \a f returns true) with \a new_value in the range [first, sent).
    ///
    /// \note   Complexity: Performs exactly \a sent - \a first applications of
    ///         the predicate.
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          [first, sent) with new_value, when the following corresponding
    ///          conditions hold: INVOKE(f, INVOKE(proj, *it)) != false
    ///
    /// \tparam Iter        The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for Iter.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///                     (deduced).
    /// \tparam T           The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements which need to replaced. The
    ///                     signature of this predicate should be equivalent
    ///                     to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a Iter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace_if algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a replace_if algorithm returns \a Iter.
    ///           It returns \a last.
    ///
    template <typename Iter, typename Sent, typename Pred,
        typename Proj = hpx::identity,
        typename T =
            typename hpx::parallel::traits::projected<Iter, Proj>::value_type>
    Iter replace_if(Iter first, Sent sent, Pred&& pred, T const& new_value,
        Proj&& proj = Proj());

    /// Replaces all elements satisfying specific criteria (for which predicate
    /// \a pred returns true) with \a new_value in the range rng.
    ///
    /// \note   Complexity: Performs exactly \a util::end(rng) - \a util::begin(rng)
    ///         applications of the predicate.
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          rng with new_value, when the following corresponding
    ///          conditions hold: INVOKE(f, INVOKE(proj, *it)) != false
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///                     (deduced).
    /// \tparam T           The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by rng.This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements which need to replaced. The
    ///                     signature of this predicate should be equivalent
    ///                     to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a replace_if algorithm returns an \a
    ///           hpx::traits::range_iterator<Rng>::type.
    ///           It returns \a last.
    ///
    template <typename Rng, typename Pred, typename Proj = hpx::identity,
        typename T = typename hpx::parallel::traits::projected<
            std::ranges::iterator_t<Rng>, Proj>::value_type>
    std::ranges::iterator_t<Rng> replace_if(
        Rng&& rng, Pred&& pred, T const& new_value, Proj&& proj = Proj());

    /// Replaces all elements satisfying specific criteria (for which predicate
    /// \a pred returns true) with \a new_value in the range rng.
    ///
    /// \note   Complexity: Performs exactly \a util::end(rng) - \a util::begin(rng)
    ///         applications of the predicate.
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          rng with new_value, when the following corresponding
    ///          conditions hold: INVOKE(f, INVOKE(proj, *it)) != false
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Iter        The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for Iter.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    ///                     (deduced).
    /// \tparam T           The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements which need to replaced. The
    ///                     signature of this predicate should be equivalent
    ///                     to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace_if algorithm invoked with an
    /// execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a replace_if algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a replace_if algorithm returns a \a hpx::future<Iter>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy.
    ///           It returns \a last.
    ///
    template <typename ExPolicy, typename Iter, typename Sent, typename Pred,
        typename Proj = hpx::identity,
        typename T =
            typename hpx::parallel::traits::projected<Iter, Proj>::value_type>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, Iter> replace_if(
        ExPolicy&& policy, Iter first, Sent sent, Pred&& pred,
        T const& new_value, Proj&& proj = Proj());

    /// Replaces all elements satisfying specific criteria (for which predicate
    /// \a pred returns true) with \a new_value in the range rng.
    ///
    /// \note   Complexity: Performs exactly \a util::end(rng) - \a util::begin(rng)
    ///         applications of the predicate.
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          rng with new_value, when the following corresponding
    ///          conditions hold: INVOKE(f, INVOKE(proj, *it)) != false
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///                     (deduced).
    /// \tparam T           The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by rng.This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements which need to replaced. The
    ///                     signature of this predicate should be equivalent
    ///                     to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace algorithm invoked with an
    /// execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a replace algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a replace_if algorithm returns a \a
    ///           hpx::future<std::ranges::iterator_t<Rng>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy.
    ///           It returns \a last.
    ///
    template <typename ExPolicy, typename Rng, typename Pred,
        typename Proj = hpx::identity,
        typename T = typename hpx::parallel::traits::projected<
            std::ranges::iterator_t<Rng>, Proj>::value_type>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        std::ranges::iterator_t<Rng>>
    replace_if(ExPolicy&& policy, Rng&& rng, Pred&& pred, T const& new_value,
        Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Replaces all elements satisfying specific criteria with \a new_value
    /// in the range [first, last).
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          [first,last) with new_value, when the following corresponding
    ///          conditions hold: INVOKE(proj, *i) == old_value
    ///
    /// \tparam Iter        The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for Iter.
    /// \tparam T1          The type of the old value to replace (deduced).
    /// \tparam T2          The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a replace algorithm returns an \a Iter.
    ///
    template <typename Iter, typename Sent, typename Proj = hpx::identity,
        typename T1 =
            typename hpx::parallel::traits::projected<Iter, Proj>::value_type,
        typename T2 = T1>
    Iter replace(Iter first, Sent sent, T1 const& old_value,
        T2 const& new_value, Proj&& proj = Proj());

    /// Replaces all elements satisfying specific criteria with \a new_value
    /// in the range \a rng.
    ///
    /// \note   Complexity: Performs exactly \a util::end(rng) - \a util::begin(rng)
    ///         assignments.
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          rng with new_value, when the following corresponding
    ///          conditions hold: INVOKE(proj, *i) == old_value
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam T1          The type of the old value to replace (deduced).
    /// \tparam T2          The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a replace algorithm returns an
    ///           \a hpx::traits::range_iterator<Rng>::type.
    ///
    template <typename Rng, typename Proj = hpx::identity,
        typename T1 = typename hpx::parallel::traits::projected<
            std::ranges::iterator_t<Rng>, Proj>::value_type,
        typename T2 = T1>
    std::ranges::iterator_t<Rng> replace(Rng&& rng, T1 const& old_value,
        T2 const& new_value, Proj&& proj = Proj());

    /// Replaces all elements satisfying specific criteria with \a new_value
    /// in the range [first, last).
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          [first,last) with new_value, when the following corresponding
    ///          conditions hold: INVOKE(proj, *i) == old_value
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Iter        The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for Iter.
    /// \tparam T1          The type of the old value to replace (deduced).
    /// \tparam T2          The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace algorithm invoked with an
    /// execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a replace algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a replace algorithm returns a \a hpx::future<Iter> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a Iter otherwise.
    ///
    template <typename ExPolicy, typename Iter, typename Sent,
        typename Proj = hpx::identity,
        typename T1 =
            typename hpx::parallel::traits::projected<Iter, Proj>::value_type,
        typename T2 = T1>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, Iter> replace(
        ExPolicy&& policy, Iter first, Sent sent, T1 const& old_value,
        T2 const& new_value, Proj&& proj = Proj());

    /// Replaces all elements satisfying specific criteria with \a new_value
    /// in the range \a rng.
    ///
    /// \note   Complexity: Performs exactly \a util::end(rng) - \a util::begin(rng)
    ///         assignments.
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          rng with new_value, when the following corresponding
    ///          conditions hold: INVOKE(proj, *i) == old_value
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam T1          The type of the old value to replace (deduced).
    /// \tparam T2          The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    /// The assignments in the parallel \a replace algorithm invoked with an
    /// execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a replace algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a replace algorithm returns an
    ///           \a hpx::future<hpx::traits::range_iterator<Rng>::type> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a hpx::traits::range_iterator<Rng>::type otherwise.
    ///
    template <typename ExPolicy, typename Rng, typename Proj = hpx::identity,
        typename T1 = typename hpx::parallel::traits::projected<
            std::ranges::iterator_t<Rng>, Proj>::value_type,
        typename T2 = T1>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        std::ranges::iterator_t<Rng>>
    replace(ExPolicy&& policy, Rng&& rng, T1 const& old_value,
        T2 const& new_value, Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Copies the all elements from the range [first, sent) to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (sent - first)) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          INVOKE(f, INVOKE(proj, *(first + (i - result)))) != false
    ///
    /// \note   Complexity: Performs exactly \a sent - \a first applications of
    ///         the predicate.
    ///
    /// \tparam InIter      The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    ///                     (deduced).
    /// \tparam T           The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements which need to replaced. The
    ///                     signature of this predicate should be equivalent
    ///                     to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace_copy_if algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a replace_copy_if algorithm returns a
    ///           \a in_out_result<InIter, OutIter>.
    ///           The \a replace_copy_if algorithm returns the input iterator
    ///           \a last and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename InIter, typename Sent, typename OutIter, typename Pred,
        typename T = typename std::iterator_traits<OutIter>::value_type,
        typename Proj = hpx::identity>
    replace_copy_if_result<InIter, OutIter> replace_copy_if(InIter first,
        Sent sent, OutIter dest, Pred&& pred, T const& new_value,
        Proj&& proj = Proj());

    /// Copies the all elements from the range rng to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (util::end(rng) - util::begin(rng))) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          INVOKE(f, INVOKE(proj, *(first + (i - result)))) != false
    ///
    /// \note   Complexity: Performs exactly \a util::end(rng) - \a util::begin(rng)
    ///         applications of the predicate.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    ///                     (deduced).
    /// \tparam T           The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements which need to replaced. The
    ///                     signature of this predicate should be equivalent
    ///                     to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace_copy_if algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a replace_copy_if algorithm returns an
    ///           \a in_out_result<std::ranges::iterator_t<Rng>,
    ///             OutIter>.
    ///           The \a replace_copy_if algorithm returns the input iterator
    ///           \a last and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename Rng, typename OutIter, typename Pred,
        typename T = typename std::iterator_traits<OutIter>::value_type,
        typename Proj = hpx::identity>
    replace_copy_if_result<std::ranges::iterator_t<Rng>, OutIter>
    replace_copy_if(Rng&& rng, OutIter dest, Pred&& pred, T const& new_value,
        Proj&& proj = Proj());

    /// Copies the all elements from the range [first, sent) to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (sent - first)) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          INVOKE(f, INVOKE(proj, *(first + (i - result)))) != false
    ///
    /// \note   Complexity: Performs exactly \a sent - \a first applications of
    ///         the predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    ///                     (deduced).
    /// \tparam T           The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements which need to replaced. The
    ///                     signature of this predicate should be equivalent
    ///                     to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace_copy_if algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a replace_copy_if algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a replace_copy_if algorithm returns an
    ///           \a hpx::future<FwdIter1, FwdIter2>.
    ///           The \a replace_copy_if algorithm returns the input iterator
    ///           \a last and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Sent,
        typename FwdIter2, typename Pred,
        typename T = typename std::iterator_traits<FwdIter2>::value_type,
        typename Proj = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        replace_copy_if_result<FwdIter1, FwdIter2>>::type
    replace_copy_if(ExPolicy&& policy, FwdIter1 first, Sent sent, FwdIter2 dest,
        Pred&& pred, T const& new_value, Proj&& proj = Proj());

    /// Copies the all elements from the range rng to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (util::end(rng) - util::begin(rng))) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          INVOKE(f, INVOKE(proj, *(first + (i - result)))) != false
    ///
    /// \note   Complexity: Performs exactly \a util::end(rng) - \a util::begin(rng)
    ///         applications of the predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    ///                     (deduced).
    /// \tparam T           The type of the new values to replace (deduced).
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
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements which need to replaced. The
    ///                     signature of this predicate should be equivalent
    ///                     to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace_copy_if algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a replace_copy_if algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a replace_copy_if algorithm returns an
    ///           \a hpx::future<in_out_result<std::ranges::iterator_t<Rng>,
    ///             OutIter>>.
    ///           The \a replace_copy_if algorithm returns the input iterator
    ///           \a last and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename Rng, typename FwdIter, typename Pred,
        typename T = typename std::iterator_traits<FwdIter>::value_type,
        typename Proj = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        replace_copy_if_result<std::ranges::iterator_t<Rng>, FwdIter>>::type
    replace_copy_if(ExPolicy&& policy, Rng&& rng, FwdIter dest, Pred&& pred,
        T const& new_value, Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Copies the all elements from the range [first, sent) to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (sent - first)) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          INVOKE(proj, *(first + (i - result))) == old_value
    ///
    /// \note   Complexity: Performs exactly \a sent - \a first applications of
    ///         the predicate.
    ///
    /// \tparam InIter      The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for Iter.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam T1          The type of the old value to replace (deduced).
    /// \tparam T2          The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace_copy algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a replace_copy algorithm returns an
    ///           \a in_out_result<InIter, OutIter>.
    ///           The \a copy algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename InIter, typename Sent, typename OutIter,
        typename Proj = hpx::identity,
        typename T1 =
            typename hpx::parallel::traits::projected<InIter, Proj>::value_type,
        typename T2 = T1>
    replace_copy_result<InIter, OutIter> replace_copy(InIter first, Sent sent,
        OutIter dest, T1 const& old_value, T2 const& new_value,
        Proj&& proj = Proj());

    /// Copies the all elements from the range rbg to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (util::end(rng) - util::begin(rng))) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          INVOKE(proj, *(first + (i - result))) == old_value
    ///
    /// \note   Complexity: Performs exactly \a util::end(rng) - \a util::begin(rng)
    ///         applications of the predicate.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam T1          The type of the old value to replace (deduced).
    /// \tparam T2          The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace_copy algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a replace_copy algorithm returns an \a
    ///           in_out_result<std::ranges::iterator_t<Rng>,
    ///           OutIter>.
    ///           The \a copy algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename Rng, typename OutIter, typename Proj = hpx::identity,
        typename T1 = typename hpx::parallel::traits::projected<
            std::ranges::iterator_t<Rng>, Proj>::value_type,
        typename T2 = T1>
    replace_copy_result<std::ranges::iterator_t<Rng>, OutIter> replace_copy(
        Rng&& rng, OutIter dest, T1 const& old_value, T2 const& new_value,
        Proj&& proj = Proj());

    /// Copies the all elements from the range [first, sent) to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (sent - first)) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          INVOKE(proj, *(first + (i - result))) == old_value
    ///
    /// \note   Complexity: Performs exactly \a sent - \a first applications of
    ///         the predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for Iter.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam T1          The type of the old value to replace (deduced).
    /// \tparam T2          The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace_copy algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a replace_copy algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a replace_copy algorithm returns a
    ///           \a hpx::future<in_out_result<FwdIter1, FwdIter2>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a in_out_result<FwdIter1, FwdIter2>
    ///           otherwise.
    ///           The \a copy algorithm returns the pair of the forward iterator
    ///           \a last and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Sent,
        typename FwdIter2, typename Proj = hpx::identity,
        typename T1 = typename hpx::parallel::traits::projected<FwdIter1,
            Proj>::value_type,
        typename T2 = T1>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        replace_copy_result<FwdIter1, FwdIter2>>::type
    replace_copy(ExPolicy&& policy, FwdIter1 first, Sent sent, FwdIter2 dest,
        T1 const& old_value, T2 const& new_value, Proj&& proj = Proj());

    /// Copies the all elements from the range rbg to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (util::end(rng) - util::begin(rng))) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          INVOKE(proj, *(first + (i - result))) == old_value
    ///
    /// \note   Complexity: Performs exactly \a util::end(rng) - \a util::begin(rng)
    ///         applications of the predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam T1          The type of the old value to replace (deduced).
    /// \tparam T2          The type of the new values to replace (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a replace_copy algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a replace_copy algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a replace_copy algorithm returns a
    ///           \a hpx::future<in_out_result<
    ///            std::ranges::iterator_t<Rng>, FwdIter>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a in_out_result<
    ///            std::ranges::iterator_t<Rng>, FwdIter>>
    ///           The \a copy algorithm returns the pair of the input iterator
    ///           \a last and the forward iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename Rng, typename FwdIter,
        typename Proj = hpx::identity,
        typename T1 = typename hpx::parallel::traits::projected<
            std::ranges::iterator_t<Rng>, Proj>::value_type,
        typename T2 = T1>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        replace_copy_result<std::ranges::iterator_t<Rng>, FwdIter>>::type
    replace_copy(ExPolicy&& policy, Rng&& rng, FwdIter dest,
        T1 const& old_value, T2 const& new_value, Proj&& proj = Proj());

}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/replace.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    /// `replace_copy_if_result` is equivalent to
    /// `hpx::parallel::util::in_out_result`
    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using replace_copy_if_result = hpx::parallel::util::in_out_result<I, O>;

    /// `replace_copy_result` is equivalent to
    /// `hpx::parallel::util::in_out_result`
    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using replace_copy_result = hpx::parallel::util::in_out_result<I, O>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::replace_if
    HPX_CXX_CORE_EXPORT inline constexpr struct replace_if_t final
      : hpx::detail::tag_dispatch<replace_if_t,
            hpx::detail::tag_parallel_algorithm<replace_if_t>>
    {
        template <typename Iter, typename Sent, typename Pred,
            typename Proj = hpx::identity,
            typename T = typename hpx::parallel::traits::projected<Iter,
                Proj>::value_type>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter> &&
                std::sentinel_for<Sent, Iter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<Iter>::value_type
                >
            )
        // clang-format on
        static Iter invoke_default(Iter first, Sent sent, Pred pred,
            T const& new_value, Proj proj = Proj())
        {
            static_assert(
                std::input_iterator<Iter>, "Required at least input iterator.");

            return hpx::parallel::detail::replace_if<Iter>().call(
                hpx::execution::seq, first, sent, HPX_MOVE(pred), new_value,
                HPX_MOVE(proj));
        }

        template <typename Rng, typename Pred, typename Proj = hpx::identity,
            typename T = typename hpx::parallel::traits::projected<
                std::ranges::iterator_t<Rng>, Proj>::value_type>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<
                        std::ranges::iterator_t<Rng>
                    >::value_type
                >
            )
        // clang-format on
        static std::ranges::iterator_t<Rng> invoke_default(
            Rng&& rng, Pred pred, T const& new_value, Proj proj = Proj())
        {
            static_assert(std::input_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least input iterator.");

            return hpx::parallel::detail::replace_if<
                std::ranges::iterator_t<Rng>>()
                .call(hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), HPX_MOVE(pred), new_value,
                    HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Iter, typename Sent,
            typename Pred, typename Proj = hpx::identity,
            typename T = typename hpx::parallel::traits::projected<Iter,
                Proj>::value_type>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter> &&
                std::sentinel_for<Sent, Iter> &&
                hpx::parallel::traits::is_indirect_callable<ExPolicy,
                    Pred, hpx::parallel::traits::projected<Proj, Iter>>::value
            )
        // clang-format on
        static typename parallel::util::detail::algorithm_result<ExPolicy,
            Iter>::type
        invoke_default(ExPolicy&& policy, Iter first, Sent sent, Pred pred,
            T const& new_value, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<Iter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::replace_if<Iter>().call(
                HPX_FORWARD(ExPolicy, policy), first, sent, HPX_MOVE(pred),
                new_value, HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng, typename Pred,
            typename Proj = hpx::identity,
            typename T = typename hpx::parallel::traits::projected<
                std::ranges::iterator_t<Rng>, Proj>::value_type>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable<ExPolicy,
                    Pred, hpx::parallel::traits::projected_range<Proj, Rng>
                >::value
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            std::ranges::iterator_t<Rng>>
        invoke_default(ExPolicy&& policy, Rng&& rng, Pred pred,
            T const& new_value, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::replace_if<
                std::ranges::iterator_t<Rng>>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                    hpx::util::end(rng), HPX_MOVE(pred), new_value,
                    HPX_MOVE(proj));
        }
    } replace_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::replace
    HPX_CXX_CORE_EXPORT inline constexpr struct replace_t final
      : hpx::detail::tag_dispatch<replace_t,
            hpx::detail::tag_parallel_algorithm<replace_t>>
    {
        template <typename Iter, typename Sent, typename Proj = hpx::identity,
            typename T1 = typename hpx::parallel::traits::projected<Iter,
                Proj>::value_type,
            typename T2 = T1>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter> &&
                std::sentinel_for<Sent, Iter>
            )
        // clang-format on
        static Iter invoke_default(Iter first, Sent sent, T1 const& old_value,
            T2 const& new_value, Proj proj = Proj())
        {
            static_assert(
                std::input_iterator<Iter>, "Required at least input iterator.");

            using type = typename std::iterator_traits<Iter>::value_type;

            return hpx::ranges::replace_if(
                first, sent,
                [old_value](type const& a) -> bool { return old_value == a; },
                new_value, HPX_MOVE(proj));
        }

        template <typename Rng, typename Proj = hpx::identity,
            typename T1 = typename hpx::parallel::traits::projected<
                std::ranges::iterator_t<Rng>, Proj>::value_type,
            typename T2 = T1>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng>
            )
        // clang-format on
        static std::ranges::iterator_t<Rng> invoke_default(Rng&& rng,
            T1 const& old_value, T2 const& new_value, Proj proj = Proj())
        {
            static_assert(std::input_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least input iterator.");

            using type = typename std::iterator_traits<
                std::ranges::iterator_t<Rng>>::value_type;

            return hpx::ranges::replace_if(
                HPX_FORWARD(Rng, rng),
                [old_value](type const& a) -> bool { return old_value == a; },
                new_value, HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Iter, typename Sent,
            typename Proj = hpx::identity,
            typename T1 = typename hpx::parallel::traits::projected<Iter,
                Proj>::value_type,
            typename T2 = T1>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<Iter> &&
                std::sentinel_for<Sent, Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter>
            )
        // clang-format on
        static typename parallel::util::detail::algorithm_result<ExPolicy,
            Iter>::type
        invoke_default(ExPolicy&& policy, Iter first, Sent sent,
            T1 const& old_value, T2 const& new_value, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<Iter>,
                "Required at least forward iterator.");

            using type = typename std::iterator_traits<Iter>::value_type;

            return hpx::ranges::replace_if(
                HPX_FORWARD(ExPolicy, policy), first, sent,
                [old_value](type const& a) -> bool { return old_value == a; },
                new_value, HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng,
            typename Proj = hpx::identity,
            typename T1 = typename hpx::parallel::traits::projected<
                std::ranges::iterator_t<Rng>, Proj>::value_type,
            typename T2 = T1>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            std::ranges::iterator_t<Rng>>
        invoke_default(ExPolicy&& policy, Rng&& rng, T1 const& old_value,
            T2 const& new_value, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            using type = typename std::iterator_traits<
                std::ranges::iterator_t<Rng>>::value_type;

            return hpx::ranges::replace_if(
                HPX_FORWARD(ExPolicy, policy), HPX_FORWARD(Rng, rng),
                [old_value](type const& a) -> bool { return old_value == a; },
                new_value, HPX_MOVE(proj));
        }
    } replace{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::replace_copy_if
    HPX_CXX_CORE_EXPORT inline constexpr struct replace_copy_if_t final
      : hpx::detail::tag_dispatch<replace_copy_if_t,
            hpx::detail::tag_parallel_algorithm<replace_copy_if_t>>
    {
        template <typename InIter, typename Sent, typename OutIter,
            typename Pred,
            typename T = typename std::iterator_traits<OutIter>::value_type,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<InIter> &&
                hpx::traits::is_iterator_v<OutIter> &&
                hpx::parallel::traits::is_projected_v<Proj, InIter> &&
                std::sentinel_for<Sent, InIter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<InIter>::value_type
                >
            )
        // clang-format on
        static replace_copy_if_result<InIter, OutIter> invoke_default(
            InIter first, Sent sent, OutIter dest, Pred pred,
            T const& new_value, Proj proj = Proj())
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");

            static_assert(std::output_iterator<OutIter,
                              hpx::traits::iter_value_t<InIter>>,
                "Required at least output iterator.");

            return hpx::parallel::detail::replace_copy_if<
                hpx::parallel::util::in_out_result<InIter, OutIter>>()
                .call(hpx::execution::seq, first, sent, dest, HPX_MOVE(pred),
                    new_value, HPX_MOVE(proj));
        }

        template <typename Rng, typename OutIter, typename Pred,
            typename T = typename std::iterator_traits<OutIter>::value_type,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<OutIter> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<
                        std::ranges::iterator_t<Rng>>::value_type
                >
            )
        // clang-format on
        static replace_copy_if_result<std::ranges::iterator_t<Rng>, OutIter>
        invoke_default(Rng&& rng, OutIter dest, Pred pred, T const& new_value,
            Proj proj = Proj())
        {
            static_assert(std::input_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least input iterator.");

            static_assert(
                std::output_iterator<OutIter,
                    hpx::traits::iter_value_t<std::ranges::iterator_t<Rng>>>,
                "Required at least output iterator.");

            return hpx::parallel::detail::replace_copy_if<hpx::parallel::util::
                    in_out_result<std::ranges::iterator_t<Rng>, OutIter>>()
                .call(hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), dest, HPX_MOVE(pred), new_value,
                    HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename FwdIter1, typename Sent,
            typename FwdIter2, typename Pred,
            typename T = typename std::iterator_traits<FwdIter2>::value_type,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter1> &&
                std::sentinel_for<Sent, FwdIter1> &&
                hpx::parallel::traits::is_indirect_callable_v<ExPolicy,
                    Pred, hpx::parallel::traits::projected<Proj, FwdIter1>
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            replace_copy_if_result<FwdIter1, FwdIter2>>
        invoke_default(ExPolicy&& policy, FwdIter1 first, Sent sent,
            FwdIter2 dest, Pred pred, T const& new_value, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::replace_copy_if<
                hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                .call(HPX_FORWARD(ExPolicy, policy), first, sent, dest,
                    HPX_MOVE(pred), new_value, HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng, typename FwdIter,
            typename Pred,
            typename T = typename std::iterator_traits<FwdIter>::value_type,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<ExPolicy,
                    Pred, hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            replace_copy_if_result<std::ranges::iterator_t<Rng>, FwdIter>>
        invoke_default(ExPolicy&& policy, Rng&& rng, FwdIter dest, Pred pred,
            T const& new_value, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::replace_copy_if<hpx::parallel::util::
                    in_out_result<std::ranges::iterator_t<Rng>, FwdIter>>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                    hpx::util::end(rng), dest, HPX_MOVE(pred), new_value,
                    HPX_MOVE(proj));
        }
    } replace_copy_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::replace_copy
    HPX_CXX_CORE_EXPORT inline constexpr struct replace_copy_t final
      : hpx::detail::tag_dispatch<replace_copy_t,
            hpx::detail::tag_parallel_algorithm<replace_copy_t>>
    {
        template <typename InIter, typename Sent, typename OutIter,
            typename Proj = hpx::identity,
            typename T1 = typename hpx::parallel::traits::projected<InIter,
                Proj>::value_type,
            typename T2 = T1>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<InIter> &&
                hpx::parallel::traits::is_projected_v<Proj, InIter> &&
                std::sentinel_for<Sent, InIter>
            )
        // clang-format on
        static replace_copy_result<InIter, OutIter> invoke_default(InIter first,
            Sent sent, OutIter dest, T1 const& old_value, T2 const& new_value,
            Proj proj = Proj())
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");

            static_assert(std::output_iterator<OutIter,
                              hpx::traits::iter_value_t<InIter>>,
                "Required at least output iterator.");

            using type = typename std::iterator_traits<InIter>::value_type;

            return hpx::ranges::replace_copy_if(
                hpx::execution::seq, first, sent, dest,
                [old_value](type const& a) -> bool { return old_value == a; },
                new_value, HPX_MOVE(proj));
        }

        template <typename Rng, typename OutIter, typename Proj = hpx::identity,
            typename T1 = typename hpx::parallel::traits::projected<
                std::ranges::iterator_t<Rng>, Proj>::value_type,
            typename T2 = T1>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng>
            )
        // clang-format on
        static replace_copy_result<std::ranges::iterator_t<Rng>, OutIter>
        invoke_default(Rng&& rng, OutIter dest, T1 const& old_value,
            T2 const& new_value, Proj proj = Proj())
        {
            static_assert(std::input_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least input iterator.");

            static_assert(
                std::output_iterator<OutIter,
                    hpx::traits::iter_value_t<std::ranges::iterator_t<Rng>>>,
                "Required at least output iterator.");

            using type = typename std::iterator_traits<
                std::ranges::iterator_t<Rng>>::value_type;

            return hpx::ranges::replace_copy_if(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                dest,
                [old_value](type const& a) -> bool { return old_value == a; },
                new_value, HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename FwdIter1, typename Sent,
            typename FwdIter2, typename Proj = hpx::identity,
            typename T1 = typename hpx::parallel::traits::projected<FwdIter1,
                Proj>::value_type,
            typename T2 = T1>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                std::sentinel_for<Sent, FwdIter1> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter1>
            )
        // clang-format on
        static typename parallel::util::detail::algorithm_result<ExPolicy,
            replace_copy_result<FwdIter1, FwdIter2>>::type
        invoke_default(ExPolicy&& policy, FwdIter1 first, Sent sent,
            FwdIter2 dest, T1 const& old_value, T2 const& new_value,
            Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            using type = typename std::iterator_traits<FwdIter1>::value_type;

            return hpx::ranges::replace_copy_if(
                HPX_FORWARD(ExPolicy, policy), first, sent, dest,
                [old_value](type const& a) -> bool { return old_value == a; },
                new_value, HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng, typename FwdIter,
            typename Proj = hpx::identity,
            typename T1 = typename hpx::parallel::traits::projected<
                std::ranges::iterator_t<Rng>, Proj>::value_type,
            typename T2 = T1>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            replace_copy_result<std::ranges::iterator_t<Rng>, FwdIter>>
        invoke_default(ExPolicy&& policy, Rng&& rng, FwdIter dest,
            T1 const& old_value, T2 const& new_value, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            using type = typename std::iterator_traits<
                std::ranges::iterator_t<Rng>>::value_type;

            return hpx::ranges::replace_copy_if(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::end(rng), dest,
                [old_value](type const& a) -> bool { return old_value == a; },
                new_value, HPX_MOVE(proj));
        }
    } replace_copy{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
