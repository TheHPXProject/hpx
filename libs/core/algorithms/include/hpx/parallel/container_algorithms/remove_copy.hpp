//  Copyright (c) 2015-2023 Hartmut Kaiser
//  Copyright (c) 2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/remove_copy.hpp
/// \page hpx::ranges::remove_copy, hpx::ranges::remove_copy_if
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {

    /////////////////////////////////////////////////////////////////////////////
    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// predicate \a pred returns false. The order of the elements that are not
    /// removed is preserved.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range [first,last) for which the following corresponding
    ///          conditions do not hold:
    ///          INVOKE(pred, INVOKE(proj, *it)) != false.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a f.
    ///
    /// \tparam I           The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     Input iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for I.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a remove_copy_if requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements to be removed. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a InIter can be dereferenced and then
    ///                     implicitly converted to Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove_copy_if algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a ranges::remove_copy_if algorithm
    ///           returns \a ranges::remove_copy_if_result<I, O>.
    ///           The \a ranges::remove_copy algorithm returns an object
    ///           {last, result + N}, where N is the number of
    ///           elements copied.
    ///
    template <typename I, typename Sent, typename O, typename Pred,
        typename Proj = hpx::identity>
    ranges::remove_copy_if_result<I, O> ranges::remove_copy_if(
        I first, Sent last, O dest, Pred&& pred, Proj&& proj = Proj());

    /////////////////////////////////////////////////////////////////////////////
    /// Copies the elements in the range, defined by rng, to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// predicate \a pred returns false. The order of the elements that are not
    /// removed is preserved.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range rng for which the following corresponding
    ///          conditions do not hold:
    ///          INVOKE(pred, INVOKE(proj, *it)) != false.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a pred.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a copy_if requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
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
    ///                     elements to be removed. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a InIter can be dereferenced and then
    ///                     implicitly converted to Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove_copy_if algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a ranges::remove_copy_if algorithm returns a
    ///           \a remove_copy_if_result<
    ///             std::ranges::iterator_t<Rng>, O>>.
    ///           The \a ranges::remove_copy algorithm returns an object
    ///           {last, result + N}, where N is the number of
    ///           elements copied.
    ///
    template <typename Rng, typename O, typename Pred,
        typename Proj = hpx::identity>
    remove_copy_if_result<std::ranges::iterator_t<Rng>, O>
    ranges::remove_copy_if(
        Rng&& rng, O dest, Pred&& pred, Proj&& proj = Proj());

    /////////////////////////////////////////////////////////////////////////////
    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// predicate \a pred returns false. The order of the elements that are not
    /// removed is preserved.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range [first,last) for which the following corresponding
    ///          conditions do not hold:
    ///          INVOKE(pred, INVOKE(proj, *it)) != false.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a f.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam I           The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     Forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for I.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a remove_copy_if requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements to be removed. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a InIter can be dereferenced and then
    ///                     implicitly converted to Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove_copy_if algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a remove_copy_if algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a ranges::remove_copy_if algorithm returns a
    ///           \a hpx::future<ranges::remove_copy_if_result<I, O>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a ranges::remove_copy_if_result<I, O>
    ///           otherwise.
    ///           The \a ranges::remove_copy algorithm returns an object
    ///           {last, result + N}, where N is the number of
    ///           elements copied.
    ///
    template <typename ExPolicy, typename I, typename Sent, typename O,
        typename Pred, typename Proj = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        remove_copy_if_result<I, O>>::type
    ranges::remove_copy_if(ExPolicy&& policy, I first, Sent last, O dest,
        Pred&& pred, Proj&& proj = Proj());

    /////////////////////////////////////////////////////////////////////////////
    /// Copies the elements in the range, defined by rng, to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// predicate \a pred returns false. The order of the elements that are not
    /// removed is preserved.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range rng for which the following corresponding
    ///          conditions do not hold:
    ///          INVOKE(pred, INVOKE(proj, *it)) != false.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a pred.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a copy_if requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
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
    ///                     elements to be removed. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a InIter can be dereferenced and then
    ///                     implicitly converted to Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove_copy_if algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a remove_copy_if algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a ranges::remove_copy_if algorithm returns a
    ///           \a hpx::future<remove_copy_if_result<
    ///             std::ranges::iterator_t<Rng>, O>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a remove_copy_if_result<
    ///             std::ranges::iterator_t<Rng>, O>>
    ///           otherwise.
    ///           The \a ranges::remove_copy algorithm returns an object
    ///           {last, result + N}, where N is the number of
    ///           elements copied.
    ///
    template <typename ExPolicy, typename Rng, typename O, typename Pred,
        typename Proj = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        remove_copy_if_result<std::ranges::iterator_t<Rng>, O>>::type
    ranges::remove_copy_if(ExPolicy&& policy, Rng&& rng, O dest, Pred&& pred,
        Proj&& proj = Proj());

    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// comparison operator returns false when compare to val.
    /// The order of the elements that are not removed is preserved.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range [first,last) for which the following corresponding
    ///          conditions do not hold: INVOKE(proj, *it) == value
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a f.
    ///
    /// \tparam I           The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for I.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam T           The type that the result of dereferencing InIter is
    ///                     compared to.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param value        Value to be removed.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove_copy algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a ranges::remove_copy algorithm returns a
    ///           \a ranges::remove_copy_result<I, O>
    ///           The \a ranges::remove_copy algorithm returns an object
    ///           {last, result + N}, where N is the number of
    ///           elements copied.
    ///
    template <typename I, typename Sent, typename O,
        typename Proj = hpx::identity,
        typename T =
            typename hpx::parallel::traits::projected<I, Proj>::value_type>
    remove_copy_result<I, O> ranges::remove_copy(
        I first, Sent last, O dest, T const& value, Proj&& proj = Proj());

    /// Copies the elements in the range, defined by rng, to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// comparison operator returns false when compare to val.
    /// The order of the elements that are not removed is preserved.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range [first,last) for which the following corresponding
    ///          conditions do not hold: INVOKE(proj, *it) == value
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a f.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam T           The type that the result of dereferencing InIter is
    ///                     compared to.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param val          Value to be removed.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove_copy algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a ranges::remove_copy algorithm returns a
    ///           \a remove_copy_result<
    ///            std::ranges::iterator_t<Rng>, O>.
    ///           The \a ranges::remove_copy algorithm returns an object
    ///           {last, result + N}, where N is the number of
    ///           elements copied.
    ///
    template <typename Rng, typename O, typename Proj = hpx::identity,
        typename T = typename hpx::parallel::traits::projected<
            std::ranges::iterator_t<Rng>, Proj>::value_type>
    remove_copy_result<std::ranges::iterator_t<Rng>, O> ranges::remove_copy(
        Rng&& rng, O dest, T const& val, Proj&& proj = Proj());

    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// comparison operator returns false when compare to val.
    /// The order of the elements that are not removed is preserved.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range [first,last) for which the following corresponding
    ///          conditions do not hold: INVOKE(proj, *it) == value
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a f.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam I           The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     Forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for I.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam T           The type that the result of dereferencing InIter is
    ///                     compared to.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param value        Value to be removed.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove_copy algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a remove_copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a ranges::remove_copy algorithm returns a
    ///           \a hpx::future<ranges::remove_copy_result<I, O>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a ranges::remove_copy_result<I, O>
    ///           otherwise.
    ///           The \a ranges::remove_copy algorithm returns an object
    ///           {last, result + N}, where N is the number of
    ///           elements copied.
    ///
    template <typename ExPolicy, typename I, typename Sent, typename O,
        typename Proj = hpx::identity,
        typename T =
            typename hpx::parallel::traits::projected<I, Proj>::value_type>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        remove_copy_result<I, O>>::type
    ranges::remove_copy(ExPolicy&& policy, I first, Sent last, O dest,
        T const& value, Proj&& proj = Proj());

    /// Copies the elements in the range, defined by rng, to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// comparison operator returns false when compare to val.
    /// The order of the elements that are not removed is preserved.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range [first,last) for which the following corresponding
    ///          conditions do not hold: INVOKE(proj, *it) == value
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a f.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam T           The type that the result of dereferencing InIter is
    ///                     compared to.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param value        Value to be removed.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove_copy algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a remove_copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a ranges::remove_copy algorithm returns a
    ///           \a hpx::future<remove_copy_result<
    ///            std::ranges::iterator_t<Rng>, O>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a remove_copy_result<
    ///            std::ranges::iterator_t<Rng>, O>
    ///           otherwise.
    ///           The \a ranges::remove_copy algorithm returns an object
    ///           {last, result + N}, where N is the number of
    ///           elements copied.
    ///
    template <typename ExPolicy, typename Rng, typename O, typename T,
        typename Proj = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        remove_copy_result<std::ranges::iterator_t<Rng>, O>>::type
    ranges::remove_copy(ExPolicy&& policy, Rng&& rng, O dest, T const& value,
        Proj&& proj = Proj());

}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/remove_copy.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using remove_copy_result = hpx::parallel::util::in_out_result<I, O>;

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using remove_copy_if_result = hpx::parallel::util::in_out_result<I, O>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::remove_copy_if
    HPX_CXX_CORE_EXPORT inline constexpr struct remove_copy_if_t final
      : hpx::detail::tag_dispatch<remove_copy_if_t,
            hpx::detail::tag_parallel_algorithm<remove_copy_if_t>>
    {
        template <typename I, typename Sent, typename O, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<I> &&
                hpx::parallel::traits::is_projected_v<Proj, I> &&
                std::sentinel_for<Sent, I> &&
                hpx::traits::is_iterator_v<O> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj, I>
                >
            )
        // clang-format on
        static remove_copy_if_result<I, O> invoke_default(
            I first, Sent last, O dest, Pred pred, Proj proj = Proj())
        {
            static_assert(std::input_iterator<I>, "Required input iterator.");

            static_assert(std::output_iterator<O, hpx::traits::iter_value_t<I>>,
                "Required output iterator.");

            return hpx::parallel::detail::remove_copy_if<
                hpx::parallel::util::in_out_result<I, O>>()
                .call(hpx::execution::seq, first, last, dest, HPX_MOVE(pred),
                    HPX_MOVE(proj));
        }

        template <typename Rng, typename O, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static remove_copy_if_result<std::ranges::iterator_t<Rng>, O>
        invoke_default(Rng&& rng, O dest, Pred pred, Proj proj = Proj())
        {
            static_assert(std::input_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least input iterator.");

            return hpx::parallel::detail::remove_copy_if<hpx::parallel::util::
                    in_out_result<std::ranges::iterator_t<Rng>, O>>()
                .call(hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), dest, HPX_MOVE(pred), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename I, typename Sent, typename O,
            typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<I> &&
                std::sentinel_for<Sent, I> &&
                hpx::traits::is_iterator_v<O> &&
                hpx::parallel::traits::is_projected_v<Proj, I> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, Pred,
                    hpx::parallel::traits::projected<Proj, I>
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            remove_copy_if_result<I, O>>
        invoke_default(ExPolicy&& policy, I first, Sent last, O dest, Pred pred,
            Proj proj = Proj())
        {
            static_assert(std::forward_iterator<I>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<O>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::remove_copy_if<
                hpx::parallel::util::in_out_result<I, O>>()
                .call(HPX_FORWARD(ExPolicy, policy), first, last, dest,
                    HPX_MOVE(pred), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng, typename O, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, Pred,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            remove_copy_if_result<std::ranges::iterator_t<Rng>, O>>
        invoke_default(
            ExPolicy&& policy, Rng&& rng, O dest, Pred pred, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::remove_copy_if<hpx::parallel::util::
                    in_out_result<std::ranges::iterator_t<Rng>, O>>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                    hpx::util::end(rng), dest, HPX_MOVE(pred), HPX_MOVE(proj));
        }
    } remove_copy_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::remove_copy
    HPX_CXX_CORE_EXPORT inline constexpr struct remove_copy_t final
      : hpx::detail::tag_dispatch<remove_copy_t,
            hpx::detail::tag_parallel_algorithm<remove_copy_t>>
    {
        template <typename I, typename Sent, typename O,
            typename Proj = hpx::identity,
            typename T =
                typename hpx::parallel::traits::projected<I, Proj>::value_type>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<I> &&
                std::sentinel_for<Sent, I> &&
                hpx::traits::is_iterator_v<O> &&
                hpx::parallel::traits::is_projected_v<Proj, I>
            )
        // clang-format on
        static remove_copy_result<I, O> invoke_default(
            I first, Sent last, O dest, T const& value, Proj proj = Proj())
        {
            static_assert(
                std::input_iterator<I>, "Required at least input iterator.");

            return hpx::ranges::remove_copy_if(
                first, last, dest,
                [value](T const& a) -> bool { return value == a; },
                HPX_MOVE(proj));
        }

        template <typename Rng, typename O, typename Proj = hpx::identity,
            typename T = typename hpx::parallel::traits::projected<
                std::ranges::iterator_t<Rng>, Proj>::value_type>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng>
            )
        // clang-format on
        static remove_copy_result<std::ranges::iterator_t<Rng>, O>
        invoke_default(Rng&& rng, O dest, T const& value, Proj proj = Proj())
        {
            static_assert(std::input_iterator<std::ranges::iterator_t<Rng>>,
                "Required at input forward iterator.");

            return hpx::ranges::remove_copy_if(
                HPX_FORWARD(Rng, rng), dest,
                [value](T const& a) -> bool { return value == a; },
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename I, typename Sent, typename O,
            typename Proj = hpx::identity,
            typename T =
                typename hpx::parallel::traits::projected<I, Proj>::value_type>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy>&&
                hpx::traits::is_iterator_v<I> &&
                std::sentinel_for<Sent, I> &&
                hpx::traits::is_iterator_v<O> &&
                hpx::parallel::traits::is_projected_v<Proj, I>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            remove_copy_result<I, O>>
        invoke_default(ExPolicy&& policy, I first, Sent last, O dest,
            T const& value, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<I>,
                "Required at least forward iterator.");

            return hpx::ranges::remove_copy_if(
                HPX_FORWARD(ExPolicy, policy), first, last, dest,
                [value](T const& a) -> bool { return value == a; },
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng, typename O,
            typename Proj = hpx::identity,
            typename T = typename hpx::parallel::traits::projected<
                std::ranges::iterator_t<Rng>, Proj>::value_type>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            remove_copy_result<std::ranges::iterator_t<Rng>, O>>
        invoke_default(ExPolicy&& policy, Rng&& rng, O dest, T const& value,
            Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            return hpx::ranges::remove_copy_if(
                HPX_FORWARD(ExPolicy, policy), HPX_FORWARD(Rng, rng), dest,
                [value](T const& a) -> bool { return value == a; },
                HPX_MOVE(proj));
        }
    } remove_copy{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
