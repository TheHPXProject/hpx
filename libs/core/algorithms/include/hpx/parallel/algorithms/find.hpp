//  Copyright (c) 2022 Bhumit Attarde
//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2017-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/find.hpp
/// \page hpx::find, hpx::find_if, hpx::find_if_not, hpx::find_end, hpx::find_first_of
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Returns the first element in the range [first, last) that is equal
    /// to value. Executed according to the policy.
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the operator==().
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam T           The type of the value to find (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param val          the value to compare the elements to
    ///
    /// The comparison operations in the parallel \a find algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a find algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a find algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a find algorithm returns the first element in the range
    ///           [first,last) that is equal to \a val.
    ///           If no such element in the range of [first,last) is equal to
    ///           \a val, then the algorithm returns \a last.
    ///
    template <typename ExPolicy, typename FwdIter, typename T>
    util::detail::algorithm_result_t<ExPolicy, FwdIter>
    find(ExPolicy&& policy, FwdIter first, FwdIter last, T const& val);

    /// Returns the first element in the range [first, last) that is equal
    /// to value. Executed according to the policy.
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the operator==().
    ///
    /// \tparam InIter      The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam T           The type of the value to find (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param val          the value to compare the elements to
    ///
    /// \returns  The \a find algorithm returns a \a InIter.
    ///           The \a find algorithm returns the first element in the range
    ///           [first,last) that is equal to \a val.
    ///           If no such element in the range of [first,last) is equal to
    ///           \a val, then the algorithm returns \a last.
    ///
    template <typename InIter, typename T>
    InIter find(InIter first, InIter last, T const& val);

    /// Returns the first element in the range [first, last) for which
    /// predicate \a f returns true. Executed according to the policy.
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param f            The unary predicate which returns true for the
    ///                     required element. The signature of the predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such
    ///                     that objects of type \a FwdIter can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type.
    ///
    /// The comparison operations in the parallel \a find_if algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a find_if algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a find_if algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a find_if algorithm returns the first element in the range
    ///           [first,last) that satisfies the predicate \a f.
    ///           If no such element exists that satisfies the predicate f, the
    ///           algorithm returns \a last.
    ///
    template <typename ExPolicy, typename FwdIter, typename F>
    util::detail::algorithm_result_t<ExPolicy, FwdIter>
    find_if(ExPolicy&& policy, FwdIter first, FwdIter last, F&& f);

    /// Returns the first element in the range [first, last) for which
    /// predicate \a f returns true.
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam InIter      The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param f            The unary predicate which returns true for the
    ///                     required element. The signature of the predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such
    ///                     that objects of type \a InIter can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type.
    ///
    /// \returns  The \a find_if algorithm returns a \a InIter.
    ///           The \a find_if algorithm returns the first element in the range
    ///           [first,last) that satisfies the predicate \a f.
    ///           If no such element exists that satisfies the predicate f, the
    ///           algorithm returns \a last.
    ///
    template <typename InIter, typename F>
    InIter find_if(InIter first, InIter last, F&& f);

    /// Returns the first element in the range [first, last) for which
    /// predicate \a f returns false. Executed according to the policy.
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param f            The unary predicate which returns false for the
    ///                     required element. The signature of the predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such
    ///                     that objects of type \a FwdIter can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type.
    ///
    /// The comparison operations in the parallel \a find_if_not algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a find_if_not algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a find_if_not algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a find_if_not algorithm returns the first element in the range
    ///           [first, last) that does \b not satisfy the predicate \a f.
    ///           If no such element exists that does not satisfy the predicate f, the
    ///           algorithm returns \a last.
    ///
    template <typename ExPolicy, typename FwdIter, typename F>
    util::detail::algorithm_result_t<ExPolicy, FwdIter>
    find_if_not(ExPolicy&& policy, FwdIter first, FwdIter last, F&& f);

    /// Returns the first element in the range [first, last) for which
    /// predicate \a f returns false.
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param f            The unary predicate which returns false for the
    ///                     required element. The signature of the predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such
    ///                     that objects of type \a FwdIter can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type.
    ///
    /// \returns  The \a find_if_not algorithm returns a \a FwdIter.
    ///           The \a find_if_not algorithm returns the first element in the range
    ///           [first, last) that does \b not satisfy the predicate \a f.
    ///           If no such element exists that does not satisfy the predicate f, the
    ///           algorithm returns \a last.
    ///
    template <typename FwdIter, typename F>
    FwdIter find_if_not(FwdIter first, FwdIter last, F&& f);

    /// Returns the last subsequence of elements [first2, last2) found in the range
    /// [first, last) using the given predicate \a op to compare elements. Executed
    /// according to the policy.
    ///
    /// \note   Complexity: at most S*(N-S+1) comparisons where
    ///         \a S = distance(first2, last2) and
    ///         \a N = distance(first1, last1).
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
    ///                     overload of \a replace requires \a Pred to meet the
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
    ///                     the algorithm will be searching for.
    /// \param last2        Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    /// \param op           The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The signature
    ///                     should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 and \a Type2 respectively.
    ///
    /// The comparison operations in the parallel \a find_end algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a find_end algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a find_end algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a find_end algorithm returns an iterator to the beginning of
    ///           the last subsequence [first2, last2) in range [first, last).
    ///           If the length of the subsequence [first2, last2) is greater
    ///           than the length of the range [first1, last1), \a last1 is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a last1 is also returned.
    ///
    /// This overload of \a find_end is available if the user decides to provide the
    /// algorithm their own predicate \a op.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename Pred = detail::equal_to>
    util::detail::algorithm_result_t<ExPolicy, FwdIter1>
    find_end(ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1,
        FwdIter2 first2, FwdIter2 last2, Pred&& op = Pred());

    /// Returns the last subsequence of elements [first2, last2) found in the range
    /// [first, last). Elements are compared using \a operator==. Executed according
    /// to the policy.
    ///
    /// \note   Complexity: at most S*(N-S+1) comparisons where
    ///         \a S = distance(first2, last2) and
    ///         \a N = distance(first1, last1).
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
    ///                     the algorithm will be searching for.
    /// \param last2        Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    ///
    /// The comparison operations in the parallel \a find_end algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a find_end algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a find_end algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a find_end algorithm returns an iterator to the beginning of
    ///           the last subsequence [first2, last2) in range [first, last).
    ///           If the length of the subsequence [first2, last2) is greater
    ///           than the length of the range [first1, last1), \a last1 is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a last1 is also returned.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
    util::detail::algorithm_result_t<ExPolicy, FwdIter1>
    find_end(ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1,
        FwdIter2 first2, FwdIter2 last2);

    /// Returns the last subsequence of elements [first2, last2) found in the range
    /// [first, last) using the given predicate \a op to compare elements.
    ///
    /// \note   Complexity: at most S*(N-S+1) comparisons where
    ///         \a S = distance(first2, last2) and
    ///         \a N = distance(first1, last1).
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
    ///                     overload of \a replace requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    ///
    /// \param first1       Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last1        Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param last2        Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    /// \param op           The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The signature
    ///                     should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 and \a Type2 respectively.
    ///
    /// \returns  The \a find_end algorithm returns a \a FwdIter1.
    ///           The \a find_end algorithm returns an iterator to the beginning of
    ///           the last subsequence [first2, last2) in range [first, last).
    ///           If the length of the subsequence [first2, last2) is greater
    ///           than the length of the range [first1, last1), \a last1 is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a last1 is also returned.
    ///
    /// This overload of \a find_end is available if the user decides to provide the
    /// algorithm their own predicate \a op.
    ///
    template <typename FwdIter1, typename FwdIter2,
        typename Pred = detail::equal_to>
    FwdIter1 find_end(FwdIter1 first1, FwdIter1 last1,
        FwdIter2 first2, FwdIter2 last2, Pred&& op = Pred());

    /// Returns the last subsequence of elements [first2, last2) found in the range
    /// [first, last). Elements are compared using \a operator==.
    ///
    /// \note   Complexity: at most S*(N-S+1) comparisons where
    ///         \a S = distance(first2, last2) and
    ///         \a N = distance(first1, last1).
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
    ///                     the algorithm will be searching for.
    /// \param last2        Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    ///
    /// \returns  The \a find_end algorithm returns a \a FwdIter1.
    ///           The \a find_end algorithm returns an iterator to the beginning of
    ///           the last subsequence [first2, last2) in range [first, last).
    ///           If the length of the subsequence [first2, last2) is greater
    ///           than the length of the range [first1, last1), \a last1 is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a last1 is also returned.
    ///
    template <typename FwdIter1, typename FwdIter2>
    FwdIter1 find_end(FwdIter1 first1, FwdIter1 last1, FwdIter2 first2, FwdIter2 last2);

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses binary predicate \a op to compare elements. Executed according to the policy.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward  iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param s_first      Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param s_last       Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    /// \param op           The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The signature
    ///                     should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 and \a Type2 respectively.
    ///
    /// The comparison operations in the parallel \a find_first_of algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a find_first_of algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a find_first_of algorithm returns a \a hpx::future<FwdIter1> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter1 otherwise.
    ///           The \a find_first_of algorithm returns an iterator to the first element
    ///           in the range [first, last) that is equal to an element from the range
    ///           [s_first, s_last).
    ///           If the length of the subsequence [s_first, s_last) is
    ///           greater than the length of the range [first, last),
    ///           \a last is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a last is also returned.
    ///
    /// This overload of \a find_first_of is available if
    /// the user decides to provide the
    /// algorithm their own predicate \a op.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename Pred = detail::equal_to>
    util::detail::algorithm_result_t<ExPolicy, FwdIter1>
    find_first_of(ExPolicy&& policy, FwdIter1 first, FwdIter1 last,
        FwdIter2 s_first, FwdIter2 s_last, Pred&& op = Pred());

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Elements are compared using \a operator==. Executed according to the policy.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward  iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param s_first      Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param s_last       Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    ///
    /// The comparison operations in the parallel \a find_first_of algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a find_first_of algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a find_first_of algorithm returns a \a hpx::future<FwdIter1> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter1 otherwise.
    ///           The \a find_first_of algorithm returns an iterator to the first element
    ///           in the range [first, last) that is equal to an element from the range
    ///           [s_first, s_last).
    ///           If the length of the subsequence [s_first, s_last) is
    ///           greater than the length of the range [first, last),
    ///           \a last is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a last is also returned.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
    util::detail::algorithm_result_t<ExPolicy, FwdIter1>
    find_first_of(ExPolicy&& policy, FwdIter1 first, FwdIter1 last,
        FwdIter2 s_first, FwdIter2 s_last);

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses binary predicate \a op to compare elements.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward  iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param s_first      Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param s_last       Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    /// \param op           The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The signature
    ///                     should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 and \a Type2 respectively.
    ///
    /// \returns  The \a find_first_of algorithm returns a \a FwdIter1.
    ///           The \a find_first_of algorithm returns an iterator to the first element
    ///           in the range [first, last) that is equal to an element from the range
    ///           [s_first, s_last).
    ///           If the length of the subsequence [s_first, s_last) is
    ///           greater than the length of the range [first, last),
    ///           \a last is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a last is also returned.
    ///
    /// This overload of \a find_first_of is available if
    /// the user decides to provide the
    /// algorithm their own predicate \a op.
    ///
    template <typename FwdIter1, typename FwdIter2, typename Pred = detail::equal_to>
    FwdIter1 find_first_of(FwdIter1 first, FwdIter1 last,
        FwdIter2 s_first, FwdIter2 s_last, Pred&& op = Pred());

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Elements are compared using \a operator==.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward  iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param s_first      Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param s_last       Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    ///
    /// \returns  The \a find_first_of algorithm returns a \a FwdIter1.
    ///           The \a find_first_of algorithm returns an iterator to the first element
    ///           in the range [first, last) that is equal to an element from the range
    ///           [s_first, s_last).
    ///           If the length of the subsequence [s_first, s_last) is
    ///           greater than the length of the range [first, last),
    ///           \a last is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a last is also returned.
    ///
    template <typename FwdIter1, typename FwdIter2>
    FwdIter1 find_first_of(FwdIter1 first, FwdIter1 last,
        FwdIter2 s_first, FwdIter2 s_last);

    /// Returns the last element in the range [first, last) that is equal to \a val.
    /// Executed according to the policy.
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         operator \a ==.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam BidiIter    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     bidirectional iterator.
    /// \tparam T           The type of the value to find (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param val          The value to find.
    ///
    /// The comparison operations in the parallel \a find_last algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a find_last algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a find_last algorithm returns a \a hpx::future<BidiIter> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a BidiIter otherwise.
    ///           The \a find_last algorithm returns an iterator to the last element
    ///           in the range [first, last) that is equal to \a val.
    ///           If no such element is found, the function returns \a last.
    ///
    template <typename ExPolicy, typename BidiIter, typename T>
    util::detail::algorithm_result_t<ExPolicy, BidiIter>
    find_last(ExPolicy&& policy, BidiIter first, BidiIter last, T const& val);

    /// Returns the last element in the range [first, last) that is equal to \a val.
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         operator \a ==.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam T           The type of the value to find (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param val          The value to find.
    ///
    /// \returns  The \a find_last algorithm returns a \a InIter.
    ///           The \a find_last algorithm returns an iterator to the last element
    ///           in the range [first, last) that is equal to \a val.
    ///           If no such element is found, the function returns \a last.
    ///
    template <typename InIter, typename T>
    InIter find_last(InIter first, InIter last, T const& val);

    /// Returns the last element in the range [first, last) for which
    /// predicate \a f returns true. Executed according to the policy.
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam BidiIter    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     bidirectional iterator.
    /// \tparam F           The type of the function/predicate to use (deduced).
    ///                     Must be callable with the type of the elements in the range.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            The predicate to check.
    ///
    /// The comparison operations in the parallel \a find_last_if algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a find_last_if algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a find_last_if algorithm returns a \a hpx::future<BidiIter> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a BidiIter otherwise.
    ///           The \a find_last_if algorithm returns an iterator to the last element
    ///           in the range [first, last) for which predicate \a f returns true.
    ///           If no such element is found, the function returns \a last.
    ///
    template <typename ExPolicy, typename BidiIter, typename F>
    util::detail::algorithm_result_t<ExPolicy, BidiIter>
    find_last_if(ExPolicy&& policy, BidiIter first, BidiIter last, F&& f);

    /// Returns the last element in the range [first, last) for which
    /// predicate \a f returns true.
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         predicate.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam F           The type of the function/predicate to use (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            The predicate to check.
    ///
    /// \returns  The \a find_last_if algorithm returns a \a InIter.
    ///           The \a find_last_if algorithm returns an iterator to the last element
    ///           in the range [first, last) for which predicate \a f returns true.
    ///           If no such element is found, the function returns \a last.
    ///
    template <typename InIter, typename F>
    InIter find_last_if(InIter first, InIter last, F&& f);

    /// Returns the last element in the range [first, last) for which
    /// predicate \a f returns false. Executed according to the policy.
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam BidiIter    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     bidirectional iterator.
    /// \tparam F           The type of the function/predicate to use (deduced).
    ///                     Must be callable with the type of the elements in the range.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            The predicate to check.
    ///
    /// The comparison operations in the parallel \a find_last_if_not algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a find_last_if_not algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a find_last_if_not algorithm returns a \a hpx::future<BidiIter> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a BidiIter otherwise.
    ///           The \a find_last_if_not algorithm returns an iterator to the last element
    ///           in the range [first, last) for which predicate \a f returns false.
    ///           If no such element is found, the function returns \a last.
    ///
    template <typename ExPolicy, typename BidiIter, typename F>
    util::detail::algorithm_result_t<ExPolicy, BidiIter>
    find_last_if_not(ExPolicy&& policy, BidiIter first, BidiIter last, F&& f);

    /// Returns the last element in the range [first, last) for which
    /// predicate \a f returns false.
    ///
    /// \note   Complexity: At most \a last - \a first applications of the
    ///         predicate.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam F           The type of the function/predicate to use (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            The predicate to check.
    ///
    /// \returns  The \a find_last_if_not algorithm returns a \a InIter.
    ///           The \a find_last_if_not algorithm returns an iterator to the last element
    ///           in the range [first, last) for which predicate \a f returns false.
    ///           If no such element is found, the function returns \a last.
    ///
    template <typename InIter, typename F>
    InIter find_last_if_not(InIter first, InIter last, F&& f);
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/advance_to_sentinel.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/find.hpp>
#include <hpx/parallel/util/adapt_placement_mode.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <iterator>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // find
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename FwdIter>
        struct find : public algorithm<find<FwdIter>, FwdIter>
        {
            constexpr find() noexcept
              : algorithm<find, FwdIter>("find")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename T, typename Proj = hpx::identity>
            static constexpr Iter sequential(ExPolicy, Iter first, Sent last,
                T const& val, Proj&& proj = Proj())
            {
                return sequential_find<ExPolicy>(
                    first, last, val, HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename T, typename Proj = hpx::identity>
            static decltype(auto) parallel(ExPolicy&& orgpolicy, Iter first,
                Sent last, T const& val, Proj&& proj = Proj())
            {
                using result = util::detail::algorithm_result<ExPolicy, Iter>;
                using difference_type =
                    typename std::iterator_traits<Iter>::difference_type;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return result::get(HPX_MOVE(first));
                    }
                }

                difference_type count = detail::distance(first, last);

                decltype(auto) policy =
                    hpx::execution::experimental::adapt_placement_mode(
                        HPX_FORWARD(ExPolicy, orgpolicy),
                        hpx::threads::thread_placement_hint::breadth_first);

                using policy_type = std::decay_t<decltype(policy)>;

                util::cancellation_token<std::size_t> tok(count);

                auto f1 = [val, proj = HPX_FORWARD(Proj, proj), tok](Iter it,
                              std::size_t part_size,
                              std::size_t base_idx) mutable -> void {
                    sequential_find<policy_type>(base_idx, it, part_size, tok,
                        val, HPX_FORWARD(Proj, proj));
                };

                auto f2 = [tok, count, first, last](
                              auto&&... data) mutable -> Iter {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in the function objects
                    // that are attached to futures are invalidated
                    util::detail::clear_container(data...);

                    auto find_res =
                        static_cast<difference_type>(tok.get_data());

                    if (find_res != count)
                    {
                        std::advance(first, find_res);
                    }
                    else
                    {
                        first = detail::advance_to_sentinel(first, last);
                    }
                    return first;
                };

                using partitioner_type =
                    util::partitioner<policy_type, Iter, void>;

                return partitioner_type::call_with_index(
                    HPX_FORWARD(decltype(policy), policy), first, count, 1,
                    HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // find_if
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename FwdIter>
        struct find_if : public algorithm<find_if<FwdIter>, FwdIter>
        {
            constexpr find_if() noexcept
              : algorithm<find_if, FwdIter>("find_if")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename F, typename Proj = hpx::identity>
            static constexpr Iter sequential(
                ExPolicy, Iter first, Sent last, F&& f, Proj&& proj = Proj())
            {
                return sequential_find_if<ExPolicy>(
                    first, last, HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename F, typename Proj = hpx::identity>
            static decltype(auto) parallel(ExPolicy&& orgpolicy, Iter first,
                Sent last, F&& f, Proj&& proj = Proj())
            {
                using result = util::detail::algorithm_result<ExPolicy, Iter>;
                using difference_type =
                    typename std::iterator_traits<Iter>::difference_type;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return result::get(HPX_MOVE(first));
                    }
                }

                difference_type count = detail::distance(first, last);

                decltype(auto) policy =
                    hpx::execution::experimental::adapt_placement_mode(
                        HPX_FORWARD(ExPolicy, orgpolicy),
                        hpx::threads::thread_placement_hint::breadth_first);

                using policy_type = std::decay_t<decltype(policy)>;

                util::cancellation_token<std::size_t> tok(count);

                auto f1 = [f = HPX_FORWARD(F, f),
                              proj = HPX_FORWARD(Proj, proj),
                              tok](Iter it, std::size_t part_size,
                              std::size_t base_idx) mutable -> void {
                    sequential_find_if<policy_type>(base_idx, it, part_size,
                        tok, HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));
                };

                auto f2 = [tok, count, first, last](
                              auto&&... data) mutable -> Iter {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in the function objects that
                    // are attached to futures are invalidated
                    util::detail::clear_container(data...);

                    auto find_res =
                        static_cast<difference_type>(tok.get_data());

                    if (find_res != count)
                    {
                        std::advance(first, find_res);
                    }
                    else
                    {
                        first = detail::advance_to_sentinel(first, last);
                    }
                    return first;
                };

                using partitioner_type =
                    util::partitioner<policy_type, Iter, void>;
                return partitioner_type::call_with_index(
                    HPX_FORWARD(decltype(policy), policy), first, count, 1,
                    HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // find_if_not
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename FwdIter>
        struct find_if_not : public algorithm<find_if_not<FwdIter>, FwdIter>
        {
            constexpr find_if_not() noexcept
              : algorithm<find_if_not, FwdIter>("find_if_not")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename F, typename Proj = hpx::identity>
            static constexpr Iter sequential(
                ExPolicy, Iter first, Sent last, F&& f, Proj&& proj = Proj())
            {
                return sequential_find_if_not<ExPolicy>(
                    first, last, HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename F, typename Proj = hpx::identity>
            static decltype(auto) parallel(ExPolicy&& orgpolicy, Iter first,
                Sent last, F&& f, Proj&& proj = Proj())
            {
                using result = util::detail::algorithm_result<ExPolicy, Iter>;
                using difference_type =
                    typename std::iterator_traits<Iter>::difference_type;
                constexpr bool has_scheduler_policy =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_policy)
                {
                    if (first == last)
                    {
                        return result::get(HPX_MOVE(first));
                    }
                }

                difference_type count = detail::distance(first, last);

                decltype(auto) policy =
                    hpx::execution::experimental::adapt_placement_mode(
                        HPX_FORWARD(ExPolicy, orgpolicy),
                        hpx::threads::thread_placement_hint::breadth_first);

                using policy_type = std::decay_t<decltype(policy)>;

                util::cancellation_token<std::size_t> tok(count);

                auto f1 = [f = HPX_FORWARD(F, f),
                              proj = HPX_FORWARD(Proj, proj),
                              tok](Iter it, std::size_t part_size,
                              std::size_t base_idx) mutable -> void {
                    sequential_find_if_not<policy_type>(base_idx, it, part_size,
                        tok, HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));
                };

                auto f2 = [tok, count, first, last](
                              auto&&... data) mutable -> Iter {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in the function objects that
                    // are attached to futures are invalidated
                    util::detail::clear_container(data...);

                    auto find_res =
                        static_cast<difference_type>(tok.get_data());

                    if (find_res != count)
                    {
                        std::advance(first, find_res);
                    }
                    else
                    {
                        first = detail::advance_to_sentinel(first, last);
                    }
                    return first;
                };

                using partitioner_type =
                    util::partitioner<policy_type, Iter, void>;
                return partitioner_type::call_with_index(
                    HPX_FORWARD(decltype(policy), policy), first, count, 1,
                    HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // find_end
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename FwdIter>
        struct find_end : public algorithm<find_end<FwdIter>, FwdIter>
        {
            constexpr find_end() noexcept
              : algorithm<find_end, FwdIter>("find_end")
            {
            }

            template <typename ExPolicy, typename Iter1, typename Sent1,
                typename Iter2, typename Sent2, typename Pred, typename Proj1,
                typename Proj2>
            static constexpr Iter1 sequential(ExPolicy, Iter1 first1,
                Sent1 last1, Iter2 first2, Sent2 last2, Pred&& op,
                Proj1&& proj1, Proj2&& proj2)
            {
                return sequential_find_end<std::decay_t<ExPolicy>>(first1,
                    last1, first2, last2, HPX_FORWARD(Pred, op),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
            }

            template <typename ExPolicy, typename Iter1, typename Sent1,
                typename Iter2, typename Sent2, typename Pred, typename Proj1,
                typename Proj2>
            static decltype(auto) parallel(ExPolicy&& orgpolicy, Iter1 first1,
                Sent1 last1, Iter2 first2, Sent2 last2, Pred&& op,
                Proj1&& proj1, Proj2&& proj2)
            {
                using result_type =
                    util::detail::algorithm_result<ExPolicy, Iter1>;
                using difference_type =
                    typename std::iterator_traits<Iter1>::difference_type;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first2 == last2)
                    {
                        return result_type::get(
                            detail::advance_to_sentinel(first1, last1));
                    }
                }

                difference_type count = detail::distance(first1, last1);
                difference_type diff = detail::distance(first2, last2);

                if constexpr (!has_scheduler_executor)
                {
                    if (diff > count)
                    {
                        return result_type::get(
                            detail::advance_to_sentinel(first1, last1));
                    }
                }

                difference_type partitioner_count = count - diff + 1;
                if constexpr (has_scheduler_executor)
                {
                    if (diff == 0 || diff > count)
                        partitioner_count = static_cast<difference_type>(0);
                }

                decltype(auto) policy =
                    hpx::execution::experimental::adapt_placement_mode(
                        HPX_FORWARD(ExPolicy, orgpolicy),
                        hpx::threads::thread_placement_hint::
                            breadth_first_reverse);

                using policy_type = std::decay_t<decltype(policy)>;

                util::cancellation_token<difference_type,
                    std::greater<difference_type>>
                    tok(-1);

                auto f1 = [diff, tok, first2, op = HPX_FORWARD(Pred, op),
                              proj1 = HPX_FORWARD(Proj1, proj1),
                              proj2 = HPX_FORWARD(Proj2, proj2)](Iter1 it,
                              std::size_t part_size,
                              std::size_t base_idx) mutable -> void {
                    sequential_find_end<policy_type>(it, first2, base_idx,
                        part_size, diff, tok, HPX_FORWARD(Pred, op),
                        HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
                };

                auto f2 = [tok, count, first1, last1](
                              auto&&... data) mutable -> Iter1 {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in the function objects that
                    // are attached to futures are invalidated
                    util::detail::clear_container(data...);

                    difference_type find_end_res = tok.get_data();

                    if (find_end_res >= 0 && find_end_res != count)
                    {
                        std::advance(first1, find_end_res);
                    }
                    else
                    {
                        first1 = last1;
                    }
                    return first1;
                };

                using partitioner_type =
                    util::partitioner<policy_type, Iter1, void>;
                return partitioner_type::call_with_index(
                    HPX_FORWARD(decltype(policy), policy), first1,
                    partitioner_count, 1, HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // find_first_of
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename FwdIter>
        struct find_first_of : public algorithm<find_first_of<FwdIter>, FwdIter>
        {
            constexpr find_first_of() noexcept
              : algorithm<find_first_of, FwdIter>("find_first_of")
            {
            }

            template <typename ExPolicy, typename InIter1, typename InIter2,
                typename Pred, typename Proj1, typename Proj2>
            static InIter1 sequential(ExPolicy, InIter1 first, InIter1 last,
                InIter2 s_first, InIter2 s_last, Pred&& op, Proj1&& proj1,
                Proj2&& proj2)
            {
                return sequential_find_first_of<std::decay_t<ExPolicy>>(first,
                    last, s_first, s_last, HPX_FORWARD(Pred, op),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
            }

            template <typename ExPolicy, typename FwdIter2, typename Pred,
                typename Proj1, typename Proj2>
            static decltype(auto) parallel(ExPolicy&& orgpolicy, FwdIter first,
                FwdIter last, FwdIter2 s_first, FwdIter2 s_last, Pred&& op,
                Proj1&& proj1, Proj2&& proj2)
            {
                using result =
                    util::detail::algorithm_result<ExPolicy, FwdIter>;
                using difference_type =
                    typename std::iterator_traits<FwdIter>::difference_type;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                        return result::get(HPX_MOVE(last));
                }

                difference_type count = std::distance(first, last);

                if (s_first == s_last)
                {
                    if constexpr (has_scheduler_executor)
                    {
                        count = static_cast<difference_type>(0);
                    }
                    else
                    {
                        return result::get(HPX_MOVE(last));
                    }
                }

                decltype(auto) policy =
                    hpx::execution::experimental::adapt_placement_mode(
                        HPX_FORWARD(ExPolicy, orgpolicy),
                        hpx::threads::thread_placement_hint::breadth_first);

                using policy_type = std::decay_t<decltype(policy)>;

                util::cancellation_token<difference_type> tok(count);

                auto f1 = [s_first, s_last, tok, op = HPX_FORWARD(Pred, op),
                              proj1 = HPX_FORWARD(Proj1, proj1),
                              proj2 = HPX_FORWARD(Proj2, proj2)](FwdIter it,
                              std::size_t part_size,
                              std::size_t base_idx) mutable -> void {
                    sequential_find_first_of<policy_type>(it, s_first, s_last,
                        base_idx, part_size, tok, HPX_FORWARD(Pred, op),
                        HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
                };

                auto f2 = [tok, count, first, last](
                              auto&&... data) mutable -> FwdIter {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in the function objects that
                    // are attached to futures are invalidated
                    util::detail::clear_container(data...);

                    difference_type find_first_of_res = tok.get_data();

                    if (find_first_of_res != count)
                    {
                        std::advance(first, find_first_of_res);
                    }
                    else
                    {
                        first = last;
                    }

                    return first;
                };

                using partitioner_type =
                    util::partitioner<policy_type, FwdIter, void>;
                return partitioner_type::call_with_index(
                    HPX_FORWARD(decltype(policy), policy), first, count, 1,
                    HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::find
    HPX_CXX_CORE_EXPORT inline constexpr struct find_t final
      : hpx::detail::tag_parallel_algorithm<find_t>
    {
    private:
        template <typename InIter,
            typename T = typename std::iterator_traits<InIter>::value_type>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter>
            )
        // clang-format on
        friend constexpr InIter tag_fallback_invoke(
            find_t, InIter first, InIter last, T const& val)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::find<InIter>().call(
                hpx::execution::seq, first, last, val, hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(find_t, ExPolicy&& policy,
            FwdIter first, FwdIter last, T const& val)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, val,
                hpx::identity_v);
        }
    } find{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::find_if
    HPX_CXX_CORE_EXPORT inline constexpr struct find_if_t final
      : hpx::detail::tag_parallel_algorithm<find_if_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter, typename F>
        // clang-format off
        requires (
            hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter> &&
            hpx::is_invocable_v<F, hpx::traits::iter_value_t<FwdIter>>
        )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(
            find_if_t, ExPolicy&& policy, FwdIter first, FwdIter last, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find_if<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                hpx::identity_v);
        }

        template <typename InIter, typename F>
        // clang-format off
        requires (
            hpx::traits::is_iterator_v<InIter> &&
            hpx::is_invocable_v<F, hpx::traits::iter_value_t<InIter>>
        )
        // clang-format on
        friend InIter tag_fallback_invoke(
            find_if_t, InIter first, InIter last, F f)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::find_if<InIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), hpx::identity_v);
        }
    } find_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::find_if_not
    HPX_CXX_CORE_EXPORT inline constexpr struct find_if_not_t final
      : hpx::detail::tag_parallel_algorithm<find_if_not_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter, typename F>
        // clang-format off
        requires (
            hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter> &&
            hpx::is_invocable_v<F, hpx::traits::iter_value_t<FwdIter>>
        )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(
            find_if_not_t, ExPolicy&& policy, FwdIter first, FwdIter last, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find_if_not<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                hpx::identity_v);
        }

        template <typename FwdIter, typename F>
        // clang-format off
        requires (
            hpx::traits::is_iterator_v<FwdIter> &&
            hpx::is_invocable_v<F, hpx::traits::iter_value_t<FwdIter>>
        )
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            find_if_not_t, FwdIter first, FwdIter last, F f)
        {
            static_assert(std::input_iterator<FwdIter>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::find_if_not<FwdIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), hpx::identity_v);
        }
    } find_if_not{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::find_end
    HPX_CXX_CORE_EXPORT inline constexpr struct find_end_t final
      : hpx::detail::tag_parallel_algorithm<find_end_t>
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
                hpx::traits::iter_value_t<FwdIter1>, hpx::traits::iter_value_t<FwdIter2>
            >
        )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(find_end_t, ExPolicy&& policy,
            FwdIter1 first1, FwdIter1 last1, FwdIter2 first2, FwdIter2 last2,
            Pred op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find_end<FwdIter1>().call(
                HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2,
                HPX_MOVE(op), hpx::identity_v, hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
        // clang-format off
        requires (
            hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2>
        )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(find_end_t, ExPolicy&& policy,
            FwdIter1 first1, FwdIter1 last1, FwdIter2 first2, FwdIter2 last2)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find_end<FwdIter1>().call(
                HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2,
                hpx::parallel::detail::equal_to{}, hpx::identity_v,
                hpx::identity_v);
        }

        template <typename FwdIter1, typename FwdIter2, typename Pred>
        // clang-format off
        requires (
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2> &&
            hpx::is_invocable_v<Pred,
                hpx::traits::iter_value_t<FwdIter1>, hpx::traits::iter_value_t<FwdIter2>
            >
        )
        // clang-format on
        friend FwdIter1 tag_fallback_invoke(find_end_t, FwdIter1 first1,
            FwdIter1 last1, FwdIter2 first2, FwdIter2 last2, Pred op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find_end<FwdIter1>().call(
                hpx::execution::seq, first1, last1, first2, last2, HPX_MOVE(op),
                hpx::identity_v, hpx::identity_v);
        }

        template <typename FwdIter1, typename FwdIter2>
        // clang-format off
        requires (
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2>
        )
        // clang-format on
        friend FwdIter1 tag_fallback_invoke(find_end_t, FwdIter1 first1,
            FwdIter1 last1, FwdIter2 first2, FwdIter2 last2)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find_end<FwdIter1>().call(
                hpx::execution::seq, first1, last1, first2, last2,
                hpx::parallel::detail::equal_to{}, hpx::identity_v,
                hpx::identity_v);
        }
    } find_end{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::find_first_of
    HPX_CXX_CORE_EXPORT inline constexpr struct find_first_of_t final
      : hpx::detail::tag_parallel_algorithm<find_first_of_t>
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
                hpx::traits::iter_value_t<FwdIter1>, hpx::traits::iter_value_t<FwdIter2>
            >
        )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(find_first_of_t,
            ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 s_first,
            FwdIter2 s_last, Pred op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Subsequence requires at least forward iterator.");

            return hpx::parallel::detail::find_first_of<FwdIter1>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, s_first, s_last,
                HPX_MOVE(op), hpx::identity_v, hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
        // clang-format off
        requires (
            hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2>
        )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(find_first_of_t,
            ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 s_first,
            FwdIter2 s_last)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Subsequence requires at least forward iterator.");

            return hpx::parallel::detail::find_first_of<FwdIter1>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, s_first, s_last,
                hpx::parallel::detail::equal_to{}, hpx::identity_v,
                hpx::identity_v);
        }

        template <typename FwdIter1, typename FwdIter2, typename Pred>
        // clang-format off
        requires (
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2> &&
            hpx::is_invocable_v<Pred,
                hpx::traits::iter_value_t<FwdIter1>, hpx::traits::iter_value_t<FwdIter2>
            >
        )
        // clang-format on
        friend FwdIter1 tag_fallback_invoke(find_first_of_t, FwdIter1 first,
            FwdIter1 last, FwdIter2 s_first, FwdIter2 s_last, Pred op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Subsequence requires at least forward iterator.");

            return hpx::parallel::detail::find_first_of<FwdIter1>().call(
                hpx::execution::seq, first, last, s_first, s_last, HPX_MOVE(op),
                hpx::identity_v, hpx::identity_v);
        }

        template <typename FwdIter1, typename FwdIter2>
        // clang-format off
        requires (
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2>
        )
        // clang-format on
        friend FwdIter1 tag_fallback_invoke(find_first_of_t, FwdIter1 first,
            FwdIter1 last, FwdIter2 s_first, FwdIter2 s_last)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Subsequence requires at least forward iterator.");

            return hpx::parallel::detail::find_first_of<FwdIter1>().call(
                hpx::execution::seq, first, last, s_first, s_last,
                hpx::parallel::detail::equal_to{}, hpx::identity_v,
                hpx::identity_v);
        }
    } find_first_of{};
}    // namespace hpx

namespace hpx::parallel::detail {
    ///////////////////////////////////////////////////////////////////////////
    // find_last
    HPX_CXX_CORE_EXPORT template <typename FwdIter>
    struct find_last : public algorithm<find_last<FwdIter>, FwdIter>
    {
        constexpr find_last() noexcept
          : algorithm<find_last, FwdIter>("find_last")
        {
        }

        template <typename ExPolicy, typename Iter, typename Sent, typename T,
            typename Proj = hpx::identity>
        static constexpr Iter sequential(
            ExPolicy, Iter first, Sent last, T const& val, Proj&& proj = Proj())
        {
            return sequential_find_last<ExPolicy>(
                first, last, val, HPX_FORWARD(Proj, proj));
        }

        template <typename ExPolicy, typename Iter, typename Sent, typename T,
            typename Proj = hpx::identity>
        static decltype(auto) parallel(ExPolicy&& orgpolicy, Iter first,
            Sent last, T const& val, Proj&& proj = Proj())
        {
            using result = util::detail::algorithm_result<ExPolicy, Iter>;
            using difference_type =
                typename std::iterator_traits<Iter>::difference_type;
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            if constexpr (!has_scheduler_executor)
            {
                if (first == last)
                {
                    return result::get(HPX_MOVE(first));
                }
            }

            difference_type count = detail::distance(first, last);

            decltype(auto) policy =
                hpx::execution::experimental::adapt_placement_mode(
                    HPX_FORWARD(ExPolicy, orgpolicy),
                    hpx::threads::thread_placement_hint::breadth_first_reverse);

            using policy_type = std::decay_t<decltype(policy)>;

            util::cancellation_token<difference_type,
                std::greater<difference_type>>
                tok(-1);

            auto f1 = [val, proj = HPX_FORWARD(Proj, proj), tok](Iter it,
                          std::size_t part_size,
                          std::size_t base_idx) mutable -> void {
                sequential_find_last<policy_type>(
                    base_idx, it, part_size, tok, val, HPX_FORWARD(Proj, proj));
            };

            auto f2 = [tok, first, last](auto&&... data) mutable -> Iter {
                static_assert(sizeof...(data) < 2);
                if constexpr (sizeof...(data) == 1)
                {
                    util::detail::clear_container(data...);
                }

                difference_type find_res = tok.get_data();

                if (find_res != -1)
                {
                    std::advance(first, find_res);
                }
                else
                {
                    first = detail::advance_to_sentinel(first, last);
                }

                return first;
            };

            using partitioner_type = util::partitioner<policy_type, Iter, void>;

            return partitioner_type::call_with_index(
                HPX_FORWARD(decltype(policy), policy), first, count, 1,
                HPX_MOVE(f1), HPX_MOVE(f2));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // find_last_if
    HPX_CXX_CORE_EXPORT template <typename FwdIter>
    struct find_last_if : public algorithm<find_last_if<FwdIter>, FwdIter>
    {
        constexpr find_last_if() noexcept
          : algorithm<find_last_if, FwdIter>("find_last_if")
        {
        }

        template <typename ExPolicy, typename Iter, typename Sent, typename F,
            typename Proj = hpx::identity>
        static constexpr Iter sequential(
            ExPolicy, Iter first, Sent last, F&& f, Proj&& proj = Proj())
        {
            return sequential_find_last_if<ExPolicy>(
                first, last, HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));
        }

        template <typename ExPolicy, typename Iter, typename Sent, typename F,
            typename Proj = hpx::identity>
        static decltype(auto) parallel(ExPolicy&& orgpolicy, Iter first,
            Sent last, F&& f, Proj&& proj = Proj())
        {
            using result = util::detail::algorithm_result<ExPolicy, Iter>;
            using difference_type =
                typename std::iterator_traits<Iter>::difference_type;
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            if constexpr (!has_scheduler_executor)
            {
                if (first == last)
                {
                    return result::get(HPX_MOVE(first));
                }
            }

            difference_type count = detail::distance(first, last);

            decltype(auto) policy =
                hpx::execution::experimental::adapt_placement_mode(
                    HPX_FORWARD(ExPolicy, orgpolicy),
                    hpx::threads::thread_placement_hint::breadth_first_reverse);

            using policy_type = std::decay_t<decltype(policy)>;

            util::cancellation_token<difference_type,
                std::greater<difference_type>>
                tok(-1);

            auto f1 = [f = HPX_FORWARD(F, f), proj = HPX_FORWARD(Proj, proj),
                          tok](Iter it, std::size_t part_size,
                          std::size_t base_idx) mutable -> void {
                sequential_find_last_if<policy_type>(base_idx, it, part_size,
                    tok, HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));
            };

            auto f2 = [tok, first, last](auto&&... data) mutable -> Iter {
                static_assert(sizeof...(data) < 2);
                if constexpr (sizeof...(data) == 1)
                {
                    util::detail::clear_container(data...);
                }

                difference_type find_res = tok.get_data();

                if (find_res != -1)
                {
                    std::advance(first, find_res);
                }
                else
                {
                    first = detail::advance_to_sentinel(first, last);
                }

                return first;
            };

            using partitioner_type = util::partitioner<policy_type, Iter, void>;

            return partitioner_type::call_with_index(
                HPX_FORWARD(decltype(policy), policy), first, count, 1,
                HPX_MOVE(f1), HPX_MOVE(f2));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // find_last_if_not
    HPX_CXX_CORE_EXPORT template <typename FwdIter>
    struct find_last_if_not
      : public algorithm<find_last_if_not<FwdIter>, FwdIter>
    {
        constexpr find_last_if_not() noexcept
          : algorithm<find_last_if_not, FwdIter>("find_last_if_not")
        {
        }

        template <typename ExPolicy, typename Iter, typename Sent, typename F,
            typename Proj = hpx::identity>
        static constexpr Iter sequential(
            ExPolicy, Iter first, Sent last, F&& f, Proj&& proj = Proj())
        {
            return sequential_find_last_if_not<ExPolicy>(
                first, last, HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));
        }

        template <typename ExPolicy, typename Iter, typename Sent, typename F,
            typename Proj = hpx::identity>
        static decltype(auto) parallel(ExPolicy&& orgpolicy, Iter first,
            Sent last, F&& f, Proj&& proj = Proj())
        {
            using result = util::detail::algorithm_result<ExPolicy, Iter>;
            using difference_type =
                typename std::iterator_traits<Iter>::difference_type;
            constexpr bool has_scheduler_policy =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            if constexpr (!has_scheduler_policy)
            {
                if (first == last)
                {
                    return result::get(HPX_MOVE(first));
                }
            }

            difference_type count = detail::distance(first, last);

            decltype(auto) policy =
                hpx::execution::experimental::adapt_placement_mode(
                    HPX_FORWARD(ExPolicy, orgpolicy),
                    hpx::threads::thread_placement_hint::breadth_first_reverse);

            using policy_type = std::decay_t<decltype(policy)>;

            util::cancellation_token<difference_type,
                std::greater<difference_type>>
                tok(-1);

            auto f1 = [f = HPX_FORWARD(F, f), proj = HPX_FORWARD(Proj, proj),
                          tok](Iter it, std::size_t part_size,
                          std::size_t base_idx) mutable -> void {
                sequential_find_last_if_not<policy_type>(base_idx, it,
                    part_size, tok, HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));
            };

            auto f2 = [tok, first, last](auto&&... data) mutable -> Iter {
                static_assert(sizeof...(data) < 2);
                if constexpr (sizeof...(data) == 1)
                {
                    util::detail::clear_container(data...);
                }

                difference_type find_res = tok.get_data();

                if (find_res != -1)
                {
                    std::advance(first, find_res);
                }
                else
                {
                    first = detail::advance_to_sentinel(first, last);
                }

                return first;
            };

            using partitioner_type = util::partitioner<policy_type, Iter, void>;

            return partitioner_type::call_with_index(
                HPX_FORWARD(decltype(policy), policy), first, count, 1,
                HPX_MOVE(f1), HPX_MOVE(f2));
        }
    };
}    // namespace hpx::parallel::detail

namespace hpx {
    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::find_last
    HPX_CXX_CORE_EXPORT inline constexpr struct find_last_t final
      : hpx::detail::tag_parallel_algorithm<find_last_t>
    {
    private:
        template <typename InIter,
            typename T = typename std::iterator_traits<InIter>::value_type>
        // clang-format off
        requires (
            hpx::traits::is_iterator_v<InIter>
        )
        // clang-format on
        friend constexpr InIter tag_fallback_invoke(
            find_last_t, InIter first, InIter last, T const& val)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::find_last<InIter>().call(
                hpx::execution::seq, first, last, val, hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
        requires (
            hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter>
        )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(find_last_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, T const& val)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find_last<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, val,
                hpx::identity_v);
        }
    } find_last{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::find_last_if
    HPX_CXX_CORE_EXPORT inline constexpr struct find_last_if_t final
      : hpx::detail::tag_parallel_algorithm<find_last_if_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter, typename F>
        // clang-format off
        requires (
            hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter> &&
            hpx::is_invocable_v<F, hpx::traits::iter_value_t<FwdIter>>
        )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(
            find_last_if_t, ExPolicy&& policy, FwdIter first, FwdIter last, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find_last_if<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                hpx::identity_v);
        }

        template <typename InIter, typename F>
        // clang-format off
        requires (
            hpx::traits::is_iterator_v<InIter> &&
            hpx::is_invocable_v<F, hpx::traits::iter_value_t<InIter>>
        )
        // clang-format on
        friend InIter tag_fallback_invoke(
            find_last_if_t, InIter first, InIter last, F f)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::find_last_if<InIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), hpx::identity_v);
        }
    } find_last_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::find_last_if_not
    HPX_CXX_CORE_EXPORT inline constexpr struct find_last_if_not_t final
      : hpx::detail::tag_parallel_algorithm<find_last_if_not_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter, typename F>
        // clang-format off
        requires (
            hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter> &&
            hpx::is_invocable_v<F, hpx::traits::iter_value_t<FwdIter>>
        )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(find_last_if_not_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find_last_if_not<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                hpx::identity_v);
        }

        template <typename FwdIter, typename F>
        // clang-format off
        requires (
            hpx::traits::is_iterator_v<FwdIter> &&
            hpx::is_invocable_v<F, hpx::traits::iter_value_t<FwdIter>>
        )
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            find_last_if_not_t, FwdIter first, FwdIter last, F f)
        {
            static_assert(std::input_iterator<FwdIter>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::find_last_if_not<FwdIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), hpx::identity_v);
        }
    } find_last_if_not{};
}    // namespace hpx

#endif    // DOXYGEN
