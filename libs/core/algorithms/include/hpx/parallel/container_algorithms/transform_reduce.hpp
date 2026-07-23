//  Copyright (c) 2020-2023 Hartmut Kaiser
//  Copyright (c) 2022 Dimitra Karatza
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/transform_reduce.hpp
/// \page hpx::ranges::transform_reduce
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Returns GENERALIZED_SUM(red_op, init, conv_op(*first), ...,
    /// conv_op(*(first + (last - first) - 1))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicates \a red_op and \a conv_op.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Iter        The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the first sorted range
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for each of the values returned
    ///                     from the invocation of \a conv_op. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1, \a Type2, and \a Ret must be
    ///                     such that an object of a type as returned from
    ///                     \a conv_op can be implicitly converted to any
    ///                     of those types.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is a
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     R fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a Iter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///                     The type \a R must be such that an object of this
    ///                     type can be implicitly converted to \a T.
    ///
    /// The reduce operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a hpx::future<T> if the
    ///           execution policy is of type \a parallel_task_policy and
    ///           returns \a T otherwise.
    ///           The \a transform_reduce algorithm returns the result of the
    ///           generalized sum over the values returned from \a conv_op when
    ///           applied to the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a transform_reduce and \a accumulate is
    /// that the behavior of transform_reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename ExPolicy, typename Iter, typename Sent, typename T,
        typename Reduce, typename Convert>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
    transform_reduce(ExPolicy&& policy, Iter first, Sent last, T init,
        Reduce&& red_op, Convert&& conv_op);

    /// Returns GENERALIZED_SUM(red_op, init, conv_op(*first), ...,
    /// conv_op(*(first + (last - first) - 1))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicates \a red_op and \a conv_op.
    ///
    /// \tparam Iter        The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param first        Refers to the beginning of the first sorted range
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for each of the values returned
    ///                     from the invocation of \a conv_op. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1, \a Type2, and \a Ret must be
    ///                     such that an object of a type as returned from
    ///                     \a conv_op can be implicitly converted to any
    ///                     of those types.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is a
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     R fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a Iter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///                     The type \a R must be such that an object of this
    ///                     type can be implicitly converted to \a T.
    ///
    /// \returns  The \a transform_reduce algorithm returns \a T.
    ///           The \a transform_reduce algorithm returns the result of the
    ///           generalized sum over the values returned from \a conv_op when
    ///           applied to the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a transform_reduce and \a accumulate is
    /// that the behavior of transform_reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename Iter, typename Sent, typename T, typename Reduce,
        typename Convert>
    T transform_reduce(Iter first, Sent last, T init, Reduce&& red_op,
        Convert&& conv_op);

    /// Returns GENERALIZED_SUM(red_op, init, conv_op(*first), ...,
    /// conv_op(*(first + (last - first) - 1))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicates \a red_op and \a conv_op.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Iter        The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter.
    /// \tparam Iter2       The type of the source iterators used (deduced)
    ///                     representing the second sequence.
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the first sorted range
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    ///
    /// The reduce operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a hpx::future<T> if the
    ///           execution policy is of type \a parallel_task_policy and
    ///           returns \a T otherwise.
    ///           The \a transform_reduce algorithm returns the result of the
    ///           generalized sum over the values returned from \a conv_op when
    ///           applied to the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a transform_reduce and \a accumulate is
    /// that the behavior of transform_reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename ExPolicy, typename Iter, typename Sent,
        typename Iter2, typename T>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
    transform_reduce(ExPolicy&& policy, Iter first, Sent last, Iter2 first2, T init);

    /// Returns GENERALIZED_SUM(red_op, init, conv_op(*first), ...,
    /// conv_op(*(first + (last - first) - 1))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicates \a red_op and \a conv_op.
    ///
    /// \tparam Iter        The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter.
    /// \tparam Iter2       The type of the source iterators used (deduced)
    ///                     representing the second sequence.
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param first        Refers to the beginning of the first sorted range
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    ///
    /// \returns  The \a transform_reduce algorithm returns \a T.
    ///           The \a transform_reduce algorithm returns the result of the
    ///           generalized sum over the values returned from \a conv_op when
    ///           applied to the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a transform_reduce and \a accumulate is
    /// that the behavior of transform_reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename Iter, typename Sent, typename Iter2, typename T>
    T transform_reduce(Iter first, Sent last, Iter2 first2, T init);

    /// Returns GENERALIZED_SUM(red_op, init, conv_op(*first), ...,
    /// conv_op(*(first + (last - first) - 1))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicates \a red_op and \a conv_op.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Iter        The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter.
    /// \tparam Iter2       The type of the source iterators used (deduced)
    ///                     representing the second sequence.
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the first sorted range
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for each of the values returned
    ///                     from the invocation of \a conv_op. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1, \a Type2, and \a Ret must be
    ///                     such that an object of a type as returned from
    ///                     \a conv_op can be implicitly converted to any
    ///                     of those types.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is a
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     R fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a Iter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///                     The type \a R must be such that an object of this
    ///                     type can be implicitly converted to \a T.
    ///
    /// The reduce operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a hpx::future<T> if the
    ///           execution policy is of type \a parallel_task_policy and
    ///           returns \a T otherwise.
    ///           The \a transform_reduce algorithm returns the result of the
    ///           generalized sum over the values returned from \a conv_op when
    ///           applied to the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a transform_reduce and \a accumulate is
    /// that the behavior of transform_reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename ExPolicy, typename Iter, typename Sent,
        typename Iter2, typename T, typename Reduce, typename Convert>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
    transform_reduce(ExPolicy&& policy, Iter first,
        Sent last, Iter2 first2, T init, Reduce&& red_op, Convert&& conv_op);

    /// Returns GENERALIZED_SUM(red_op, init, conv_op(*first), ...,
    /// conv_op(*(first + (last - first) - 1))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicates \a red_op and \a conv_op.
    ///
    /// \tparam Iter        The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam Sent        The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter.
    /// \tparam Iter2       The type of the source iterators used (deduced)
    ///                     representing the second sequence.
    ///                     This iterator type must meet the requirements of an
    ///                     random access iterator.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param first        Refers to the beginning of the first sorted range
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the sequence of elements
    ///                     of the second range the algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for each of the values returned
    ///                     from the invocation of \a conv_op. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1, \a Type2, and \a Ret must be
    ///                     such that an object of a type as returned from
    ///                     \a conv_op can be implicitly converted to any
    ///                     of those types.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is a
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     R fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a Iter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///                     The type \a R must be such that an object of this
    ///                     type can be implicitly converted to \a T.
    ///
    /// \returns  The \a transform_reduce algorithm returns \a T.
    ///           The \a transform_reduce algorithm returns the result of the
    ///           generalized sum over the values returned from \a conv_op when
    ///           applied to the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a transform_reduce and \a accumulate is
    /// that the behavior of transform_reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename Iter, typename Sent, typename Iter2, typename T,
        typename Reduce, typename Convert>
    T transform_reduce(Iter first, Sent last, Iter2 first2, T init,
        Reduce&& red_op, Convert&& conv_op);

    /// Returns GENERALIZED_SUM(red_op, init, conv_op(*first), ...,
    /// conv_op(*(first + (last - first) - 1))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicates \a red_op and \a conv_op.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param init         The initial value for the generalized sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for each of the values returned
    ///                     from the invocation of \a conv_op. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1, \a Type2, and \a Ret must be
    ///                     such that an object of a type as returned from
    ///                     \a conv_op can be implicitly converted to any
    ///                     of those types.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is a
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     R fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a Iter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///                     The type \a R must be such that an object of this
    ///                     type can be implicitly converted to \a T.
    ///
    /// The reduce operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a hpx::future<T> if the
    ///           execution policy is of type \a parallel_task_policy and
    ///           returns \a T otherwise.
    ///           The \a transform_reduce algorithm returns the result of the
    ///           generalized sum over the values returned from \a conv_op when
    ///           applied to the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a transform_reduce and \a accumulate is
    /// that the behavior of transform_reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename ExPolicy, typename Rng, typename T, typename Reduce,
        typename Convert>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
    transform_reduce(ExPolicy&& policy, Rng&& rng, T init, Reduce&& red_op,
        Convert&& conv_op);

    /// Returns GENERALIZED_SUM(red_op, init, conv_op(*first), ...,
    /// conv_op(*(first + (last - first) - 1))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicates \a red_op and \a conv_op.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param init         The initial value for the generalized sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for each of the values returned
    ///                     from the invocation of \a conv_op. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1, \a Type2, and \a Ret must be
    ///                     such that an object of a type as returned from
    ///                     \a conv_op can be implicitly converted to any
    ///                     of those types.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is a
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     R fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a Iter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///                     The type \a R must be such that an object of this
    ///                     type can be implicitly converted to \a T.
    ///
    /// \returns  The \a transform_reduce algorithm returns \a T.
    ///           The \a transform_reduce algorithm returns the result of the
    ///           generalized sum over the values returned from \a conv_op when
    ///           applied to the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a transform_reduce and \a accumulate is
    /// that the behavior of transform_reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename Rng, typename T, typename Reduce, typename Convert>
    T transform_reduce(Rng&& rng, T init, Reduce&& red_op, Convert&& conv_op);

    ///////////////////////////////////////////////////////////////////////////
    /// Returns the result of accumulating init with the inner products of the
    /// pairs formed by the elements of two ranges starting at first1 and
    /// first2.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op2.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Iter2       The type of the second source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an forward iterator.
    /// \tparam T           The type of the value to be used as return)
    ///                     values (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the result will be calculated with.
    /// \param init         The initial value for the sum.
    ///
    /// The operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a hpx::future<T> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a T otherwise.
    ///
    template <typename ExPolicy, typename Rng, typename Iter2, typename T>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
    transform_reduce(ExPolicy&& policy, Rng&& rng, Iter2 first2, T init);

    ///////////////////////////////////////////////////////////////////////////
    /// Returns the result of accumulating init with the inner products of the
    /// pairs formed by the elements of two ranges starting at first1 and
    /// first2.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op2.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Iter2       The type of the second source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an forward iterator.
    /// \tparam T           The type of the value to be used as return)
    ///                     values (deduced).
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the result will be calculated with.
    /// \param init         The initial value for the sum.
    ///
    /// \returns  The \a transform_reduce algorithm returns \a T.
    ///
    template <typename Rng, typename Iter2, typename T>
    T transform_reduce(Rng&& rng, Iter2 first2, T init);

    ///////////////////////////////////////////////////////////////////////////
    /// Returns the result of accumulating init with the inner products of the
    /// pairs formed by the elements of two ranges starting at first1 and
    /// first2.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op2.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Iter2       The type of the second source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an forward iterator.
    /// \tparam T           The type of the value to be used as return)
    ///                     values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the multiplication operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the result will be calculated with.
    /// \param init         The initial value for the sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for the initial value and each
    ///                     of the return values of \a op2.
    ///                     This is a binary predicate. The
    ///                     signature of this predicate should be equivalent to
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The type \a Ret must be
    ///                     such that it can be implicitly converted to a type
    ///                     of \a T.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the input values
    ///                     of the sequence. This is a binary predicate. The
    ///                     signature of this predicate should be equivalent to
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The type \a Ret must be such that it can be
    ///                     implicitly converted to an object for the second
    ///                     argument type of \a op1.
    ///
    /// The operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a hpx::future<T>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a T otherwise.
    ///
    template <typename ExPolicy, typename Rng, typename Iter2,
        typename T, typename Reduce, typename Convert>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
    transform_reduce(ExPolicy&& policy, Rng&& rng, Iter2 first2, T init,
        Reduce&& red_op, Convert&& conv_op);

    ///////////////////////////////////////////////////////////////////////////
    /// Returns the result of accumulating init with the inner products of the
    /// pairs formed by the elements of two ranges starting at first1 and
    /// first2.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op2.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Iter2       The type of the second source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an forward iterator.
    /// \tparam T           The type of the value to be used as return)
    ///                     values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the multiplication operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the result will be calculated with.
    /// \param init         The initial value for the sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for the initial value and each
    ///                     of the return values of \a op2.
    ///                     This is a binary predicate. The
    ///                     signature of this predicate should be equivalent to
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The type \a Ret must be
    ///                     such that it can be implicitly converted to a type
    ///                     of \a T.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the input values
    ///                     of the sequence. This is a binary predicate. The
    ///                     signature of this predicate should be equivalent to
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The type \a Ret must be such that it can be
    ///                     implicitly converted to an object for the second
    ///                     argument type of \a op1.
    ///
    /// \returns  The \a transform_reduce algorithm returns \a T.
    ///
    template <typename Rng, typename Iter2, typename T, typename Reduce,
        typename Convert>
    T transform_reduce(Rng&& rng, Iter2 first2, T init, Reduce&& red_op,
        Convert&& conv_op);

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/transform_reduce.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::transform_reduce
    HPX_CXX_CORE_EXPORT inline constexpr struct transform_reduce_t final
      : hpx::detail::tag_dispatch<transform_reduce_t,
            hpx::detail::tag_parallel_algorithm<transform_reduce_t>>
    {
        template <typename ExPolicy, typename Iter, typename Sent, typename T,
            typename Reduce, typename Convert>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::sentinel_for<Sent, Iter> &&
                hpx::is_invocable_v<Convert,
                    typename std::iterator_traits<Iter>::value_type
                > &&
                hpx::is_invocable_v<Reduce,
                   typename hpx::util::invoke_result<Convert,
                       typename std::iterator_traits<Iter>::value_type
                   >::type,
                   typename hpx::util::invoke_result<Convert,
                       typename std::iterator_traits<Iter>::value_type
                   >::type
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
        invoke_default(ExPolicy&& policy, Iter first, Sent last, T init,
            Reduce red_op, Convert conv_op)
        {
            static_assert(std::forward_iterator<Iter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::transform_reduce<T>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(init),
                HPX_MOVE(red_op), HPX_MOVE(conv_op));
        }

        template <typename Iter, typename Sent, typename T, typename Reduce,
            typename Convert>
        // clang-format off
            requires(
                std::sentinel_for<Sent, Iter> &&
                hpx::is_invocable_v<Convert,
                   typename std::iterator_traits<Iter>::value_type
                > &&
                hpx::is_invocable_v<Reduce,
                   typename hpx::util::invoke_result<Convert,
                       typename std::iterator_traits<Iter>::value_type
                   >::type,
                   typename hpx::util::invoke_result<Convert,
                       typename std::iterator_traits<Iter>::value_type
                   >::type
                >
            )
        // clang-format on
        static T invoke_default(
            Iter first, Sent last, T init, Reduce red_op, Convert conv_op)
        {
            static_assert(
                std::input_iterator<Iter>, "Requires at least input iterator.");

            return hpx::parallel::detail::transform_reduce<T>().call(
                hpx::execution::seq, first, last, HPX_MOVE(init),
                HPX_MOVE(red_op), HPX_MOVE(conv_op));
        }

        template <typename ExPolicy, typename Iter, typename Sent,
            typename Iter2, typename T>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::sentinel_for<Sent, Iter> &&
                hpx::traits::is_iterator_v<Iter2>
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
        invoke_default(
            ExPolicy&& policy, Iter first, Sent last, Iter2 first2, T init)
        {
            static_assert(std::forward_iterator<Iter>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<Iter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, first2,
                HPX_MOVE(init), hpx::parallel::detail::plus(),
                hpx::parallel::detail::multiplies());
        }

        template <typename Iter, typename Sent, typename Iter2, typename T>
        // clang-format off
            requires(
                std::sentinel_for<Sent, Iter> &&
                hpx::traits::is_iterator_v<Iter2>
            )
        // clang-format on
        static T invoke_default(Iter first, Sent last, Iter2 first2, T init)
        {
            static_assert(
                std::input_iterator<Iter>, "Requires at least input iterator.");
            static_assert(std::input_iterator<Iter2>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                hpx::execution::seq, first, last, first2, HPX_MOVE(init),
                hpx::parallel::detail::plus(),
                hpx::parallel::detail::multiplies());
        }

        template <typename ExPolicy, typename Iter, typename Sent,
            typename Iter2, typename T, typename Reduce, typename Convert>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::sentinel_for<Sent, Iter> &&
                hpx::traits::is_iterator_v<Iter2> &&
                hpx::is_invocable_v<Convert,
                    typename std::iterator_traits<Iter>::value_type,
                    typename std::iterator_traits<Iter2>::value_type
                > &&
                hpx::is_invocable_v<Reduce,
                    typename hpx::util::invoke_result<Convert,
                        typename std::iterator_traits<Iter>::value_type,
                        typename std::iterator_traits<Iter2>::value_type
                    >::type,
                    typename hpx::util::invoke_result<Convert,
                        typename std::iterator_traits<Iter>::value_type,
                        typename std::iterator_traits<Iter2>::value_type
                    >::type
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
        invoke_default(ExPolicy&& policy, Iter first, Sent last, Iter2 first2,
            T init, Reduce red_op, Convert conv_op)
        {
            static_assert(std::forward_iterator<Iter>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<Iter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, first2,
                HPX_MOVE(init), HPX_MOVE(red_op), HPX_MOVE(conv_op));
        }

        template <typename Iter, typename Sent, typename Iter2, typename T,
            typename Reduce, typename Convert>
        // clang-format off
            requires(
                std::sentinel_for<Sent, Iter> &&
                hpx::traits::is_iterator_v<Iter2> &&
                hpx::is_invocable_v<Convert,
                    typename std::iterator_traits<Iter>::value_type,
                    typename std::iterator_traits<Iter2>::value_type
                > &&
                hpx::is_invocable_v<Reduce,
                    typename hpx::util::invoke_result<Convert,
                        typename std::iterator_traits<Iter>::value_type,
                        typename std::iterator_traits<Iter2>::value_type
                    >::type,
                    typename hpx::util::invoke_result<Convert,
                        typename std::iterator_traits<Iter>::value_type,
                        typename std::iterator_traits<Iter2>::value_type
                    >::type
                >
            )
        // clang-format on
        static T invoke_default(Iter first, Sent last, Iter2 first2, T init,
            Reduce red_op, Convert conv_op)
        {
            static_assert(
                std::input_iterator<Iter>, "Requires at least input iterator.");
            static_assert(std::input_iterator<Iter2>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                hpx::execution::seq, first, last, first2, HPX_MOVE(init),
                HPX_MOVE(red_op), HPX_MOVE(conv_op));
        }

        // range based versions

        template <typename ExPolicy, typename Rng, typename T, typename Reduce,
            typename Convert>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::is_invocable_v<Convert,
                    typename hpx::traits::range_traits<Rng>::value_type
                > &&
                hpx::is_invocable_v<Reduce,
                    typename hpx::util::invoke_result<Convert,
                        typename hpx::traits::range_traits<Rng>::value_type
                    >::type,
                    typename hpx::util::invoke_result<Convert,
                        typename hpx::traits::range_traits<Rng>::value_type
                    >::type
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
        invoke_default(ExPolicy&& policy, Rng&& rng, T init, Reduce red_op,
            Convert conv_op)
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::transform_reduce<T>().call(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::end(rng), HPX_MOVE(init), HPX_MOVE(red_op),
                HPX_MOVE(conv_op));
        }

        template <typename Rng, typename T, typename Reduce, typename Convert>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::is_invocable_v<Convert,
                    typename hpx::traits::range_traits<Rng>::value_type
                > &&
                hpx::is_invocable_v<Reduce,
                    typename hpx::util::invoke_result<Convert,
                        typename hpx::traits::range_traits<Rng>::value_type
                    >::type,
                    typename hpx::util::invoke_result<Convert,
                        typename hpx::traits::range_traits<Rng>::value_type
                    >::type
                >
            )
        // clang-format on
        static T invoke_default(
            Rng&& rng, T init, Reduce red_op, Convert conv_op)
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::input_iterator<iterator_type>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::transform_reduce<T>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                HPX_MOVE(init), HPX_MOVE(red_op), HPX_MOVE(conv_op));
        }

        template <typename ExPolicy, typename Rng, typename Iter2, typename T>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<Iter2>
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
        invoke_default(ExPolicy&& policy, Rng&& rng, Iter2 first2, T init)
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<Iter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::end(rng), first2, HPX_MOVE(init),
                hpx::parallel::detail::plus(),
                hpx::parallel::detail::multiplies());
        }

        template <typename Rng, typename Iter2, typename T>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<Iter2>
            )
        // clang-format on
        static T invoke_default(Rng&& rng, Iter2 first2, T init)
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::input_iterator<iterator_type>,
                "Requires at least input iterator.");
            static_assert(std::input_iterator<Iter2>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                first2, HPX_MOVE(init), hpx::parallel::detail::plus(),
                hpx::parallel::detail::multiplies());
        }

        template <typename ExPolicy, typename Rng, typename Iter2, typename T,
            typename Reduce, typename Convert>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<Iter2> &&
                hpx::is_invocable_v<Convert,
                    typename hpx::traits::range_traits<Rng>::value_type,
                    typename std::iterator_traits<Iter2>::value_type
                > &&
                hpx::is_invocable_v<Reduce,
                    typename hpx::util::invoke_result<Convert,
                        typename hpx::traits::range_traits<Rng>::value_type,
                        typename std::iterator_traits<Iter2>::value_type
                    >::type,
                    typename hpx::util::invoke_result<Convert,
                        typename hpx::traits::range_traits<Rng>::value_type,
                        typename std::iterator_traits<Iter2>::value_type
                    >::type
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
        invoke_default(ExPolicy&& policy, Rng&& rng, Iter2 first2, T init,
            Reduce red_op, Convert conv_op)
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<Iter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::end(rng), first2, HPX_MOVE(init), HPX_MOVE(red_op),
                HPX_MOVE(conv_op));
        }

        template <typename Rng, typename Iter2, typename T, typename Reduce,
            typename Convert>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<Iter2> &&
                hpx::is_invocable_v<Convert,
                    typename hpx::traits::range_traits<Rng>::value_type,
                    typename std::iterator_traits<Iter2>::value_type
                > &&
                hpx::is_invocable_v<Reduce,
                    typename hpx::util::invoke_result<Convert,
                        typename hpx::traits::range_traits<Rng>::value_type,
                        typename std::iterator_traits<Iter2>::value_type
                    >::type,
                    typename hpx::util::invoke_result<Convert,
                        typename hpx::traits::range_traits<Rng>::value_type,
                        typename std::iterator_traits<Iter2>::value_type
                    >::type
                >
            )
        // clang-format on
        static T invoke_default(
            Rng&& rng, Iter2 first2, T init, Reduce red_op, Convert conv_op)
        {
            using iterator_type = std::ranges::iterator_t<Rng>;

            static_assert(std::input_iterator<iterator_type>,
                "Requires at least input iterator.");
            static_assert(std::input_iterator<Iter2>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                first2, HPX_MOVE(init), HPX_MOVE(red_op), HPX_MOVE(conv_op));
        }
    } transform_reduce{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
