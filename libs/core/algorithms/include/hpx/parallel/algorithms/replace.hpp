//  Copyright (c) 2014-2023 Hartmut Kaiser
//  Copyright (c)      2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/replace.hpp
/// \page hpx::replace, hpx::replace_if, hpx::replace_copy, hpx::replace_copy_if
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {

    /// Replaces all elements satisfying specific criteria with \a new_value
    /// in the range [first, last).
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          [first, last) with new_value, when the following corresponding
    ///          conditions hold: *it == old_value
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam T           The type of the old and new values to replace (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
    ///
    /// The assignments in the parallel \a replace algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a replace algorithm returns a \a void.
    ///
    template <typename InIter,
        typename T = typename std::iterator_traits<InIter>::value_type>
    void replace(
        InIter first, InIter last, T const& old_value, T const& new_value);

    /// Replaces all elements satisfying specific criteria with \a new_value
    /// in the range [first, last). Executed according to the policy.
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          [first, last) with new_value, when the following corresponding
    ///          conditions hold: *it == old_value
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam T           The type of the old and new values to replace (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
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
    /// \returns  The \a replace algorithm returns a \a hpx::future<void> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a void otherwise.
    ///
    template <typename ExPolicy, typename FwdIter,
        typename T = typename std::iterator_traits<FwdIter>::value_type>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, void> replace(
        ExPolicy&& policy, FwdIter first, FwdIter last, T const& old_value,
        T const& new_value);

    /// Replaces all elements satisfying specific criteria (for which predicate
    /// \a pred returns true) with \a new_value in the range [first, last).
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          [first, last) with new_value, when the following corresponding
    ///          conditions hold: INVOKE(f, *it) != false
    ///
    /// \note   Complexity: Performs exactly \a last - \a first applications of
    ///         the predicate.
    ///
    /// \tparam Iter        The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    ///                     (deduced).
    /// \tparam T           The type of the new values to replace (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
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
    ///                     type \a InIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param new_value    Refers to the new value to use as the replacement.
    ///
    /// The assignments in the parallel \a replace_if algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a replace_if algorithm returns \a void.
    ///
    template <typename Iter, typename Pred,
        typename T = typename std::iterator_traits<Iter>::value_type>
    void replace_if(Iter first, Iter last, Pred&& pred, T const& new_value);

    /// Replaces all elements satisfying specific criteria (for which predicate
    /// \a f returns true) with \a new_value in the range [first, last).
    /// Executed according to the policy.
    ///
    /// Effects: Substitutes elements referred by the iterator it in the range
    ///          [first, last) with new_value, when the following corresponding
    ///          conditions hold: INVOKE(f, *it) != false
    ///
    /// \note   Complexity: Performs exactly \a last - \a first applications of
    ///         the predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    ///                     (deduced).
    /// \tparam T           The type of the new values to replace (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
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
    /// \returns  The \a replace_if algorithm returns a \a hpx::future<void>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy
    ///           and returns \a void otherwise.
    ///
    template <typename ExPolicy, typename FwdIter, typename Pred,
        typename T = typename std::iterator_traits<FwdIter>::value_type>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, void> replace_if(
        ExPolicy&& policy, FwdIter first, FwdIter last, Pred&& pred,
        T const& new_value);

    /// Copies the all elements from the range [first, last) to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (last - first)) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          *(first + (i - result)) == old_value
    ///
    /// \note   Complexity: Performs exactly \a last - \a first applications of
    ///         the predicate.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam T           The type of the old and new values (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
    ///
    /// The assignments in the parallel \a replace_copy algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a replace_copy algorithm returns an
    ///           \a OutIter
    ///           The \a replace_copy algorithm returns the
    ///           Iterator to the element past the last element copied.
    ///
    template <typename InIter, typename OutIter,
        typename T = typename std::iterator_traits<OutIter>::value_type>
    OutIter replace_copy(InIter first, InIter last, OutIter dest,
        T const& old_value, T const& new_value);

    /// Copies the all elements from the range [first, last) to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value. Executed according to the policy.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (last - first)) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          *(first + (i - result)) == old_value
    ///
    /// \note   Complexity: Performs exactly \a last - \a first applications of
    ///         the predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam T           The type of the old and new values (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param old_value    Refers to the old value of the elements to replace.
    /// \param new_value    Refers to the new value to use as the replacement.
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
    ///           \a hpx::future<FwdIter2>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2
    ///           otherwise.
    ///           The \a replace_copy algorithm returns the
    ///           Iterator to the element past the last element copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename T = typename std::iterator_traits<FwdIter2>::value_type>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    replace_copy(ExPolicy&& policy, FwdIter1 first, FwdIter1 last,
        FwdIter2 dest, T const& old_value, T const& new_value);

    /// Copies the all elements from the range [first, last) to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (last - first)) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          INVOKE(f, *(first + (i - result))) != false
    ///
    /// \note   Complexity: Performs exactly \a last - \a first applications of
    ///         the predicate.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
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
    ///                     elements which need to replaced. The
    ///                     signature of this predicate should be equivalent
    ///                     to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a InIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param new_value    Refers to the new value to use as the replacement.
    ///
    /// The assignments in the parallel \a replace_copy_if algorithm
    /// execute in sequential order in the calling thread.
    ///
    ///
    /// \returns  The \a replace_copy_if algorithm returns an
    ///           \a OutIter.
    ///           The \a replace_copy_if algorithm returns
    ///           the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename InIter, typename OutIter, typename Pred,
        typename T = typename std::iterator_traits<OutIter>::value_type>
    OutIter replace_copy_if(InIter first, InIter last, OutIter dest,
        Pred&& pred, T const& new_value);

    /// Copies the all elements from the range [first, last) to another range
    /// beginning at \a dest replacing all elements satisfying a specific
    /// criteria with \a new_value.
    ///
    /// Effects: Assigns to every iterator it in the range
    ///          [result, result + (last - first)) either new_value or
    ///          *(first + (it - result)) depending on whether the following
    ///          corresponding condition holds:
    ///          INVOKE(f, *(first + (i - result))) != false
    ///
    /// \note   Complexity: Performs exactly \a last - \a first applications of
    ///         the predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a replace_copy_if requires \a Pred to
    ///                     meet the requirements of \a CopyConstructible.
    ///                     (deduced).
    /// \tparam T           The type of the new values to replace (deduced).
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
    ///                     elements which need to replaced. The
    ///                     signature of this predicate should be equivalent
    ///                     to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param new_value    Refers to the new value to use as the replacement.
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
    /// \returns  The \a replace_copy_if algorithm returns a
    ///           \a hpx::future<FwdIter2>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy
    ///           and returns \a FwdIter2
    ///           otherwise.
    ///           The \a replace_copy_if algorithm returns the
    ///           iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename Pred,
        typename T = typename std::iterator_traits<FwdIter2>::value_type>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    replace_copy_if(ExPolicy&& policy, FwdIter1 first, FwdIter1 last,
        FwdIter2 dest, Pred&& pred, T const& new_value);

}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/replace.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // replace
    namespace detail {
        /// \cond NOINTERNAL

        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct replace : public algorithm<replace<Iter>, Iter>
        {
            constexpr replace() noexcept
              : algorithm<replace, Iter>("replace")
            {
            }

            template <typename ExPolicy, typename InIter, typename T1,
                typename T2, typename Proj>
            static constexpr InIter sequential(ExPolicy&& policy, InIter first,
                InIter last, T1 const& old_value, T2 const& new_value,
                Proj&& proj)
            {
                return sequential_replace<ExPolicy>(
                    HPX_FORWARD(ExPolicy, policy), first, last, old_value,
                    new_value, HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename FwdIter, typename T1,
                typename T2, typename Proj>
            static constexpr util::detail::algorithm_result_t<ExPolicy, FwdIter>
            parallel(ExPolicy&& policy, FwdIter first, FwdIter last,
                T1 const& old_value, T2 const& new_value, Proj&& proj)
            {
                return sequential_replace<ExPolicy>(
                    HPX_FORWARD(ExPolicy, policy), first, last, old_value,
                    new_value, HPX_FORWARD(Proj, proj));
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // replace_if
    namespace detail {
        /// \cond NOINTERNAL

        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct replace_if : public algorithm<replace_if<Iter>, Iter>
        {
            constexpr replace_if() noexcept
              : algorithm<replace_if, Iter>("replace_if")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent,
                typename F, typename T, typename Proj>
            static constexpr InIter sequential(ExPolicy&& policy, InIter first,
                Sent last, F&& f, T const& new_value, Proj&& proj)
            {
                return sequential_replace_if<ExPolicy>(
                    HPX_FORWARD(ExPolicy, policy), first, last,
                    HPX_FORWARD(F, f), new_value, HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename F, typename T, typename Proj>
            static constexpr decltype(auto) parallel(ExPolicy&& policy,
                FwdIter first, Sent last, F&& f, T const& new_value,
                Proj&& proj)
            {
                return sequential_replace_if<ExPolicy>(
                    HPX_FORWARD(ExPolicy, policy), first, last,
                    HPX_FORWARD(F, f), new_value, HPX_FORWARD(Proj, proj));
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // replace_copy
    namespace detail {
        /// \cond NOINTERNAL

        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct replace_copy : public algorithm<replace_copy<IterPair>, IterPair>
        {
            constexpr replace_copy() noexcept
              : algorithm<replace_copy, IterPair>("replace_copy")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent,
                typename OutIter, typename T, typename Proj>
            static constexpr util::in_out_result<InIter, OutIter> sequential(
                ExPolicy&& policy, InIter first, Sent sent, OutIter dest,
                T const& old_value, T const& new_value, Proj&& proj)
            {
                return sequential_replace_copy<ExPolicy>(
                    HPX_FORWARD(ExPolicy, policy), first, sent, dest, old_value,
                    new_value, HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename FwdIter1, typename Sent,
                typename FwdIter2, typename T, typename Proj>
            static constexpr util::detail::algorithm_result_t<ExPolicy,
                util::in_out_result<FwdIter1, FwdIter2>>
            parallel(ExPolicy&& policy, FwdIter1 first, Sent sent,
                FwdIter2 dest, T const& old_value, T const& new_value,
                Proj&& proj)
            {
                return sequential_replace_copy<ExPolicy>(
                    HPX_FORWARD(ExPolicy, policy), first, sent, dest, old_value,
                    new_value, HPX_FORWARD(Proj, proj));
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // replace_copy_if
    namespace detail {
        /// \cond NOINTERNAL

        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct replace_copy_if
          : public algorithm<replace_copy_if<IterPair>, IterPair>
        {
            constexpr replace_copy_if() noexcept
              : algorithm<replace_copy_if, IterPair>("replace_copy_if")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent,
                typename OutIter, typename F, typename T, typename Proj>
            static constexpr util::in_out_result<InIter, OutIter> sequential(
                ExPolicy&& policy, InIter first, Sent sent, OutIter dest, F&& f,
                T const& new_value, Proj&& proj)
            {
                return sequential_replace_copy_if<ExPolicy>(
                    HPX_FORWARD(ExPolicy, policy), first, sent, dest,
                    HPX_FORWARD(F, f), new_value, HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename FwdIter1, typename Sent,
                typename FwdIter2, typename F, typename T, typename Proj>
            static constexpr decltype(auto) parallel(ExPolicy&& policy,
                FwdIter1 first, Sent sent, FwdIter2 dest, F&& f,
                T const& new_value, Proj&& proj)
            {
                return sequential_replace_copy_if<ExPolicy>(
                    HPX_FORWARD(ExPolicy, policy), first, sent, dest,
                    HPX_FORWARD(F, f), new_value, HPX_FORWARD(Proj, proj));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::replace_if
    HPX_CXX_CORE_EXPORT inline constexpr struct replace_if_t final
      : hpx::detail::tag_dispatch<replace_if_t,
            hpx::detail::tag_parallel_algorithm<replace_if_t>>
    {
        template <typename Iter, typename Pred,
            typename T = typename std::iterator_traits<Iter>::value_type>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<Iter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<Iter>::value_type
                >
            )
        // clang-format on
        static void invoke_default(
            Iter first, Iter last, Pred pred, T const& new_value)
        {
            static_assert(
                std::input_iterator<Iter>, "Required at least input iterator.");

            hpx::parallel::detail::replace_if<Iter>().call(
                hpx::execution::sequenced_policy{}, first, last, HPX_MOVE(pred),
                new_value, hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter, typename Pred,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter>::value_type
                >
            )
        // clang-format on
        static decltype(auto) invoke_default(ExPolicy&& policy, FwdIter first,
            FwdIter last, Pred pred, T const& new_value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return parallel::util::detail::algorithm_result<ExPolicy>::get(
                hpx::parallel::detail::replace_if<FwdIter>().call(
                    HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(pred),
                    new_value, hpx::identity_v));
        }
    } replace_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::replace
    HPX_CXX_CORE_EXPORT inline constexpr struct replace_t final
      : hpx::detail::tag_dispatch<replace_t,
            hpx::detail::tag_parallel_algorithm<replace_t>>
    {
        template <typename InIter,
            typename T = typename std::iterator_traits<InIter>::value_type>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter>
            )
        // clang-format on
        static void invoke_default(
            InIter first, InIter last, T const& old_value, T const& new_value)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");

            typedef typename std::iterator_traits<InIter>::value_type Type;

            return hpx::replace_if(
                hpx::execution::seq, first, last,
                [old_value](Type const& a) -> bool { return old_value == a; },
                new_value);
        }

        template <typename ExPolicy, typename FwdIter,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static decltype(auto) invoke_default(ExPolicy&& policy, FwdIter first,
            FwdIter last, T const& old_value, T const& new_value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::replace_if(
                HPX_FORWARD(ExPolicy, policy), first, last,
                [old_value](auto const& a) { return old_value == a; },
                new_value);
        }
    } replace{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::replace_copy_if
    HPX_CXX_CORE_EXPORT inline constexpr struct replace_copy_if_t final
      : hpx::detail::tag_dispatch<replace_copy_if_t,
            hpx::detail::tag_parallel_algorithm<replace_copy_if_t>>
    {
        template <typename InIter, typename OutIter, typename Pred,
            typename T = typename std::iterator_traits<OutIter>::value_type>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter> &&
                hpx::traits::is_iterator_v<OutIter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<InIter>::value_type
                >
            )
        // clang-format on
        static OutIter invoke_default(InIter first, InIter last, OutIter dest,
            Pred pred, T const& new_value)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");

            static_assert(std::output_iterator<OutIter,
                              hpx::traits::iter_value_t<InIter>>,
                "Required at least output iterator.");

            return parallel::util::get_second_element(
                hpx::parallel::detail::replace_copy_if<
                    hpx::parallel::util::in_out_result<InIter, OutIter>>()
                    .call(hpx::execution::sequenced_policy{}, first, last, dest,
                        HPX_MOVE(pred), new_value, hpx::identity_v));
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename Pred,
            typename T = typename std::iterator_traits<FwdIter2>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter1>::value_type
                >
            )
        // clang-format on
        static decltype(auto) invoke_default(ExPolicy&& policy, FwdIter1 first,
            FwdIter1 last, FwdIter2 dest, Pred pred, T const& new_value)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return parallel::util::get_second_element(
                hpx::parallel::detail::replace_copy_if<
                    hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call(HPX_FORWARD(ExPolicy, policy), first, last, dest,
                        HPX_MOVE(pred), new_value, hpx::identity_v));
        }
    } replace_copy_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::replace_copy
    HPX_CXX_CORE_EXPORT inline constexpr struct replace_copy_t final
      : hpx::detail::tag_dispatch<replace_copy_t,
            hpx::detail::tag_parallel_algorithm<replace_copy_t>>
    {
        template <typename InIter, typename OutIter,
            typename T = typename std::iterator_traits<OutIter>::value_type>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter> &&
                hpx::traits::is_iterator_v<OutIter>
            )
        // clang-format on
        static OutIter invoke_default(InIter first, InIter last, OutIter dest,
            T const& old_value, T const& new_value)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");

            static_assert(std::output_iterator<OutIter,
                              hpx::traits::iter_value_t<InIter>>,
                "Required at least output iterator.");

            typedef typename std::iterator_traits<InIter>::value_type Type;

            return hpx::replace_copy_if(
                hpx::execution::seq, first, last, dest,
                [old_value](Type const& a) -> bool { return old_value == a; },
                new_value);
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename T = typename std::iterator_traits<FwdIter2>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        static decltype(auto) invoke_default(ExPolicy&& policy, FwdIter1 first,
            FwdIter1 last, FwdIter2 dest, T const& old_value,
            T const& new_value)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::replace_copy_if(
                HPX_FORWARD(ExPolicy, policy), first, last, dest,
                [old_value](auto const& a) { return old_value == a; },
                new_value);
        }
    } replace_copy{};
}    // namespace hpx

#endif    // DOXYGEN
