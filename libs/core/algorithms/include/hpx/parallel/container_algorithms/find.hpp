//  Copyright (c) 2018 Bruno Pitrus
//  Copyright (c) 2020-2023 Hartmut Kaiser
//  Copyright (c) 2022 Dimitra Karatza
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/find.hpp
/// \page hpx::ranges::find, hpx::ranges::find_if, hpx::ranges::find_if_not, hpx::ranges::find_end, hpx::ranges::find_first_of, hpx::ranges::find_last, hpx::ranges::find_last_if, hpx::ranges::find_last_if_not
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
    // clang-format off

    /// Returns the first element in the range [first, last) that is equal
    /// to value
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the operator==().
    ///
    /// \tparam Iter        The type of the begin source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter.
    /// \tparam T           The type of the value to find (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param val          the value to compare the elements to
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a find algorithm returns the first element in the range
    ///           [first,last) that is equal to \a val.
    ///           If no such element in the range of [first,last) is equal to
    ///           \a val, then the algorithm returns \a last.
    ///
    template <typename Iter, typename Sent,
        typename Proj = hpx::identity,
        typename T = typename hpx::parallel::traits::projected<Iter,
            Proj>::value_type>
    Iter find(Iter first, Sent last, T const& val, Proj&& proj = Proj());

    /// Returns the first element in the range [first, last) that is equal
    /// to value
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the operator==().
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam T           The type of the value to find (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param val          the value to compare the elements to
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
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
    template <typename Rng,
        typename Proj = hpx::identity,
        typename T = typename hpx::parallel::traits::projected<
            std::ranges::iterator_t<Rng>, Proj>::value_type>
    std::ranges::iterator_t<Rng>
    find(Rng&& rng, T const& val, Proj&& proj = Proj());

    /// Returns the first element in the range [first, last) for which
    /// predicate \a pred returns true
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam Iter        The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param pred         The unary predicate which returns true for the
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
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a find_if algorithm returns the first element in the range
    ///           [first,last) that satisfies the predicate \a f.
    ///           If no such element exists that satisfies the predicate f, the
    ///           algorithm returns \a last.
    ///
    template <typename Iter, typename Sent, typename Pred,
        typename Proj = hpx::identity>
    Iter find_if(Iter first, Sent last, Pred&& pred, Proj&& proj = Proj());

    /// Returns the first element in the range \a rng for which
    /// predicate \a pred returns true
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param pred         The unary predicate which returns true for the
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
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a find_if algorithm returns the first element in the range
    ///           [first,last) that satisfies the predicate \a f.
    ///           If no such element exists that satisfies the predicate f, the
    ///           algorithm returns \a last.
    ///
    template <typename Rng, typename Pred,
        typename Proj = hpx::identity>
    std::ranges::iterator_t<Rng>
    find_if(Rng&& rng, Pred&& pred, Proj&& proj = Proj());

    /// Returns the first element in the range [first, last) for which
    /// predicate \a f returns false
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam Iter        The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param pred         The unary predicate which returns false for the
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
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a find_if_not algorithm returns the first element in the range
    ///           [first, last) that does \b not satisfy the predicate \a f.
    ///           If no such element exists that does not satisfy the predicate f, the
    ///           algorithm returns \a last.
    ///
    template <typename Iter, typename Sent, typename Pred,
        typename Proj = hpx::identity>
    Iter find_if_not(Iter first, Sent last, Pred&& pred, Proj&& proj = Proj());

    /// Returns the first element in the range \a rng for which
    /// predicate \a f returns false
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a equal requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param pred         The unary predicate which returns false for the
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
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a find_if_not algorithm returns the first element in the range
    ///           [first, last) that does \b not satisfy the predicate \a f.
    ///           If no such element exists that does not satisfy the predicate f, the
    ///           algorithm returns \a last.
    ///
    template <typename Rng, typename Pred,
        typename Proj = hpx::identity>
    std::ranges::iterator_t<Rng>
    find_if_not(Rng&& rng, Pred&& pred, Proj&& proj = Proj());

    /// Returns the last subsequence of elements \a rng2 found in the range
    /// \a rng using the given predicate \a f to compare elements.
    ///
    /// \note   Complexity: at most S*(N-S+1) comparisons where
    ///         \a S = distance(begin(rng2), end(rng2)) and
    ///         \a N = distance(begin(rng), end(rng)).
    ///
    /// \tparam Rng1        The type of the first source range (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam Rng2        The type of the second source range (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a replace requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function applied
    ///                     to the first sequence. This
    ///                     defaults to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function applied
    ///                     to the second sequence. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng1         Refers to the first sequence of elements
    ///                     the algorithm will be applied to.
    /// \param rng2         Refers to the second sequence of elements
    ///                     the algorithm will be applied to.
    /// \param op           The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The signature
    ///                     should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a iterator_t<Rng> and \a iterator_t<Rng2>
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 and \a Type2 respectively.
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the first
    ///                     range of type dereferenced \a iterator_t<Rng1>
    ///                     as a projection operation before the function \a op
    ///                     is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the second
    ///                     range of type dereferenced \a iterator_t<Rng2>
    ///                     as a projection operation before the function \a op
    ///                     is invoked.
    ///
    /// \returns  The \a find_end algorithm returns an iterator to the beginning of
    ///           the last subsequence \a rng2 in range \a rng.
    ///           If the length of the subsequence \a rng2 is greater
    ///           than the length of the range \a rng, \a end(rng) is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a end(rng) is also returned.
    ///
    /// This overload of \a find_end is available if the user decides to provide the
    /// algorithm their own predicate \a op.
    ///
    template <typename Rng1, typename Rng2, typename Pred = equal_to,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    std::ranges::iterator_t<Rng1>
    find_end(Rng1&& rng1, Rng2&& rng2, Pred&& op = Pred(),
        Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    /// Returns the last subsequence of elements \a [first2, last2) found in
    /// the range \a [first1, last1) using the given predicate \a f to
    /// compare elements.
    ///
    /// \note   Complexity: at most S*(N-S+1) comparisons where
    ///         \a S = distance(first2, last2) and
    ///         \a N = distance(first1, last1).
    ///
    /// \tparam Iter1       The type of the begin source iterators for the first
    ///                     sequence used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent1       The type of the end source iterators for the first
    ///                     sequence used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter1.
    /// \tparam Iter2       The type of the begin source iterators for the second
    ///                     sequence used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent2       The type of the end source iterators for the second
    ///                     sequence used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter2.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a replace requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function applied
    ///                     to the first sequence. This
    ///                     defaults to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function applied
    ///                     to the second sequence. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first1       Refers to the beginning of the first sequence of
    ///                     elements the algorithm will be applied to.
    /// \param last1        Refers to the end of the first sequence of elements
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the algorithm will be applied to.
    /// \param last2        Refers to the end of the second sequence of elements
    ///                     the algorithm will be applied to.
    /// \param op           The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The signature
    ///                     should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a iterator_t<Rng> and \a iterator_t<Rng2>
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 and \a Type2 respectively.
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the first
    ///                     range of type dereferenced \a iterator_t<Rng1>
    ///                     as a projection operation before the function \a op
    ///                     is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the second
    ///                     range of type dereferenced \a iterator_t<Rng2>
    ///                     as a projection operation before the function \a op
    ///                     is invoked.
    ///
    /// \returns  The \a find_end algorithm returns an iterator to the beginning of
    ///           the last subsequence \a rng2 in range \a rng.
    ///           If the length of the subsequence \a rng2 is greater
    ///           than the length of the range \a rng, \a end(rng) is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a end(rng) is also returned.
    ///
    /// This overload of \a find_end is available if the user decides to provide the
    /// algorithm their own predicate \a op.
    ///
    template <typename Iter1, typename Sent1,
        typename Iter2, typename Sent2, typename Pred = equal_to,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    Iter1 find_end(Iter1 first1, Sent1 last1, Iter2 first2,
        Sent2 last2, Pred&& op = Pred(), Proj1&& proj1 = Proj1(),
        Proj2&& proj2 = Proj2());

    /// Searches the range \a rng1 for any elements in the range \a rng2.
    /// Uses binary predicate \a p to compare elements
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(begin(rng2), end(rng2)) and
    ///         \a N = distance(begin(rng1), end(rng1)).
    ///
    /// \tparam Rng1        The type of the first source range (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam Rng2        The type of the second source range (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a replace requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is
    ///                     applied to the elements in \a rng1.
    /// \tparam Proj2       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is
    ///                     applied to the elements in \a rng2.
    ///
    /// \param rng1         Refers to the first sequence of elements
    ///                     the algorithm will be applied to.
    /// \param rng2         Refers to the second sequence of elements
    ///                     the algorithm will be applied to.
    /// \param op           The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The signature
    ///                     should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a iterator_t<Rng1>
    ///                     and \a iterator_t<Rng2>
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 and \a Type2 respectively.
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a iterator_t<Rng1> before the function
    ///                     \a op is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a iterator_t<Rng2> before the function
    ///                     \a op is invoked.
    ///
    /// \returns  The \a find_first_of algorithm returns an iterator to the first element
    ///           in the range \a rng1 that is equal to an element from the range
    ///           \a rng2.
    ///           If the length of the subsequence \a rng2 is
    ///           greater than the length of the range \a rng1,
    ///           \a end(rng1) is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a end(rng1) is also returned.
    ///
    /// This overload of \a find_first_of is available if the user decides to provide the
    /// algorithm their own predicate \a op.
    ///
    template <typename Rng1, typename Rng2, typename Pred = equal_to,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    std::ranges::iterator_t<Rng1>
    find_first_of(Rng1&& rng1, Rng2&& rng2,
        Pred&& op = Pred(), Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    /// Searches the range \a [first1, last1) for any elements in the
    /// range \a [first2, last2).
    /// Uses binary predicate \a p to compare elements
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(first2, last2) and
    ///         \a N = distance(first1, last1).
    ///
    /// \tparam Iter1       The type of the begin source iterators for the first
    ///                     sequence used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent1       The type of the end source iterators for the first
    ///                     sequence used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter1.
    /// \tparam Iter2       The type of the begin source iterators for the second
    ///                     sequence used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent2       The type of the end source iterators for the second
    ///                     sequence used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter2.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a replace requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is
    ///                     applied to the elements in \a rng1.
    /// \tparam Proj2       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is
    ///                     applied to the elements in \a rng2.
    ///
    /// \param first1       Refers to the beginning of the first sequence of
    ///                     elements the algorithm will be applied to.
    /// \param last1        Refers to the end of the first sequence of elements
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the algorithm will be applied to.
    /// \param last2        Refers to the end of the second sequence of elements
    ///                     the algorithm will be applied to.
    /// \param op           The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The signature
    ///                     should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a iterator_t<Rng1>
    ///                     and \a iterator_t<Rng2>
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 and \a Type2 respectively.
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a iterator_t<Rng1> before the function
    ///                     \a op is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a iterator_t<Rng2> before the function
    ///                     \a op is invoked.
    ///
    /// \returns  The \a find_first_of algorithm returns an iterator to the first element
    ///           in the range \a rng1 that is equal to an element from the range
    ///           \a rng2.
    ///           If the length of the subsequence \a rng2 is
    ///           greater than the length of the range \a rng1,
    ///           \a end(rng1) is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a end(rng1) is also returned.
    ///
    /// This overload of \a find_first_of is available if the user decides to provide the
    /// algorithm their own predicate \a op.
    ///
    template <typename Iter1, typename Sent1, typename Iter2, typename Sent2,
        typename Pred = equal_to,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    Iter1 find_first_of(Iter1 first1, Sent1 last1, Iter2 first2,
        Sent2 last2, Pred&& op = Pred(), Proj1&& proj1 = Proj1(),
        Proj2&& proj2 = Proj2());

    /// Returns the last element in the range [first, last) that is equal
    /// to value
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the operator==().
    ///
    /// \tparam Iter        The type of the begin source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     bidirectional iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     sentinel for Iter.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    /// \tparam T           The type of the value to find (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param val          the value to compare the elements to
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a find_last algorithm returns the last element in the range
    ///           [first,last) that is equal to \a val.
    ///           If no such element in the range of [first,last) is equal to
    ///           \a val, then the algorithm returns \a last.
    ///
    template <typename Iter, typename Sent,
        typename Proj = hpx::identity,
        typename T = typename hpx::parallel::traits::projected<Iter,
            Proj>::value_type>
    Iter find_last(Iter first, Sent last, T const& val, Proj&& proj = Proj());

    /// Returns the last element in the range [first, last) that is equal
    /// to value
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the operator==().
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a bidirectional iterator.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    /// \tparam T           The type of the value to find (deduced).
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param val          the value to compare the elements to
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a find_last algorithm returns the last element in the range
    ///           [first,last) that is equal to \a val.
    ///           If no such element in the range of [first,last) is equal to
    ///           \a val, then the algorithm returns \a last.
    ///
    template <typename Rng,
        typename Proj = hpx::identity,
        typename T = typename hpx::parallel::traits::projected<
            typename std::ranges::iterator_t<Rng>, Proj>::value_type>
    typename std::ranges::iterator_t<Rng>
    find_last(Rng&& rng, T const& val, Proj&& proj = Proj());

    /// Returns the last element in the range [first, last) for which
    /// predicate \a pred returns true.
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam Iter        The type of the begin source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     bidirectional iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     sentinel for Iter.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param pred         The unary predicate which returns true for the
    ///                     required element. The signature of the predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a find_last_if algorithm returns the last element in the range
    ///           [first,last) that satisfies the predicate \a pred.
    ///           If no such element exists, the algorithm returns \a last.
    ///
    template <typename Iter, typename Sent, typename Pred,
        typename Proj = hpx::identity>
    Iter find_last_if(Iter first, Sent last, Pred&& pred, Proj&& proj = Proj());

    /// Returns the last element in the range [first, last) for which
    /// predicate \a pred returns true.
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a bidirectional iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param pred         The unary predicate which returns true for the
    ///                     required element. The signature of the predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a find_last_if algorithm returns the last element in the range
    ///           [first,last) that satisfies the predicate \a pred.
    ///           If no such element exists, the algorithm returns \a last.
    ///
    template <typename Rng, typename Pred, typename Proj = hpx::identity>
    typename std::ranges::iterator_t<Rng>
    find_last_if(Rng&& rng, Pred&& pred, Proj&& proj = Proj());

    /// Returns the last element in the range [first, last) for which
    /// predicate \a pred returns false.
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam Iter        The type of the begin source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     bidirectional iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     sentinel for Iter.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param pred         The unary predicate which returns false for the
    ///                     required element. The signature of the predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a find_last_if_not algorithm returns the last element in the
    ///           range [first,last) that does not satisfy the predicate \a pred.
    ///           If no such element exists, the algorithm returns \a last.
    ///
    template <typename Iter, typename Sent, typename Pred,
        typename Proj = hpx::identity>
    Iter find_last_if_not(Iter first, Sent last, Pred&& pred, Proj&& proj = Proj());

    /// Returns the last element in the range [first, last) for which
    /// predicate \a pred returns false.
    ///
    /// \note   Complexity: At most last - first
    ///         applications of the predicate.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of a bidirectional iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param pred         The unary predicate which returns false for the
    ///                     required element. The signature of the predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a find_last_if_not algorithm returns the last element in the
    ///           range [first,last) that does not satisfy the predicate \a pred.
    ///           If no such element exists, the algorithm returns \a last.
    ///
    template <typename Rng, typename Pred, typename Proj = hpx::identity>
    typename std::ranges::iterator_t<Rng>
    find_last_if_not(Rng&& rng, Pred&& pred, Proj&& proj = Proj());

    /// \brief Execution-policy overload of \c find.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename Proj = hpx::identity,
        typename T = std::remove_cvref_t<
            std::invoke_result_t<Proj&, std::iter_value_t<I>&>>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirect_binary_predicate<std::ranges::equal_to,
            std::projected<I, Proj>, T const*>
    parallel::util::detail::algorithm_result_t<ExPolicy, I>
    find(ExPolicy&& policy, I first, S last, T const& val, Proj proj = {});

    /// \brief Execution-policy overload of \c find.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        typename Proj = hpx::identity,
        typename T = std::remove_cvref_t<
            std::invoke_result_t<Proj&, std::iter_value_t<std::ranges::iterator_t<R>>&>>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::indirect_binary_predicate<std::ranges::equal_to,
            std::projected<std::ranges::iterator_t<R>, Proj>, T const*>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_iterator_t<R>>
    find(ExPolicy&& policy, R&& rng, T const& val, Proj proj = {});

    /// \brief Execution-policy overload of \c find_if.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename F,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirect_unary_predicate<F, std::projected<I, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy, I>
    find_if(ExPolicy&& policy, I first, S last, F f, Proj proj = {});

    /// \brief Execution-policy overload of \c find_if.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R, typename F,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::indirect_unary_predicate<F, std::projected<std::ranges::iterator_t<R>, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_iterator_t<R>>
    find_if(ExPolicy&& policy, R&& rng, F f, Proj proj = {});

    /// \brief Execution-policy overload of \c find_if_not.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename F,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirect_unary_predicate<F, std::projected<I, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy, I>
    find_if_not(ExPolicy&& policy, I first, S last, F f, Proj proj = {});

    /// \brief Execution-policy overload of \c find_if_not.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R, typename F,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::indirect_unary_predicate<F, std::projected<std::ranges::iterator_t<R>, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_iterator_t<R>>
    find_if_not(ExPolicy&& policy, R&& rng, F f, Proj proj = {});

    /// \brief Execution-policy overload of \c find_end.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R1,
        std::ranges::random_access_range R2,
        typename Pred = std::ranges::equal_to,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R1> && std::ranges::sized_range<R2> &&
        std::indirectly_comparable<std::ranges::iterator_t<R1>,
            std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R1>>
    find_end(ExPolicy&& policy, R1&& rng1,
        R2&& rng2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {});

    /// \brief Execution-policy overload of \c find_end.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I1,
        std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
        std::sized_sentinel_for<I2> S2,
        typename Pred = std::ranges::equal_to,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I1>>
    find_end(ExPolicy&& policy, I1 first1,
        S1 last1, I2 first2, S2 last2, Pred pred = {},
        Proj1 proj1 = {}, Proj2 proj2 = {});

    /// \brief Execution-policy overload of \c find_first_of.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R1,
        std::ranges::random_access_range R2,
        typename Pred = std::ranges::equal_to,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R1> && std::ranges::sized_range<R2> &&
        std::indirectly_comparable<std::ranges::iterator_t<R1>,
            std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_iterator_t<R1>>
    find_first_of(ExPolicy&& policy, R1&& rng1,
        R2&& rng2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {});

    /// \brief Execution-policy overload of \c find_first_of.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I1,
        std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
        std::sized_sentinel_for<I2> S2,
        typename Pred = std::ranges::equal_to,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    parallel::util::detail::algorithm_result_t<ExPolicy, I1>
    find_first_of(ExPolicy&& policy, I1 first1,
        S1 last1, I2 first2, S2 last2, Pred pred = {},
        Proj1 proj1 = {}, Proj2 proj2 = {});

    /// \brief Execution-policy overload of \c find_last.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename Proj = hpx::identity,
        typename T = std::remove_cvref_t<
            std::invoke_result_t<Proj&, std::iter_value_t<I>&>>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirect_binary_predicate<std::ranges::equal_to,
            std::projected<I, Proj>, T const*>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I>>
    find_last(ExPolicy&& policy, I first, S last, T const& val, Proj proj = {});

    /// \brief Execution-policy overload of \c find_last.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        typename Proj = hpx::identity,
        typename T = std::remove_cvref_t<
            std::invoke_result_t<Proj&, std::iter_value_t<std::ranges::iterator_t<R>>&>>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::indirect_binary_predicate<std::ranges::equal_to,
            std::projected<std::ranges::iterator_t<R>, Proj>, T const*>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R>>
    find_last(ExPolicy&& policy, R&& rng, T const& val, Proj proj = {});

    /// \brief Execution-policy overload of \c find_last_if.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename F,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirect_unary_predicate<F, std::projected<I, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I>>
    find_last_if(ExPolicy&& policy, I first, S last, F f, Proj proj = {});

    /// \brief Execution-policy overload of \c find_last_if.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R, typename F,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::indirect_unary_predicate<F, std::projected<std::ranges::iterator_t<R>, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R>>
    find_last_if(ExPolicy&& policy, R&& rng, F f, Proj proj = {});

    /// \brief Execution-policy overload of \c find_last_if_not.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename F,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirect_unary_predicate<F, std::projected<I, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I>>
    find_last_if_not(ExPolicy&& policy, I first, S last, F f, Proj proj = {});

    /// \brief Execution-policy overload of \c find_last_if_not.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R, typename F,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::indirect_unary_predicate<F, std::projected<std::ranges::iterator_t<R>, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R>>
    find_last_if_not(ExPolicy&& policy, R&& rng, F f, Proj proj = {});

    // clang-format on
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/algorithm_value.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/find.hpp>

#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::find
    HPX_CXX_CORE_EXPORT inline constexpr struct find_t final
      : hpx::detail::tag_dispatch<find_t,
            hpx::detail::tag_parallel_algorithm<find_t>>
    {
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::iter_value_t<I>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirect_binary_predicate<std::ranges::equal_to,
                std::projected<I, Proj>, T const*>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, T const& val, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::detail::find_if<I>().call(
                HPX_FORWARD(ExPolicy, policy), first, end,
                parallel::detail::equal_to_value(val), HPX_MOVE(proj));
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<std::invoke_result_t<Proj&,
                std::iter_value_t<std::ranges::iterator_t<R>>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::indirect_binary_predicate<std::ranges::equal_to,
                std::projected<std::ranges::iterator_t<R>, Proj>, T const*>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, T const& val, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), val, HPX_MOVE(proj)),
                [](I result) -> std::ranges::borrowed_iterator_t<R> {
                    return result;
                });
        }

        template <typename Iter, typename Sent, typename Proj = hpx::identity,
            typename T = typename hpx::parallel::traits::projected<Iter,
                Proj>::value_type>
        // clang-format off
            requires(
                std::sentinel_for<Sent, Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter>
            )
        // clang-format on
        static Iter invoke_default(
            Iter first, Sent last, T const& val, Proj&& proj = Proj())
        {
            static_assert(
                std::input_iterator<Iter>, "Requires at least input iterator.");

            return hpx::parallel::detail::find<Iter>().call(
                hpx::execution::seq, first, last, val, HPX_FORWARD(Proj, proj));
        }

        template <typename Rng, typename Proj = hpx::identity,
            typename T = typename hpx::parallel::traits::projected<
                std::ranges::iterator_t<Rng>, Proj>::value_type>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng>
            )
        // clang-format on
        static std::ranges::iterator_t<Rng> invoke_default(
            Rng&& rng, T const& val, Proj&& proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::input_iterator<iterator_type>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::find<iterator_type>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                val, HPX_FORWARD(Proj, proj));
        }
        using base_type = hpx::detail::tag_dispatch<find_t,
            hpx::detail::tag_parallel_algorithm<find_t>>;
        using base_type::operator();

        /// \brief Supports list-initialized values in policy iterator calls.
        template <typename ExPolicy, typename Iter, typename Sent,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::iter_value_t<Iter>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::random_access_iterator<Iter> &&
            std::sized_sentinel_for<Sent, Iter> &&
            requires(ExPolicy&& policy, Iter first, Sent last, T const& value,
                Proj proj) {
                invoke_default(HPX_FORWARD(ExPolicy, policy), first, last,
                    value, HPX_MOVE(proj));
            }
        decltype(auto) operator()(ExPolicy&& policy, Iter first, Sent last,
            T const& value, Proj proj = Proj()) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy), first,
                last, value, HPX_MOVE(proj));
        }

        /// \brief Supports list-initialized values in policy range calls.
        template <typename ExPolicy, typename Rng,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::ranges::range_value_t<Rng>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::random_access_range<Rng> &&
            std::ranges::sized_range<Rng> &&
            requires(ExPolicy&& policy, Rng&& rng, T const& value, Proj proj) {
                invoke_default(HPX_FORWARD(ExPolicy, policy),
                    HPX_FORWARD(Rng, rng), value, HPX_MOVE(proj));
            }
        decltype(auto) operator()(ExPolicy&& policy, Rng&& rng, T const& value,
            Proj proj = Proj()) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy),
                HPX_FORWARD(Rng, rng), value, HPX_MOVE(proj));
        }
    } find{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::find_if
    HPX_CXX_CORE_EXPORT inline constexpr struct find_if_t final
      : hpx::detail::tag_dispatch<find_if_t,
            hpx::detail::tag_parallel_algorithm<find_if_t>>
    {
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename F,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirect_unary_predicate<F, std::projected<I, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, F f, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::detail::find_if<I>().call(
                HPX_FORWARD(ExPolicy, policy), first, end, HPX_MOVE(f),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename F, typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::indirect_unary_predicate<F,
                std::projected<std::ranges::iterator_t<R>, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, F f, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), HPX_MOVE(f),
                    HPX_MOVE(proj)),
                [](I result) -> std::ranges::borrowed_iterator_t<R> {
                    return result;
                });
        }

        template <typename Iter, typename Sent, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::sentinel_for<Sent, Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<Iter>::value_type
                >
            )
        // clang-format on
        static Iter invoke_default(
            Iter first, Sent last, Pred pred, Proj proj = Proj())
        {
            static_assert(
                std::input_iterator<Iter>, "Requires at least input iterator.");

            return hpx::parallel::detail::find_if<Iter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(pred),
                HPX_MOVE(proj));
        }

        template <typename Rng, typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<
                        std::ranges::iterator_t<Rng>
                    >::value_type>
            )
        // clang-format on
        static std::ranges::iterator_t<Rng> invoke_default(
            Rng&& rng, Pred pred, Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::input_iterator<iterator_type>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::find_if<iterator_type>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                HPX_MOVE(pred), HPX_MOVE(proj));
        }
    } find_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::find_if_not
    HPX_CXX_CORE_EXPORT inline constexpr struct find_if_not_t final
      : hpx::detail::tag_dispatch<find_if_not_t,
            hpx::detail::tag_parallel_algorithm<find_if_not_t>>
    {
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename F,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirect_unary_predicate<F, std::projected<I, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, F f, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::detail::find_if_not<I>().call(
                HPX_FORWARD(ExPolicy, policy), first, end, HPX_MOVE(f),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename F, typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::indirect_unary_predicate<F,
                std::projected<std::ranges::iterator_t<R>, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, F f, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), HPX_MOVE(f),
                    HPX_MOVE(proj)),
                [](I result) -> std::ranges::borrowed_iterator_t<R> {
                    return result;
                });
        }

        template <typename Iter, typename Sent, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::sentinel_for<Sent, Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<Iter>::value_type
                >
            )
        // clang-format on
        static Iter invoke_default(
            Iter first, Sent last, Pred pred, Proj proj = Proj())
        {
            static_assert(
                std::input_iterator<Iter>, "Requires at least input iterator.");

            return hpx::parallel::detail::find_if_not<Iter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(pred),
                HPX_MOVE(proj));
        }

        template <typename Rng, typename Pred, typename Proj = hpx::identity>
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
            Rng&& rng, Pred pred, Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::input_iterator<iterator_type>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::find_if_not<iterator_type>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                HPX_MOVE(pred), HPX_MOVE(proj));
        }
    } find_if_not{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::find_end
    HPX_CXX_CORE_EXPORT inline constexpr struct find_end_t final
      : hpx::detail::tag_dispatch<find_end_t,
            hpx::detail::tag_parallel_algorithm<find_end_t>>
    {
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
            auto first1 = std::ranges::begin(rng1);
            auto first2 = std::ranges::begin(rng2);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first1,
                    first1 + std::ranges::distance(rng1), first2,
                    first2 + std::ranges::distance(rng2), HPX_MOVE(pred),
                    HPX_MOVE(proj1), HPX_MOVE(proj2)),
                [](std::ranges::subrange<I1> result)
                    -> std::ranges::borrowed_subrange_t<R1> { return result; });
        }

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
            return parallel::util::detail::convert_to_result(
                parallel::detail::find_end<I1>().call(
                    HPX_FORWARD(ExPolicy, policy), first1, end1, first2, end2,
                    HPX_MOVE(pred), HPX_MOVE(proj1), HPX_MOVE(proj2)),
                [end1, size = end2 - first2](
                    I1 found) -> std::ranges::subrange<I1> {
                    return {found, found == end1 ? end1 : found + size};
                });
        }

        template <typename Rng1, typename Rng2, typename Pred = equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::parallel::traits::is_projected_range_v<Proj1, Rng1> &&
                hpx::parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected_range<Proj1, Rng1>,
                    hpx::parallel::traits::projected_range<Proj2, Rng2>
                >
            )
        // clang-format on
        static std::ranges::iterator_t<Rng1> invoke_default(Rng1&& rng1,
            Rng2&& rng2, Pred op = Pred(), Proj1 proj1 = Proj1(),
            Proj2 proj2 = Proj2())
        {
            using iterator_type = std::ranges::iterator_t<Rng1>;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng2>>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find_end<iterator_type>().call(
                hpx::execution::seq, hpx::util::begin(rng1),
                hpx::util::end(rng1), hpx::util::begin(rng2),
                hpx::util::end(rng2), HPX_MOVE(op), HPX_MOVE(proj1),
                HPX_MOVE(proj2));
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
        static Iter1 invoke_default(Iter1 first1, Sent1 last1, Iter2 first2,
            Sent2 last2, Pred op = Pred(), Proj1 proj1 = Proj1(),
            Proj2 proj2 = Proj2())
        {
            static_assert(std::forward_iterator<Iter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<Iter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::find_end<Iter1>().call(
                hpx::execution::seq, first1, last1, first2, last2, HPX_MOVE(op),
                HPX_MOVE(proj1), HPX_MOVE(proj2));
        }
    } find_end{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::find_first_of
    HPX_CXX_CORE_EXPORT inline constexpr struct find_first_of_t final
      : hpx::detail::tag_dispatch<find_first_of_t,
            hpx::detail::tag_parallel_algorithm<find_first_of_t>>
    {
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
            auto first1 = std::ranges::begin(rng1);
            auto first2 = std::ranges::begin(rng2);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first1,
                    first1 + std::ranges::distance(rng1), first2,
                    first2 + std::ranges::distance(rng2), HPX_MOVE(pred),
                    HPX_MOVE(proj1), HPX_MOVE(proj2)),
                [](I1 result) -> std::ranges::borrowed_iterator_t<R1> {
                    return result;
                });
        }

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
            return parallel::detail::find_first_of<I1>().call(
                HPX_FORWARD(ExPolicy, policy), first1, end1, first2, end2,
                HPX_MOVE(pred), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename Rng1, typename Rng2, typename Pred = equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::parallel::traits::is_projected_range_v<Proj1, Rng1> &&
                hpx::parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected_range<Proj1, Rng1>,
                    hpx::parallel::traits::projected_range<Proj2, Rng2>
                >
            )
        // clang-format on
        static std::ranges::iterator_t<Rng1> invoke_default(Rng1&& rng1,
            Rng2&& rng2, Pred op = Pred(), Proj1 proj1 = Proj1(),
            Proj2 proj2 = Proj2())
        {
            using iterator_type = std::ranges::iterator_t<Rng1>;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng2>>,
                "Subsequence requires at least forward iterator.");

            return hpx::parallel::detail::find_first_of<iterator_type>().call(
                hpx::execution::seq, hpx::util::begin(rng1),
                hpx::util::end(rng1), hpx::util::begin(rng2),
                hpx::util::end(rng2), HPX_MOVE(op), HPX_MOVE(proj1),
                HPX_MOVE(proj2));
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
        static Iter1 invoke_default(Iter1 first1, Sent1 last1, Iter2 first2,
            Sent2 last2, Pred op = Pred(), Proj1 proj1 = Proj1(),
            Proj2 proj2 = Proj2())
        {
            static_assert(std::forward_iterator<Iter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<Iter2>,
                "Subsequence requires at least forward iterator.");

            return hpx::parallel::detail::find_first_of<Iter1>().call(
                hpx::execution::seq, first1, last1, first2, last2, HPX_MOVE(op),
                HPX_MOVE(proj1), HPX_MOVE(proj2));
        }
    } find_first_of{};

    namespace detail {

        // Helper: given a future<Iter> (or Iter) and a Sentinel, produce an
        // algorithm_result for a subrange_t<Iter, Sent>. This avoids
        // repeating the same async/sync pattern in every find_last* CPO.
        template <typename ExPolicy, typename Iter, typename Sent>
        hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            hpx::ranges::subrange_t<Iter, Sent>>
        make_subrange_result(
            hpx::parallel::util::detail::algorithm_result_t<ExPolicy, Iter>&&
                result,
            Sent last)
        {
            using subrange_type = hpx::ranges::subrange_t<Iter, Sent>;
            using result_type =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    subrange_type>;

            if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
            {
                if constexpr (hpx::execution_policy_has_scheduler_executor_v<
                                  ExPolicy>)
                {
                    return result_type::get(hpx::execution::experimental::then(
                        HPX_MOVE(result),
                        [last](Iter it) { return subrange_type(it, last); }));
                }
                else
                {
                    return result_type::get(
                        HPX_MOVE(result).then([last](hpx::future<Iter>&& f) {
                            return subrange_type(f.get(), last);
                        }));
                }
            }
            else
            {
                return result_type::get(subrange_type(HPX_MOVE(result), last));
            }
        }

    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::find_last
    HPX_CXX_CORE_EXPORT inline constexpr struct find_last_t final
      : hpx::detail::tag_dispatch<find_last_t,
            hpx::detail::tag_parallel_algorithm<find_last_t>>
    {
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::iter_value_t<I>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirect_binary_predicate<std::ranges::equal_to,
                std::projected<I, Proj>, T const*>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, T const& val, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::util::detail::convert_to_result(
                parallel::detail::find_last_if<I>().call(
                    HPX_FORWARD(ExPolicy, policy), first, end,
                    parallel::detail::equal_to_value(val), HPX_MOVE(proj)),
                [end](I found) -> std::ranges::subrange<I> {
                    return {found, end};
                });
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<std::invoke_result_t<Proj&,
                std::iter_value_t<std::ranges::iterator_t<R>>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::indirect_binary_predicate<std::ranges::equal_to,
                std::projected<std::ranges::iterator_t<R>, Proj>, T const*>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, T const& val, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), val, HPX_MOVE(proj)),
                [](std::ranges::subrange<I> result)
                    -> std::ranges::borrowed_subrange_t<R> { return result; });
        }

        template <typename Iter, typename Sent, typename T,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::sentinel_for<Sent, Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter>
            )
        // clang-format on
        static hpx::ranges::subrange_t<Iter, Sent> invoke_default(
            Iter first, Sent last, T const& val, Proj proj = Proj())
        {
            static_assert(std::bidirectional_iterator<Iter>,
                "Requires at least bidirectional iterator.");

            return hpx::ranges::subrange_t<Iter, Sent>(
                hpx::parallel::detail::find_last<Iter>().call(
                    hpx::execution::seq, first, last, val, HPX_MOVE(proj)),
                last);
        }

        template <typename Rng, typename T, typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng>
            )
        // clang-format on
        static hpx::ranges::subrange_t<std::ranges::iterator_t<Rng>,
            std::ranges::sentinel_t<Rng>>
        invoke_default(Rng&& rng, T const& val, Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::bidirectional_iterator<iterator_type>,
                "Requires at least bidirectional iterator.");

            return hpx::ranges::subrange_t<iterator_type,
                std::ranges::sentinel_t<Rng>>(
                hpx::parallel::detail::find_last<iterator_type>().call(
                    hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), val, HPX_MOVE(proj)),
                hpx::util::end(rng));
        }
        using base_type = hpx::detail::tag_dispatch<find_last_t,
            hpx::detail::tag_parallel_algorithm<find_last_t>>;
        using base_type::operator();

        /// \brief Supports list-initialized values in policy iterator calls.
        template <typename ExPolicy, typename Iter, typename Sent,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::iter_value_t<Iter>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::random_access_iterator<Iter> &&
            std::sized_sentinel_for<Sent, Iter> &&
            requires(ExPolicy&& policy, Iter first, Sent last, T const& value,
                Proj proj) {
                invoke_default(HPX_FORWARD(ExPolicy, policy), first, last,
                    value, HPX_MOVE(proj));
            }
        decltype(auto) operator()(ExPolicy&& policy, Iter first, Sent last,
            T const& value, Proj proj = Proj()) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy), first,
                last, value, HPX_MOVE(proj));
        }

        /// \brief Supports list-initialized values in policy range calls.
        template <typename ExPolicy, typename Rng,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::ranges::range_value_t<Rng>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::random_access_range<Rng> &&
            std::ranges::sized_range<Rng> &&
            requires(ExPolicy&& policy, Rng&& rng, T const& value, Proj proj) {
                invoke_default(HPX_FORWARD(ExPolicy, policy),
                    HPX_FORWARD(Rng, rng), value, HPX_MOVE(proj));
            }
        decltype(auto) operator()(ExPolicy&& policy, Rng&& rng, T const& value,
            Proj proj = Proj()) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy),
                HPX_FORWARD(Rng, rng), value, HPX_MOVE(proj));
        }
    } find_last{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::find_last_if
    HPX_CXX_CORE_EXPORT inline constexpr struct find_last_if_t final
      : hpx::detail::tag_dispatch<find_last_if_t,
            hpx::detail::tag_parallel_algorithm<find_last_if_t>>
    {
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename F,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirect_unary_predicate<F, std::projected<I, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, F f, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::util::detail::convert_to_result(
                parallel::detail::find_last_if<I>().call(
                    HPX_FORWARD(ExPolicy, policy), first, end, HPX_MOVE(f),
                    HPX_MOVE(proj)),
                [end](I found) -> std::ranges::subrange<I> {
                    return {found, end};
                });
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename F, typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::indirect_unary_predicate<F,
                std::projected<std::ranges::iterator_t<R>, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, F f, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), HPX_MOVE(f),
                    HPX_MOVE(proj)),
                [](std::ranges::subrange<I> result)
                    -> std::ranges::borrowed_subrange_t<R> { return result; });
        }

        template <typename Iter, typename Sent, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::sentinel_for<Sent, Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<Iter>::value_type
                >
            )
        // clang-format on
        static hpx::ranges::subrange_t<Iter, Sent> invoke_default(
            Iter first, Sent last, Pred pred, Proj proj = Proj())
        {
            static_assert(std::bidirectional_iterator<Iter>,
                "Requires at least bidirectional iterator.");

            return hpx::ranges::subrange_t<Iter, Sent>(
                hpx::parallel::detail::find_last_if<Iter>().call(
                    hpx::execution::seq, first, last, HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                last);
        }

        template <typename Rng, typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<
                        std::ranges::iterator_t<Rng>
                    >::value_type>
            )
        // clang-format on
        static hpx::ranges::subrange_t<std::ranges::iterator_t<Rng>,
            std::ranges::sentinel_t<Rng>>
        invoke_default(Rng&& rng, Pred pred, Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::bidirectional_iterator<iterator_type>,
                "Requires at least bidirectional iterator.");

            return hpx::ranges::subrange_t<iterator_type,
                std::ranges::sentinel_t<Rng>>(
                hpx::parallel::detail::find_last_if<iterator_type>().call(
                    hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), HPX_MOVE(pred), HPX_MOVE(proj)),
                hpx::util::end(rng));
        }
    } find_last_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::find_last_if_not
    HPX_CXX_CORE_EXPORT inline constexpr struct find_last_if_not_t final
      : hpx::detail::tag_dispatch<find_last_if_not_t,
            hpx::detail::tag_parallel_algorithm<find_last_if_not_t>>
    {
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename F,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirect_unary_predicate<F, std::projected<I, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, F f, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::util::detail::convert_to_result(
                parallel::detail::find_last_if_not<I>().call(
                    HPX_FORWARD(ExPolicy, policy), first, end, HPX_MOVE(f),
                    HPX_MOVE(proj)),
                [end](I found) -> std::ranges::subrange<I> {
                    return {found, end};
                });
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename F, typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::indirect_unary_predicate<F,
                std::projected<std::ranges::iterator_t<R>, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, F f, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), HPX_MOVE(f),
                    HPX_MOVE(proj)),
                [](std::ranges::subrange<I> result)
                    -> std::ranges::borrowed_subrange_t<R> { return result; });
        }

        template <typename Iter, typename Sent, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::sentinel_for<Sent, Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<Iter>::value_type
                >
            )
        // clang-format on
        static hpx::ranges::subrange_t<Iter, Sent> invoke_default(
            Iter first, Sent last, Pred pred, Proj proj = Proj())
        {
            static_assert(std::bidirectional_iterator<Iter>,
                "Requires at least bidirectional iterator.");

            return hpx::ranges::subrange_t<Iter, Sent>(
                hpx::parallel::detail::find_last_if_not<Iter>().call(
                    hpx::execution::seq, first, last, HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                last);
        }

        template <typename Rng, typename Pred, typename Proj = hpx::identity>
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
        static hpx::ranges::subrange_t<std::ranges::iterator_t<Rng>,
            std::ranges::sentinel_t<Rng>>
        invoke_default(Rng&& rng, Pred pred, Proj proj = Proj())
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::bidirectional_iterator<iterator_type>,
                "Requires at least bidirectional iterator.");

            return hpx::ranges::subrange_t<iterator_type,
                std::ranges::sentinel_t<Rng>>(
                hpx::parallel::detail::find_last_if_not<iterator_type>().call(
                    hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), HPX_MOVE(pred), HPX_MOVE(proj)),
                hpx::util::end(rng));
        }
    } find_last_if_not{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
