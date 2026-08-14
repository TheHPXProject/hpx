//  Copyright (c) 2015-2023 Hartmut Kaiser
//  Copyright (c) 2021 Giannis Gonidelis
//  Copyright (c) 2022 Dimitra Karatza
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/transform.hpp
/// \page hpx::ranges::transform
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
    // clang-format off

    /// Applies the given function \a f to the given range \a rng and stores
    /// the result in another range, beginning at dest.
    ///
    /// \note   Complexity: Exactly size(rng) applications of \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the invocations of \a f.
    /// \tparam FwdIter1    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent1       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     sentinel for FwdIter1.
    /// \tparam FwdIter2    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the first sequence of
    ///                     elements the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to \a Type. The type \a Ret
    ///                     must be such that an object of type \a FwdIter2 can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a f is invoked.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform algorithm returns a
    ///           \a hpx::future<ranges::unary_transform_result<FwdIter1, FwdIter2> >
    ///           if the execution policy is of type \a parallel_task_policy
    ///           and returns \a ranges::unary_transform_result<FwdIter1, FwdIter2>
    ///           otherwise.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the input sequence and
    ///           the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Sent1,
        typename FwdIter2, typename F,
        typename Proj = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        ranges::unary_transform_result<FwdIter1, FwdIter2>>::type
    transform(ExPolicy&& policy, FwdIter1 first, Sent1 last, FwdIter2 dest,
        F&& f, Proj&& proj = Proj());

    /// Applies the given function \a f to the given range \a rng and stores
    /// the result in another range, beginning at dest.
    ///
    /// \note   Complexity: Exactly size(rng) applications of \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the invocations of \a f.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The type \a Type must be such that an object of
    ///                     type \a range_iterator<Rng>::type can be dereferenced and then
    ///                     implicitly converted to \a Type. The type \a Ret
    ///                     must be such that an object of type \a OutIter can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a f is invoked.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform algorithm returns a
    ///           \a hpx::future<ranges::unary_transform_result<range_iterator<Rng>::type, FwdIter>>
    ///           if the execution policy is of type \a parallel_task_policy
    ///           and returns \a ranges::unary_transform_result<range_iterator<Rng>::type, FwdIter>
    ///           otherwise.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the input sequence and
    ///           the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename Rng, typename FwdIter,
        typename F, typename Proj = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        ranges::unary_transform_result<
            std::ranges::iterator_t<Rng>, FwdIter>>
    transform(ExPolicy&& policy, Rng&& rng, FwdIter dest, F&& f,
        Proj&& proj = Proj());

    /// Applies the given function \a f to pairs of elements from two ranges:
    /// one defined by \a rng and the other beginning at first2, and
    /// stores the result in another range, beginning at dest.
    ///
    /// \note   Complexity: Exactly size(rng) applications of \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the invocations of \a f.
    /// \tparam FwdIter1    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent1       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     sentinel for FwdIter1.
    /// \tparam FwdIter2    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent2       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     sentinel for FwdIter2.
    /// \tparam FwdIter3    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj1       The type of an optional projection function to be
    ///                     used for elements of the first sequence. This
    ///                     defaults to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function to be
    ///                     used for elements of the second sequence. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first1       Refers to the beginning of the first sequence of
    ///                     elements the algorithm will be applied to.
    /// \param last1        Refers to the end of the first sequence of elements
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the algorithm will be applied to.
    /// \param last2        Refers to the end of the second sequence of elements
    ///                     the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The types \a Type1 and \a Type2 must be such that
    ///                     objects of types FwdIter1 and FwdIter2 can be
    ///                     dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively. The type \a Ret
    ///                     must be such that an object of type \a FwdIter3 can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     first sequence as a projection operation before the
    ///                     actual predicate \a f is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     second sequence as a projection operation before
    ///                     the actual predicate \a f is invoked.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform algorithm returns A \a
    ///           hpx::future<ranges::binary_transform_result<FwdIter1, FwdIter2, FwdIter3>>
    ///           if the execution policy is of type \a parallel_task_policy
    ///           and returns
    ///           \a ranges::binary_transform_result<FwdIter1, FwdIter2, FwdIter3>
    ///           otherwise.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the first input sequence,
    ///           an iterator referring to the first element after the second
    ///           input sequence, and the output iterator referring to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Sent1,
        typename FwdIter2, typename Sent2, typename FwdIter3, typename F,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        ranges::binary_transform_result<FwdIter1, FwdIter2, FwdIter3>>::type
    transform(ExPolicy&& policy, FwdIter1 first1, Sent1 last1, FwdIter2 first2,
        Sent2 last2, FwdIter3 dest, F&& f, Proj1&& proj1 = Proj1(),
        Proj2&& proj2 = Proj2());

    /// Applies the given function \a f to pairs of elements from two ranges:
    /// one defined by [first1, last1) and the other beginning at first2, and
    /// stores the result in another range, beginning at dest.
    ///
    /// \note   Complexity: Exactly min(last2-first2, last1-first1)
    ///         applications of \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the invocations of \a f.
    /// \tparam Rng1        The type of the first source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Rng2        The type of the second source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj1       The type of an optional projection function to be
    ///                     used for elements of the first sequence. This
    ///                     defaults to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function to be
    ///                     used for elements of the second sequence. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng1         Refers to the first sequence of elements the
    ///                     algorithm will be applied to.
    /// \param rng2         Refers to the second sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The types \a Type1 and \a Type2 must be such that
    ///                     objects of types range_iterator<Rng1>::type and
    ///                     range_iterator<Rng2>::type can be
    ///                     dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively. The type \a Ret
    ///                     must be such that an object of type \a FwdIter can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     first sequence as a projection operation before the
    ///                     actual predicate \a f is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     second sequence as a projection operation before
    ///                     the actual predicate \a f is invoked.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note The algorithm will invoke the binary predicate until it reaches
    ///       the end of the shorter of the two given input sequences
    ///
    /// \returns  The \a transform algorithm returns a
    ///           \a hpx::future<ranges::binary_transform_result<
    ///           std::ranges::iterator_t<Rng1>,
    ///           std::ranges::iterator_t<Rng2>,
    ///           FwdIter> >
    ///           if the execution policy is of type \a parallel_task_policy
    ///           and returns
    ///           \a ranges::binary_transform_result<
    ///           std::ranges::iterator_t<Rng1>,
    ///           std::ranges::iterator_t<Rng2>,
    ///           FwdIter>
    ///           otherwise.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the first input sequence,
    ///           an iterator referring to the first element after the second
    ///           input sequence, and the output iterator referring to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename Rng1, typename Rng2, typename FwdIter,
        typename F, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        ranges::binary_transform_result<
            std::ranges::iterator_t<Rng1>,
            std::ranges::iterator_t<Rng2>, FwdIter>>
    transform_t(ExPolicy&& policy, Rng1&& rng1, Rng2&& rng2, FwdIter dest,
        F&& f, Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    /// Applies the given function \a f to the given range \a rng and stores
    /// the result in another range, beginning at dest.
    ///
    /// \note   Complexity: Exactly size(rng) applications of \a f
    ///
    /// \tparam FwdIter1    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent1       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     sentinel for FwdIter1.
    /// \tparam FwdIter2    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the first sequence of
    ///                     elements the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to \a Type. The type \a Ret
    ///                     must be such that an object of type \a FwdIter2 can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a f is invoked.
    ///
    /// \returns  The \a transform algorithm returns \a
    ///           ranges::unary_transform_result<FwdIter1, FwdIter2>.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the input sequence and
    ///           the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename FwdIter1, typename Sent1, typename FwdIter2,
        typename F,
        typename Proj = hpx::identity>
    ranges::unary_transform_result<FwdIter1, FwdIter2>
    transform(FwdIter1 first, Sent1 last, FwdIter2 dest, F&& f,
        Proj&& proj = Proj());

    /// Applies the given function \a f to the given range \a rng and stores
    /// the result in another range, beginning at dest.
    ///
    /// \note   Complexity: Exactly size(rng) applications of \a f
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The type \a Type must be such that an object of
    ///                     type \a range_iterator<Rng>::type can be dereferenced and then
    ///                     implicitly converted to \a Type. The type \a Ret
    ///                     must be such that an object of type \a OutIter can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a f is invoked.
    ///
    /// \returns  The \a transform algorithm returns \a
    ///           ranges::unary_transform_result<range_iterator<Rng>::type, FwdIter>.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the input sequence and
    ///           the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename Rng, typename FwdIter,
        typename F, typename Proj = hpx::identity>
    ranges::unary_transform_result<
        std::ranges::iterator_t<Rng>, FwdIter>
    transform(Rng&& rng, FwdIter dest, F&& f, Proj&& proj = Proj());

    /// Applies the given function \a f to pairs of elements from two ranges:
    /// one defined by \a rng and the other beginning at first2, and
    /// stores the result in another range, beginning at dest.
    ///
    /// \note   Complexity: Exactly size(rng) applications of \a f
    ///
    /// \tparam FwdIter1    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent1       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     sentinel for FwdIter1.
    /// \tparam FwdIter2    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent2       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     sentinel for FwdIter2.
    /// \tparam FwdIter3    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj1       The type of an optional projection function to be
    ///                     used for elements of the first sequence. This
    ///                     defaults to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function to be
    ///                     used for elements of the second sequence. This
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
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The types \a Type1 and \a Type2 must be such that
    ///                     objects of types FwdIter1 and FwdIter2 can be
    ///                     dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively. The type \a Ret
    ///                     must be such that an object of type \a FwdIter3 can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     first sequence as a projection operation before the
    ///                     actual predicate \a f is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     second sequence as a projection operation before
    ///                     the actual predicate \a f is invoked.
    ///
    ///
    /// \returns  The \a transform algorithm returns \a
    ///           ranges::binary_transform_result<FwdIter1, FwdIter2, FwdIter3>.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the first input sequence,
    ///           an iterator referring to the first element after the second
    ///           input sequence, and the output iterator referring to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename FwdIter1, typename Sent1,
        typename FwdIter2, typename Sent2, typename FwdIter3, typename F,
        typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    ranges::binary_transform_result<FwdIter1, FwdIter2, FwdIter3>
    transform(FwdIter1 first1, Sent1 last1, FwdIter2 first2,
        Sent2 last2, FwdIter3 dest, F&& f, Proj1&& proj1 = Proj1(),
        Proj2&& proj2 = Proj2());

    /// Applies the given function \a f to pairs of elements from two ranges:
    /// one defined by [first1, last1) and the other beginning at first2, and
    /// stores the result in another range, beginning at dest.
    ///
    /// \note   Complexity: Exactly min(last2-first2, last1-first1)
    ///         applications of \a f
    ///
    /// \tparam Rng1        The type of the first source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Rng2        The type of the second source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj1       The type of an optional projection function to be
    ///                     used for elements of the first sequence. This
    ///                     defaults to \a hpx::identity
    /// \tparam Proj2       The type of an optional projection function to be
    ///                     used for elements of the second sequence. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng1         Refers to the first sequence of elements the
    ///                     algorithm will be applied to.
    /// \param rng2         Refers to the second sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The types \a Type1 and \a Type2 must be such that
    ///                     objects of types range_iterator<Rng1>::type and
    ///                     range_iterator<Rng2>::type can be
    ///                     dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively. The type \a Ret
    ///                     must be such that an object of type \a FwdIter can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     first sequence as a projection operation before the
    ///                     actual predicate \a f is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of the
    ///                     second sequence as a projection operation before
    ///                     the actual predicate \a f is invoked.
    ///
    /// \note The algorithm will invoke the binary predicate until it reaches
    ///       the end of the shorter of the two given input sequences
    ///
    /// \returns  The \a transform algorithm returns \a
    ///           ranges::binary_transform_result<
    ///           std::ranges::iterator_t<Rng1>,
    ///           std::ranges::iterator_t<Rng2>,
    ///           FwdIter>.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the first input sequence,
    ///           an iterator referring to the first element after the second
    ///           input sequence, and the output iterator referring to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename Rng1, typename Rng2, typename FwdIter,
        typename F, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    ranges::binary_transform_result<
        std::ranges::iterator_t<Rng1>,
        std::ranges::iterator_t<Rng2>, FwdIter>
    transform(Rng1&& rng1, Rng2&& rng2, FwdIter dest, F&& f,
        Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    // clang-format on
}}    // namespace hpx::ranges
#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/transform.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using unary_transform_result = parallel::util::in_out_result<I, O>;

    HPX_CXX_CORE_EXPORT template <typename I1, typename I2, typename O>
    using binary_transform_result = parallel::util::in_in_out_result<I1, I2, O>;

    ///////////////////////////////////////////////////////////////////////////
    // a for hpx::ranges::transform
    HPX_CXX_CORE_EXPORT inline constexpr struct transform_t final
      : hpx::detail::tag_dispatch<transform_t,
            hpx::detail::tag_parallel_algorithm<transform_t>>
    {
        template <typename ExPolicy, typename FwdIter1, typename Sent1,
            typename FwdIter2, typename F, typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent1, FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        static decltype(auto) invoke_default(ExPolicy&& policy, FwdIter1 first,
            Sent1 last, FwdIter2 dest, F f, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");

            return parallel::detail::transform<
                unary_transform_result<FwdIter1, FwdIter2>>()
                .call(HPX_FORWARD(ExPolicy, policy), first, last, dest,
                    HPX_MOVE(f), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng, typename FwdIter, typename F,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static decltype(auto) invoke_default(
            ExPolicy&& policy, Rng&& rng, FwdIter dest, F f, Proj proj = Proj())
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            return parallel::detail::transform<
                unary_transform_result<iterator_type, FwdIter>>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                    hpx::util::end(rng), dest, HPX_MOVE(f), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename FwdIter1, typename Sent1,
            typename FwdIter2, typename Sent2, typename FwdIter3, typename F,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent1, FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                std::sentinel_for<Sent2, FwdIter2> &&
                hpx::traits::is_iterator_v<FwdIter3>
            )
        // clang-format on
        static decltype(auto) invoke_default(ExPolicy&& policy, FwdIter1 first1,
            Sent1 last1, FwdIter2 first2, Sent2 last2, FwdIter3 dest, F f,
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            static_assert(std::forward_iterator<FwdIter1> &&
                    std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return parallel::detail::transform_binary2<
                binary_transform_result<FwdIter1, FwdIter2, FwdIter3>>()
                .call(HPX_FORWARD(ExPolicy, policy), first1, last1, first2,
                    last2, dest, HPX_MOVE(f), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename ExPolicy, typename Rng1, typename Rng2,
            typename FwdIter, typename F, typename Proj1 = hpx::identity,
            typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng1> &&
                std::ranges::range<Rng2> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static decltype(auto) invoke_default(ExPolicy&& policy, Rng1&& rng1,
            Rng2&& rng2, FwdIter dest, F f, Proj1 proj1 = Proj1(),
            Proj2 proj2 = Proj2())
        {
            using iterator_type1 =
                typename hpx::traits::range_traits<Rng1>::iterator_type;
            using iterator_type2 =
                typename hpx::traits::range_traits<Rng2>::iterator_type;

            static_assert(std::forward_iterator<iterator_type1> &&
                    std::forward_iterator<iterator_type2>,
                "Requires at least forward iterator.");

            return parallel::detail::transform_binary2<binary_transform_result<
                iterator_type1, iterator_type2, FwdIter>>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng1),
                    hpx::util::end(rng1), hpx::util::begin(rng2),
                    hpx::util::end(rng2), dest, HPX_MOVE(f), HPX_MOVE(proj1),
                    HPX_MOVE(proj2));
        }

        template <typename FwdIter1, typename Sent1, typename FwdIter2,
            typename F, typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent1, FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        static ranges::unary_transform_result<FwdIter1, FwdIter2>
        invoke_default(
            FwdIter1 first, Sent1 last, FwdIter2 dest, F f, Proj proj = Proj())
        {
            static_assert(std::input_iterator<FwdIter1>,
                "Requires at least input iterator.");

            return parallel::detail::transform<
                unary_transform_result<FwdIter1, FwdIter2>>()
                .call(hpx::execution::seq, first, last, dest, HPX_MOVE(f),
                    HPX_MOVE(proj));
        }

        template <typename Rng, typename FwdIter, typename F,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static ranges::unary_transform_result<std::ranges::iterator_t<Rng>,
            FwdIter>
        invoke_default(Rng&& rng, FwdIter dest, F f, Proj proj = Proj())
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::input_iterator<iterator_type>,
                "Requires at least input iterator.");

            return parallel::detail::transform<
                unary_transform_result<iterator_type, FwdIter>>()
                .call(hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), dest, HPX_MOVE(f), HPX_MOVE(proj));
        }

        template <typename FwdIter1, typename Sent1, typename FwdIter2,
            typename Sent2, typename FwdIter3, typename F,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent1, FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                std::sentinel_for<Sent2, FwdIter2> &&
                hpx::traits::is_iterator_v<FwdIter3>
            )
        // clang-format on
        static ranges::binary_transform_result<FwdIter1, FwdIter2, FwdIter3>
        invoke_default(FwdIter1 first1, Sent1 last1, FwdIter2 first2,
            Sent2 last2, FwdIter3 dest, F f, Proj1 proj1 = Proj1(),
            Proj2 proj2 = Proj2())
        {
            static_assert(
                std::input_iterator<FwdIter1> && std::input_iterator<FwdIter2>,
                "Requires at least input iterator.");

            return parallel::detail::transform_binary2<
                binary_transform_result<FwdIter1, FwdIter2, FwdIter3>>()
                .call(hpx::execution::seq, first1, last1, first2, last2, dest,
                    HPX_MOVE(f), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename Rng1, typename Rng2, typename FwdIter, typename F,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng1> &&
                std::ranges::range<Rng2> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static ranges::binary_transform_result<std::ranges::iterator_t<Rng1>,
            std::ranges::iterator_t<Rng2>, FwdIter>
        invoke_default(Rng1&& rng1, Rng2&& rng2, FwdIter dest, F f,
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            using iterator_type1 =
                typename hpx::traits::range_traits<Rng1>::iterator_type;
            using iterator_type2 =
                typename hpx::traits::range_traits<Rng2>::iterator_type;

            static_assert(std::forward_iterator<iterator_type1> &&
                    std::forward_iterator<iterator_type2>,
                "Requires at least forward iterator.");

            return parallel::detail::transform_binary2<binary_transform_result<
                iterator_type1, iterator_type2, FwdIter>>()
                .call(hpx::execution::seq, hpx::util::begin(rng1),
                    hpx::util::end(rng1), hpx::util::begin(rng2),
                    hpx::util::end(rng2), dest, HPX_MOVE(f), HPX_MOVE(proj1),
                    HPX_MOVE(proj2));
        }
    } transform{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
