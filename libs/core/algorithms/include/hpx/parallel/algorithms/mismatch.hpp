//  Copyright (c) 2022 Bhumit Attarde
//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/mismatch.hpp
/// \page hpx::mismatch
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Returns the first mismatching pair of elements from two ranges: one
    /// defined by [first1, last1) and another defined by [first2,last2). If
    /// last2 is not provided, it denotes first2 + (last1 - first1).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: At most min(last1 - first1, last2 - first2)
    ///         applications of the predicate \a op or \a operator==.
    ///         If \a FwdIter1 and \a FwdIter2 meet the requirements of
    ///         \a RandomAccessIterator and
    ///         (last1 - first1) != (last2 - first2) then no applications
    ///         of the predicate \a op or \a operator== are made.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a mismatch requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
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
    ///
    /// The comparison operations in the parallel \a mismatch algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a mismatch algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note     The two ranges are considered mismatch if, for every iterator
    ///           i in the range [first1,last1), *i mismatches *(first2 + (i - first1)).
    ///           This overload of mismatch uses operator== to determine if two
    ///           elements are mismatch.
    ///
    /// \returns  The \a mismatch algorithm returns a
    ///           \a hpx::future<std::pair<FwdIter1,FwdIter2>>
    ///           if the execution policy is of type \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a std::pair<FwdIter1,FwdIter2> otherwise.
    ///           If no mismatches are found when the comparison reaches last1
    ///           or last2, whichever happens first, the pair holds the end
    ///           iterator and the corresponding iterator from the other range.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename Pred>
    hpx::parallel::util::detail::algorithm_result_t
        <ExPolicy, std::pair<FwdIter1, FwdIter2>>
    mismatch(ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1,
            FwdIter2 first2, FwdIter2 last2, Pred&& op);

    /// Returns the first mismatching pair of elements from two ranges: one
    /// defined by [first1, last1) and another defined by [first2,last2). If
    /// last2 is not provided, it denotes first2 + (last1 - first1).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: At most min(last1 - first1, last2 - first2)
    ///         applications of \a operator==.
    ///         If \a FwdIter1 and \a FwdIter2 meet the requirements of
    ///         \a RandomAccessIterator and
    ///         (last1 - first1) != (last2 - first2) then no applications
    ///         of \a operator== are made.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
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
    ///
    /// The comparison operations in the parallel \a mismatch algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a mismatch algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note     The two ranges are considered mismatch if, for every iterator
    ///           i in the range [first1,last1), *i mismatches *(first2 + (i - first1)).
    ///           This overload of mismatch uses operator== to determine if two
    ///           elements are mismatch.
    ///
    /// \returns  The \a mismatch algorithm returns a
    ///           \a hpx::future<std::pair<FwdIter1,FwdIter2>>
    ///           if the execution policy is of type \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a std::pair<FwdIter1,FwdIter2> otherwise.
    ///           If no mismatches are found when the comparison reaches last1
    ///           or last2, whichever happens first, the pair holds the end
    ///           iterator and the corresponding iterator from the other range.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
    hpx::parallel::util::detail::algorithm_result_t
        <ExPolicy, std::pair<FwdIter1, FwdIter2>>
    mismatch(ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1,
            FwdIter2 first2, FwdIter2 last2);

    /// Returns the first mismatching pair of elements from two ranges: one
    /// defined by [first1, last1) and another defined by [first2,last2). If
    /// last2 is not provided, it denotes first2 + (last1 - first1).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: At most last1 - first1
    ///         applications of the predicate \a op or \a operator==.
    ///         If \a FwdIter1 and \a FwdIter2 meet the requirements of
    ///         \a RandomAccessIterator and
    ///         (last1 - first1) != (last2 - first2) then no applications
    ///         of the predicate \a op or \a operator== are made.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a mismatch requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last1        Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
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
    ///
    /// The comparison operations in the parallel \a mismatch algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a mismatch algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note     The two ranges are considered mismatch if, for every iterator
    ///           i in the range [first1,last1), *i mismatches *(first2 + (i - first1)).
    ///           This overload of mismatch uses operator== to determine if two
    ///           elements are mismatch.
    ///
    /// \returns  The \a mismatch algorithm returns a
    ///           \a hpx::future<std::pair<FwdIter1,FwdIter2>>
    ///           if the execution policy is of type \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a std::pair<FwdIter1,FwdIter2> otherwise.
    ///           If no mismatches are found when the comparison reaches last1
    ///           or last2, whichever happens first, the pair holds the end
    ///           iterator and the corresponding iterator from the other range.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename Pred>
    hpx::parallel::util::detail::algorithm_result_t
        <ExPolicy, std::pair<FwdIter1, FwdIter2>>
    mismatch(ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1,
            FwdIter2 first2, Pred&& op);

    /// Returns the first mismatching pair of elements from two ranges: one
    /// defined by [first1, last1) and another defined by [first2,last2). If
    /// last2 is not provided, it denotes first2 + (last1 - first1).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: At most last1 - first1
    ///         applications of \a operator==.
    ///         If \a FwdIter1 and \a FwdIter2 meet the requirements of
    ///         \a RandomAccessIterator and
    ///         (last1 - first1) != (last2 - first2) then no applications
    ///         of \a operator== are made.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a mismatch requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last1        Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
    ///
    /// The comparison operations in the parallel \a mismatch algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a mismatch algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note     The two ranges are considered mismatch if, for every iterator
    ///           i in the range [first1,last1), *i mismatches *(first2 + (i - first1)).
    ///           This overload of mismatch uses operator== to determine if two
    ///           elements are mismatch.
    ///
    /// \returns  The \a mismatch algorithm returns a
    ///           \a hpx::future<std::pair<FwdIter1,FwdIter2>>
    ///           if the execution policy is of type \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a std::pair<FwdIter1,FwdIter2> otherwise.
    ///           If no mismatches are found when the comparison reaches last1
    ///           or last2, whichever happens first, the pair holds the end
    ///           iterator and the corresponding iterator from the other range.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
    hpx::parallel::util::detail::algorithm_result_t
        <ExPolicy, std::pair<FwdIter1, FwdIter2>>
    mismatch(ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1,
            FwdIter2 first2);

    /// Returns the first mismatching pair of elements from two ranges: one
    /// defined by [first1, last1) and another defined by [first2,last2). If
    /// last2 is not provided, it denotes first2 + (last1 - first1).
    ///
    /// \note   Complexity: At most min(last1 - first1, last2 - first2)
    ///         applications of the predicate \a op or \a operator==.
    ///         If \a FwdIter1 and \a FwdIter2 meet the requirements of
    ///         \a RandomAccessIterator and
    ///         (last1 - first1) != (last2 - first2) then no applications
    ///         of the predicate \a op or \a operator== are made.
    ///
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a mismatch requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
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
    ///
    /// \note     The two ranges are considered mismatch if, for every iterator
    ///           i in the range [first1,last1), *i mismatches *(first2 + (i - first1)).
    ///           This overload of mismatch uses operator== to determine if two
    ///           elements are mismatch.
    ///
    /// \returns  The \a mismatch algorithm returns a
    ///           \a std::pair<FwdIter1,FwdIter2>.
    ///           If no mismatches are found when the comparison reaches last1
    ///           or last2, whichever happens first, the pair holds the end
    ///           iterator and the corresponding iterator from the other range.
    ///
    template <typename FwdIter1, typename FwdIter2,
        typename Pred>
    std::pair<FwdIter1, FwdIter2> mismatch(FwdIter1 first1, FwdIter1 last1,
            FwdIter2 first2, FwdIter2 last2, Pred&& op);

    /// Returns the first mismatching pair of elements from two ranges: one
    /// defined by [first1, last1) and another defined by [first2,last2). If
    /// last2 is not provided, it denotes first2 + (last1 - first1).
    ///
    /// \note   Complexity: At most min(last1 - first1, last2 - first2)
    ///         applications of \a operator==.
    ///         If \a FwdIter1 and \a FwdIter2 meet the requirements of
    ///         \a RandomAccessIterator and
    ///         (last1 - first1) != (last2 - first2) then no applications
    ///         of \a operator== are made.
    ///
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last1        Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
    /// \param last2        Refers to the end of the sequence of elements of
    ///                     the second range the algorithm will be applied to.
    ///
    /// \note     The two ranges are considered mismatch if, for every iterator
    ///           i in the range [first1,last1), *i mismatches *(first2 + (i - first1)).
    ///           This overload of mismatch uses operator== to determine if two
    ///           elements are mismatch.
    ///
    /// \returns  The \a mismatch algorithm returns a
    ///           \a std::pair<FwdIter1,FwdIter2>.
    ///           If no mismatches are found when the comparison reaches last1
    ///           or last2, whichever happens first, the pair holds the end
    ///           iterator and the corresponding iterator from the other range.
    ///
    template <typename FwdIter1, typename FwdIter2>
    std::pair<FwdIter1, FwdIter2> mismatch(FwdIter1 first1, FwdIter1 last1,
            FwdIter2 first2, FwdIter2 last2);

    /// Returns the first mismatching pair of elements from two ranges: one
    /// defined by [first1, last1) and another defined by [first2,last2). If
    /// last2 is not provided, it denotes first2 + (last1 - first1).
    ///
    /// \note   Complexity: At most last1 - first1
    ///         applications of the predicate \a op or \a operator==.
    ///         If \a FwdIter1 and \a FwdIter2 meet the requirements of
    ///         \a RandomAccessIterator and
    ///         (last1 - first1) != (last2 - first2) then no applications
    ///         of the predicate \a op or \a operator== are made.
    ///
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a mismatch requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    ///
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last1        Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
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
    ///
    /// \note     The two ranges are considered mismatch if, for every iterator
    ///           i in the range [first1,last1), *i mismatches *(first2 + (i - first1)).
    ///           This overload of mismatch uses operator== to determine if two
    ///           elements are mismatch.
    ///
    /// \returns  The \a mismatch algorithm returns a
    ///           \a std::pair<FwdIter1,FwdIter2>.
    ///           If no mismatches are found when the comparison reaches last1
    ///           or last2, whichever happens first, the pair holds the end
    ///           iterator and the corresponding iterator from the other range.
    ///
    template <typename FwdIter1, typename FwdIter2,
        typename Pred>
    std::pair<FwdIter1, FwdIter2> mismatch(FwdIter1 first1, FwdIter1 last1,
            FwdIter2 first2, Pred&& op);

    /// Returns the first mismatching pair of elements from two ranges: one
    /// defined by [first1, last1) and another defined by [first2,last2). If
    /// last2 is not provided, it denotes first2 + (last1 - first1).
    ///
    /// \note   Complexity: At most last1 - first1
    ///         applications of \a operator==.
    ///         If \a FwdIter1 and \a FwdIter2 meet the requirements of
    ///         \a RandomAccessIterator and
    ///         (last1 - first1) != (last2 - first2) then no applications
    ///         of \a operator== are made.
    ///
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a mismatch requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    ///
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last1        Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
    ///
    /// \note     The two ranges are considered mismatch if, for every iterator
    ///           i in the range [first1,last1), *i mismatches *(first2 + (i - first1)).
    ///           This overload of mismatch uses operator== to determine if two
    ///           elements are mismatch.
    ///
    /// \returns  The \a mismatch algorithm returns a
    ///           \a std::pair<FwdIter1,FwdIter2>.
    ///           If no mismatches are found when the comparison reaches last1
    ///           or last2, whichever happens first, the pair holds the end
    ///           iterator and the corresponding iterator from the other range.
    ///
    template <typename FwdIter1, typename FwdIter2>
    std::pair<FwdIter1, FwdIter2> mismatch(FwdIter1 first1,
        FwdIter1 last1, FwdIter2 first2);

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/advance_to_sentinel.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/mismatch.hpp>
#include <hpx/parallel/util/adapt_placement_mode.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // mismatch (binary)
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct mismatch_binary
          : public algorithm<mismatch_binary<IterPair>, IterPair>
        {
            constexpr mismatch_binary() noexcept
              : algorithm<mismatch_binary, IterPair>("mismatch_binary")
            {
            }

            template <typename ExPolicy, typename Iter1, typename Sent1,
                typename Iter2, typename Sent2, typename F, typename Proj1,
                typename Proj2>
            static constexpr util::in_in_result<Iter1, Iter2> sequential(
                ExPolicy, Iter1 first1, Sent1 last1, Iter2 first2, Sent2 last2,
                F&& f, Proj1&& proj1, Proj2&& proj2)
            {
                return sequential_mismatch_binary<std::decay_t<ExPolicy>>(
                    first1, last1, first2, last2, HPX_FORWARD(F, f),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
            }

            template <typename ExPolicy, typename Iter1, typename Sent1,
                typename Iter2, typename Sent2, typename F, typename Proj1,
                typename Proj2>
            static decltype(auto) parallel(ExPolicy&& orgpolicy, Iter1 first1,
                Sent1 last1, Iter2 first2, Sent2 last2, F&& f, Proj1&& proj1,
                Proj2&& proj2)
            {
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first1 == last1 || first2 == last2)
                    {
                        return util::detail::algorithm_result<ExPolicy,
                            util::in_in_result<Iter1, Iter2>>::get(util::
                                in_in_result<Iter1, Iter2>{first1, first2});
                    }
                }

                auto count1 = detail::distance(first1, last1);
                auto count2 = detail::distance(first2, last2);
                auto count = (std::min) (count1, count2);

                using zip_iterator = hpx::util::zip_iterator<Iter1, Iter2>;

                decltype(auto) policy =
                    hpx::execution::experimental::adapt_placement_mode(
                        HPX_FORWARD(ExPolicy, orgpolicy),
                        hpx::threads::thread_placement_hint::breadth_first);

                using policy_type = std::decay_t<decltype(policy)>;

                hpx::parallel::util::cancellation_token<std::size_t> tok(count);

                auto f1 = [tok, f = HPX_FORWARD(F, f),
                              proj1 = HPX_FORWARD(Proj1, proj1),
                              proj2 = HPX_FORWARD(Proj2, proj2)](
                              zip_iterator it, std::size_t part_count,
                              std::size_t base_idx) mutable -> void {
                    sequential_mismatch_binary<policy_type>(base_idx, it,
                        part_count, tok, HPX_FORWARD(F, f),
                        HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
                };

                auto f2 = [=](auto&&... data) mutable
                    -> util::in_in_result<Iter1, Iter2> {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in function object that is
                    // attached to futures are invalidated
                    util::detail::clear_container(data...);

                    auto mismatched = tok.get_data();
                    std::advance(first1, mismatched);
                    std::advance(first2, mismatched);

                    return {first1, first2};
                };

                using partitioner_type = util::partitioner<policy_type,
                    util::in_in_result<Iter1, Iter2>, void>;

                return partitioner_type::call_with_index(
                    HPX_FORWARD(decltype(policy), policy),
                    zip_iterator(first1, first2), count, 1, HPX_MOVE(f1),
                    HPX_MOVE(f2));
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename I1, typename I2>
        constexpr std::pair<I1, I2> get_pair(
            util::in_in_result<I1, I2>&& p) noexcept
        {
            return {HPX_MOVE(p.in1), HPX_MOVE(p.in2)};
        }

        HPX_CXX_CORE_EXPORT template <typename InInSender>
            requires(hpx::execution::experimental::is_sender_v<InInSender>)
        decltype(auto) get_pair(InInSender&& in_in_sender) noexcept
        {
            return hpx::execution::experimental::then(
                HPX_FORWARD(InInSender, in_in_sender), [](auto&& in_in_result) {
                    return get_pair(
                        HPX_FORWARD(decltype(in_in_result), in_in_result));
                });
        }

        HPX_CXX_CORE_EXPORT template <typename I1, typename I2>
        hpx::future<std::pair<I1, I2>> get_pair(
            hpx::future<util::in_in_result<I1, I2>>&& f) noexcept
        {
            return hpx::make_future<std::pair<I1, I2>>(HPX_MOVE(f),
                [](util::in_in_result<I1, I2>&& p) -> std::pair<I1, I2> {
                    return {HPX_MOVE(p.in1), HPX_MOVE(p.in2)};
                });
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // mismatch
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct mismatch : public algorithm<mismatch<IterPair>, IterPair>
        {
            constexpr mismatch() noexcept
              : algorithm<mismatch, IterPair>("mismatch")
            {
            }

            template <typename ExPolicy, typename InIter1, typename Sent,
                typename InIter2, typename F>
            static constexpr IterPair sequential(
                ExPolicy, InIter1 first1, Sent last1, InIter2 first2, F&& f)
            {
                return sequential_mismatch<std::decay_t<ExPolicy>>(
                    first1, last1, first2, HPX_FORWARD(F, f));
            }

            template <typename ExPolicy, typename FwdIter1, typename Sent,
                typename FwdIter2, typename F>
            static decltype(auto) parallel(ExPolicy&& orgpolicy,
                FwdIter1 first1, Sent last1, FwdIter2 first2, F&& f)
            {
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first1 == last1)
                    {
                        return util::detail::algorithm_result<ExPolicy,
                            IterPair>::get(std::make_pair(first1, first2));
                    }
                }

                using difference_type =
                    typename std::iterator_traits<FwdIter1>::difference_type;
                difference_type count = detail::distance(first1, last1);

                using zip_iterator =
                    hpx::util::zip_iterator<FwdIter1, FwdIter2>;

                decltype(auto) policy =
                    hpx::execution::experimental::adapt_placement_mode(
                        HPX_FORWARD(ExPolicy, orgpolicy),
                        hpx::threads::thread_placement_hint::breadth_first);

                using policy_type = std::decay_t<decltype(policy)>;

                hpx::parallel::util::cancellation_token<std::size_t> tok(count);

                auto f1 = [tok, f = HPX_FORWARD(F, f)](zip_iterator it,
                              std::size_t part_count,
                              std::size_t base_idx) mutable -> void {
                    sequential_mismatch<policy_type>(
                        base_idx, it, part_count, tok, HPX_FORWARD(F, f));
                };

                auto f2 = [=](auto&&... data) mutable
                    -> std::pair<FwdIter1, FwdIter2> {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in function object that is
                    // attached to futures are invalidated
                    util::detail::clear_container(data...);

                    difference_type mismatched =
                        static_cast<difference_type>(tok.get_data());
                    std::advance(first1, mismatched);

                    std::advance(first2, mismatched);
                    return std::make_pair(first1, first2);
                };

                using partitioner_type =
                    util::partitioner<policy_type, IterPair, void>;
                return partitioner_type::call_with_index(
                    HPX_FORWARD(decltype(policy), policy),
                    zip_iterator(first1, first2), count, 1, HPX_MOVE(f1),
                    HPX_MOVE(f2));
            }
        };
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::mismatch
    HPX_CXX_CORE_EXPORT inline constexpr struct mismatch_t final
      : hpx::detail::tag_parallel_algorithm<mismatch_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename Pred>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter1>::value_type,
                    typename std::iterator_traits<FwdIter2>::value_type
                >
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(mismatch_t, ExPolicy&& policy,
            FwdIter1 first1, FwdIter1 last1, FwdIter2 first2, FwdIter2 last2,
            Pred op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::get_pair(
                hpx::parallel::detail::mismatch_binary<
                    hpx::parallel::util::in_in_result<FwdIter1, FwdIter2>>()
                    .call(HPX_FORWARD(ExPolicy, policy), first1, last1, first2,
                        last2, HPX_MOVE(op), hpx::identity_v, hpx::identity_v));
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(mismatch_t, ExPolicy&& policy,
            FwdIter1 first1, FwdIter1 last1, FwdIter2 first2, FwdIter2 last2)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::get_pair(
                hpx::parallel::detail::mismatch_binary<
                    hpx::parallel::util::in_in_result<FwdIter1, FwdIter2>>()
                    .call(HPX_FORWARD(ExPolicy, policy), first1, last1, first2,
                        last2, hpx::parallel::detail::equal_to{},
                        hpx::identity_v, hpx::identity_v));
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename Pred>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter1>::value_type,
                    typename std::iterator_traits<FwdIter2>::value_type
                >
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(mismatch_t, ExPolicy&& policy,
            FwdIter1 first1, FwdIter1 last1, FwdIter2 first2, Pred op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::mismatch<
                std::pair<FwdIter1, FwdIter2>>()
                .call(HPX_FORWARD(ExPolicy, policy), first1, last1, first2,
                    HPX_MOVE(op));
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(mismatch_t, ExPolicy&& policy,
            FwdIter1 first1, FwdIter1 last1, FwdIter2 first2)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::mismatch<
                std::pair<FwdIter1, FwdIter2>>()
                .call(HPX_FORWARD(ExPolicy, policy), first1, last1, first2,
                    hpx::parallel::detail::equal_to{});
        }

        template <typename FwdIter1, typename FwdIter2, typename Pred>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter1>::value_type,
                    typename std::iterator_traits<FwdIter2>::value_type
                >
            )
        // clang-format on
        friend std::pair<FwdIter1, FwdIter2> tag_fallback_invoke(mismatch_t,
            FwdIter1 first1, FwdIter1 last1, FwdIter2 first2, FwdIter2 last2,
            Pred op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::get_pair(
                hpx::parallel::detail::mismatch_binary<
                    hpx::parallel::util::in_in_result<FwdIter1, FwdIter2>>()
                    .call(hpx::execution::seq, first1, last1, first2, last2,
                        HPX_MOVE(op), hpx::identity_v, hpx::identity_v));
        }

        template <typename FwdIter1, typename FwdIter2>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        friend std::pair<FwdIter1, FwdIter2> tag_fallback_invoke(mismatch_t,
            FwdIter1 first1, FwdIter1 last1, FwdIter2 first2, FwdIter2 last2)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::get_pair(
                hpx::parallel::detail::mismatch_binary<
                    hpx::parallel::util::in_in_result<FwdIter1, FwdIter2>>()
                    .call(hpx::execution::seq, first1, last1, first2, last2,
                        hpx::parallel::detail::equal_to{}, hpx::identity_v,
                        hpx::identity_v));
        }

        template <typename FwdIter1, typename FwdIter2, typename Pred>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter1>::value_type,
                    typename std::iterator_traits<FwdIter2>::value_type
                >
            )
        // clang-format on
        friend std::pair<FwdIter1, FwdIter2> tag_fallback_invoke(mismatch_t,
            FwdIter1 first1, FwdIter1 last1, FwdIter2 first2, Pred op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::mismatch<
                std::pair<FwdIter1, FwdIter2>>()
                .call(hpx::execution::seq, first1, last1, first2, HPX_MOVE(op));
        }

        template <typename FwdIter1, typename FwdIter2>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        friend std::pair<FwdIter1, FwdIter2> tag_fallback_invoke(
            mismatch_t, FwdIter1 first1, FwdIter1 last1, FwdIter2 first2)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::mismatch<
                std::pair<FwdIter1, FwdIter2>>()
                .call(hpx::execution::seq, first1, last1, first2,
                    hpx::parallel::detail::equal_to{});
        }
    } mismatch{};
}    // namespace hpx

#endif    // DOXYGEN
