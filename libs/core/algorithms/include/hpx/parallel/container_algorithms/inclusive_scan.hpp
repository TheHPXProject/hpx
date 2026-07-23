//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/inclusive_scan.hpp
/// \page hpx::ranges::inclusive_scan
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Assigns through each iterator \a i in [result, result + (last - first))
    /// the value of
    /// GENERALIZED_NONCOMMUTATIVE_SUM(op, *first, ...,
    ///  *(first + (i - result))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Op          The type of the binary function object used for
    ///                     the reduction operation.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Specifies the function (or function object) which
    ///                     will be invoked for each of the values of the input
    ///                     sequence. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1 and \a Ret must be
    ///                     such that an object of a type as given by the input
    ///                     sequence can be implicitly converted to any
    ///                     of those types.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm
    /// invoked without an execution policy object will execute in sequential
    /// order in the calling thread.
    ///
    /// \returns  The \a inclusive_scan algorithm returns \a
    ///           util::in_out_result<InIter, OutIter>.
    ///           The \a inclusive_scan algorithm returns an input iterator to
    ///           the point denoted by the sentinel and an output iterator
    ///           to the element in the destination range, one past the last
    ///           element copied.
    ///
    /// \note   GENERALIZED_NONCOMMUTATIVE_SUM(+, a1, ..., aN) is defined as:
    ///         * a1 when N is 1
    ///         * GENERALIZED_NONCOMMUTATIVE_SUM(+, a1, ..., aK)
    ///           + GENERALIZED_NONCOMMUTATIVE_SUM(+, aM, ..., aN)
    ///           where 1 < K+1 = M <= N.
    ///
    /// The difference between \a exclusive_scan and \a inclusive_scan is that
    /// \a inclusive_scan includes the ith input element in the ith sum.
    ///
    template <typename InIter, typename Sent, typename OutIter,
        typename Op = std::plus<typename
        std::iterator_traits<InIter>::value_type>>
    inclusive_scan_result<InIter, OutIter>
    inclusive_scan(InIter first, Sent last, OutIter dest, Op&& op);

    ///////////////////////////////////////////////////////////////////////////
    /// Assigns through each iterator \a i in [result, result + (last - first))
    /// the value of
    /// GENERALIZED_NONCOMMUTATIVE_SUM(op, *first, ...,
    /// *(first + (i - result))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Op          The type of the binary function object used for
    ///                     the reduction operation.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Specifies the function (or function object) which
    ///                     will be invoked for each of the values of the input
    ///                     sequence. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1 and \a Ret must be
    ///                     such that an object of a type as given by the input
    ///                     sequence can be implicitly converted to any
    ///                     of those types.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a inclusive_scan algorithm returns a
    ///           \a hpx::future<util::in_out_result<FwdIter1, FwdIter2>> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a util::in_out_result<FwdIter1, FwdIter2> otherwise.
    ///           The \a inclusive_scan algorithm returns an input iterator to
    ///           the point denoted by the sentinel and an output iterator
    ///           to the element in the destination range, one past the last
    ///           element copied.
    ///
    /// \note   GENERALIZED_NONCOMMUTATIVE_SUM(+, a1, ..., aN) is defined as:
    ///         * a1 when N is 1
    ///         * GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aK)
    ///           + GENERALIZED_NONCOMMUTATIVE_SUM(+, aM, ..., aN)
    ///           where 1 < K+1 = M <= N.
    ///
    /// The difference between \a exclusive_scan and \a inclusive_scan is that
    /// \a inclusive_scan includes the ith input element in the ith sum.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Sent,
        typename FwdIter2, typename Op = std::plus<typename
        std::iterator_traits<FwdIter1>::value_type>>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        inclusive_scan_result<FwdIter1, FwdIter2>>::type
    inclusive_scan(ExPolicy&& policy, FwdIter1 first, Sent last,
        FwdIter2 dest, Op&& op);

    ///////////////////////////////////////////////////////////////////////////
    /// Assigns through each iterator \a i in [result, result + (last - first))
    /// the value of
    /// GENERALIZED_NONCOMMUTATIVE_SUM(op, *first, ...,
    ///  *(first + (i - result))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Op          The type of the binary function object used for
    ///                     the reduction operation.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Specifies the function (or function object) which
    ///                     will be invoked for each of the values of the input
    ///                     sequence. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1 and \a Ret must be
    ///                     such that an object of a type as given by the input
    ///                     sequence can be implicitly converted to any
    ///                     of those types.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm
    /// invoked without an execution policy object will execute in sequential
    /// order in the calling thread.
    ///
    /// \returns  The \a inclusive_scan algorithm returns
    ///           \a util::in_out_result<std::ranges::iterator_t<Rng>, O>
    ///           The \a inclusive_scan algorithm returns an input iterator to
    ///           the point denoted by the sentinel and an output iterator
    ///           to the element in the destination range, one past the last
    ///           element copied.
    ///
    /// \note   GENERALIZED_NONCOMMUTATIVE_SUM(+, a1, ..., aN) is defined as:
    ///         * a1 when N is 1
    ///         * GENERALIZED_NONCOMMUTATIVE_SUM(+, a1, ..., aK)
    ///           + GENERALIZED_NONCOMMUTATIVE_SUM(+, aM, ..., aN)
    ///           where 1 < K+1 = M <= N.
    ///
    /// The difference between \a exclusive_scan and \a inclusive_scan is that
    /// \a inclusive_scan includes the ith input element in the ith sum.
    ///
    template <typename Rng, typename O,
        typename Op = std::plus<typename hpx::traits::range_traits<Rng>::value_type>>
    inclusive_scan_result<std::ranges::iterator_t<Rng>, O>
    inclusive_scan(Rng&& rng, O dest, Op&& op);

    ///////////////////////////////////////////////////////////////////////////
    /// Assigns through each iterator \a i in [result, result + (last - first))
    /// the value of
    /// GENERALIZED_NONCOMMUTATIVE_SUM(op, *first, ...,
    /// *(first + (i - result))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op.
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
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Op          The type of the binary function object used for
    ///                     the reduction operation.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Specifies the function (or function object) which
    ///                     will be invoked for each of the values of the input
    ///                     sequence. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1 and \a Ret must be
    ///                     such that an object of a type as given by the input
    ///                     sequence can be implicitly converted to any
    ///                     of those types.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a inclusive_scan algorithm returns a
    ///           \a hpx::future<util::in_out_result
    ///           <std::ranges::iterator_t<Rng>, O>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a util::in_out_result
    ///           <std::ranges::iterator_t<Rng>, O>
    ///           otherwise.
    ///           The \a inclusive_scan algorithm returns an input iterator to
    ///           the point denoted by the sentinel and an output iterator
    ///           to the element in the destination range, one past the last
    ///           element copied
    ///
    /// \note   GENERALIZED_NONCOMMUTATIVE_SUM(+, a1, ..., aN) is defined as:
    ///         * a1 when N is 1
    ///         * GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aK)
    ///           + GENERALIZED_NONCOMMUTATIVE_SUM(+, aM, ..., aN)
    ///           where 1 < K+1 = M <= N.
    ///
    /// The difference between \a exclusive_scan and \a inclusive_scan is that
    /// \a inclusive_scan includes the ith input element in the ith sum.
    ///
    template <typename ExPolicy, typename Rng,  typename O,
        typename Op = std::plus<typename hpx::traits::range_traits<Rng>::value_type>>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        inclusive_scan_result<std::ranges::iterator_t<Rng>, O>>
    inclusive_scan(ExPolicy&& policy, Rng&& rng, O dest, Op&& op);

    ///////////////////////////////////////////////////////////////////////////
    /// Assigns through each iterator \a i in [result, result + (last - first))
    /// the value of
    /// GENERALIZED_NONCOMMUTATIVE_SUM(op, init, *first, ...,
    /// *(first + (i - result))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Op          The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Specifies the function (or function object) which
    ///                     will be invoked for each of the values of the input
    ///                     sequence. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1 and \a Ret must be
    ///                     such that an object of a type as given by the input
    ///                     sequence can be implicitly converted to any
    ///                     of those types.
    /// \param init         The initial value for the generalized sum.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm
    /// invoked without an execution policy object will execute in sequential
    /// order in the calling thread.
    ///
    /// \returns  The \a inclusive_scan algorithm returns \a
    ///           util::in_out_result<InIter, OutIter>.
    ///           The \a inclusive_scan algorithm returns an input iterator to
    ///           the point denoted by the sentinel and an output iterator
    ///           to the element in the destination range, one past the last
    ///           element copied.
    ///
    /// \note   GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aN) is defined as:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aK),
    ///           GENERALIZED_NONCOMMUTATIVE_SUM(op, aM, ..., aN))
    ///           where 1 < K+1 = M <= N.
    ///
    /// The difference between \a exclusive_scan and \a inclusive_scan is that
    /// \a inclusive_scan includes the ith input element in the ith sum. If
    /// \a op is not mathematically associative, the behavior of
    /// \a inclusive_scan may be non-deterministic.
    ///
    template <typename InIter, typename Sent, typename OutIter,
        typename Op,
        typename T = typename std::iterator_traits<InIter>::value_type>
    inclusive_scan_result<InIter, OutIter>
    inclusive_scan(InIter first, Sent last, OutIter dest, Op&& op, T init);

    ///////////////////////////////////////////////////////////////////////////
    /// Assigns through each iterator \a i in [result, result + (last - first))
    /// the value of
    /// GENERALIZED_NONCOMMUTATIVE_SUM(op, init, *first, ...,
    /// *(first + (i - result))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for InIter.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Op          The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Specifies the function (or function object) which
    ///                     will be invoked for each of the values of the input
    ///                     sequence. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1 and \a Ret must be
    ///                     such that an object of a type as given by the input
    ///                     sequence can be implicitly converted to any
    ///                     of those types.
    /// \param init         The initial value for the generalized sum.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a inclusive_scan algorithm returns a
    ///           \a hpx::future<util::in_out_result<InIter, OutIter>> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a util::in_out_result<InIter, OutIter> otherwise.
    ///           The \a inclusive_scan algorithm returns an input iterator to
    ///           the point denoted by the sentinel and an output iterator
    ///           to the element in the destination range, one past the last
    ///           element copied.
    ///
    /// \note   GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aN) is defined as:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aK),
    ///           GENERALIZED_NONCOMMUTATIVE_SUM(op, aM, ..., aN))
    ///           where 1 < K+1 = M <= N.
    ///
    /// The difference between \a exclusive_scan and \a inclusive_scan is that
    /// \a inclusive_scan includes the ith input element in the ith sum. If
    /// \a op is not mathematically associative, the behavior of
    /// \a inclusive_scan may be non-deterministic.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Sent,
        typename FwdIter2, typename Op,
        typename T = typename std::iterator_traits<FwdIter1>::value_type>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        inclusive_scan_result<FwdIter1, FwdIter2>>::type
    inclusive_scan(ExPolicy&& policy, InIter first, Sent last, OutIter dest,
        Op&& op, T init);

        ///////////////////////////////////////////////////////////////////////////
    /// Assigns through each iterator \a i in [result, result + (last - first))
    /// the value of
    /// GENERALIZED_NONCOMMUTATIVE_SUM(op, init, *first, ...,
    /// *(first + (i - result))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam O           The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Op          The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Specifies the function (or function object) which
    ///                     will be invoked for each of the values of the input
    ///                     sequence. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1 and \a Ret must be
    ///                     such that an object of a type as given by the input
    ///                     sequence can be implicitly converted to any
    ///                     of those types.
    /// \param init         The initial value for the generalized sum.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm
    /// invoked without an execution policy object will execute in sequential
    /// order in the calling thread.
    ///
    /// \returns  The \a inclusive_scan algorithm returns
    ///           \a util::in_out_result<std::ranges::iterator_t<Rng>, O>
    ///           The \a inclusive_scan algorithm returns an input iterator to
    ///           the point denoted by the sentinel and an output iterator
    ///           to the element in the destination range, one past the last
    ///           element copied.
    ///
    /// \note   GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aN) is defined as:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aK),
    ///           GENERALIZED_NONCOMMUTATIVE_SUM(op, aM, ..., aN))
    ///           where 1 < K+1 = M <= N.
    ///
    /// The difference between \a exclusive_scan and \a inclusive_scan is that
    /// \a inclusive_scan includes the ith input element in the ith sum. If
    /// \a op is not mathematically associative, the behavior of
    /// \a inclusive_scan may be non-deterministic.
    ///
    template <typename Rng, typename O,
        typename Op,
        typename T = typename std::iterator_traits<
            std::ranges::iterator_t<Rng>>::value_type>
    inclusive_scan_result<std::ranges::iterator_t<Rng>, O> inclusive_scan(
        Rng&& rng, O dest, Op&& op, T init);

    ///////////////////////////////////////////////////////////////////////////
    /// Assigns through each iterator \a i in [result, result + (last - first))
    /// the value of
    /// GENERALIZED_NONCOMMUTATIVE_SUM(op, init, *first, ...,
    /// *(first + (i - result))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a op.
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
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Op          The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param op           Specifies the function (or function object) which
    ///                     will be invoked for each of the values of the input
    ///                     sequence. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1 and \a Ret must be
    ///                     such that an object of a type as given by the input
    ///                     sequence can be implicitly converted to any
    ///                     of those types.
    /// \param init         The initial value for the generalized sum.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a inclusive_scan algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a inclusive_scan algorithm returns a
    ///           \a hpx::future<util::in_out_result
    ///           <std::ranges::iterator_t<Rng>, O>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a util::in_out_result
    ///           <std::ranges::iterator_t<Rng>, O>
    ///           otherwise.
    ///           The \a inclusive_scan algorithm returns an input iterator to
    ///           the point denoted by the sentinel and an output iterator
    ///           to the element in the destination range, one past the last
    ///           element copied
    ///
    /// \note   GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aN) is defined as:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aK),
    ///           GENERALIZED_NONCOMMUTATIVE_SUM(op, aM, ..., aN))
    ///           where 1 < K+1 = M <= N.
    ///
    /// The difference between \a exclusive_scan and \a inclusive_scan is that
    /// \a inclusive_scan includes the ith input element in the ith sum. If
    /// \a op is not mathematically associative, the behavior of
    /// \a inclusive_scan may be non-deterministic.
    ///
    template <typename ExPolicy, typename Rng,  typename O,
        typename Op,
        typename T = typename std::iterator_traits<
            std::ranges::iterator_t<Rng>>::value_type>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        inclusive_scan_result<std::ranges::iterator_t<Rng>, O>>
    inclusive_scan(ExPolicy&& policy, Rng&& rng, O dest, Op&& op, T init);
    // clang-format on
}}    // namespace hpx::ranges

#else

#include <hpx/config.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/inclusive_scan.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using inclusive_scan_result = parallel::util::in_out_result<I, O>;

    HPX_CXX_CORE_EXPORT inline constexpr struct inclusive_scan_t final
      : hpx::detail::tag_dispatch<inclusive_scan_t,
            hpx::detail::tag_parallel_algorithm<inclusive_scan_t>>
    {
        template <typename InIter, typename Sent, typename OutIter,
            typename Op = std::plus<>>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<InIter> &&
                std::sentinel_for<Sent, InIter> &&
                hpx::traits::is_iterator_v<OutIter> &&
                hpx::is_invocable_v<Op,
                    typename std::iterator_traits<InIter>::value_type,
                    typename std::iterator_traits<InIter>::value_type
                >
            )
        // clang-format on
        static inclusive_scan_result<InIter, OutIter> invoke_default(
            InIter first, Sent last, OutIter dest, Op op = Op())
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");
            static_assert(std::output_iterator<OutIter,
                              hpx::traits::iter_value_t<InIter>>,
                "Requires at least output iterator.");

            using result_type = inclusive_scan_result<InIter, OutIter>;

            return hpx::parallel::detail::inclusive_scan<result_type>().call(
                hpx::execution::seq, first, last, dest, HPX_MOVE(op));
        }

        template <typename ExPolicy, typename FwdIter1, typename Sent,
            typename FwdIter2, typename Op = std::plus<>>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent, FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Op,
                    typename std::iterator_traits<FwdIter1>::value_type,
                    typename std::iterator_traits<FwdIter1>::value_type
                >
            )
        // clang-format on
        static typename parallel::util::detail::algorithm_result<ExPolicy,
            inclusive_scan_result<FwdIter1, FwdIter2>>::type
        invoke_default(ExPolicy&& policy, FwdIter1 first, Sent last,
            FwdIter2 dest, Op op = Op())
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            using result_type = inclusive_scan_result<FwdIter1, FwdIter2>;

            return hpx::parallel::detail::inclusive_scan<result_type>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, dest, HPX_MOVE(op));
        }

        template <typename Rng, typename O, typename Op = std::plus<>>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::is_invocable_v<Op,
                    typename hpx::traits::range_traits<Rng>::value_type,
                    typename hpx::traits::range_traits<Rng>::value_type
                >
            )
        // clang-format on
        static inclusive_scan_result<std::ranges::iterator_t<Rng>, O>
        invoke_default(Rng&& rng, O dest, Op op = Op())
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::input_iterator<iterator_type>,
                "Requires at least input iterator.");

            using result_type =
                inclusive_scan_result<std::ranges::iterator_t<Rng>, O>;

            return hpx::parallel::detail::inclusive_scan<result_type>().call(
                hpx::execution::seq, std::begin(rng), std::end(rng), dest,
                HPX_FORWARD(Op, op));
        }

        template <typename ExPolicy, typename Rng, typename O,
            typename Op = std::plus<>>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::is_invocable_v<Op,
                    typename hpx::traits::range_traits<Rng>::value_type,
                    typename hpx::traits::range_traits<Rng>::value_type
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            inclusive_scan_result<std::ranges::iterator_t<Rng>, O>>
        invoke_default(ExPolicy&& policy, Rng&& rng, O dest, Op op = Op())
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            using result_type =
                inclusive_scan_result<std::ranges::iterator_t<Rng>, O>;

            return hpx::parallel::detail::inclusive_scan<result_type>().call(
                HPX_FORWARD(ExPolicy, policy), std::begin(rng), std::end(rng),
                dest, HPX_MOVE(op));
        }

        template <typename InIter, typename Sent, typename OutIter, typename Op,
            typename T = typename std::iterator_traits<InIter>::value_type>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<InIter> &&
                std::sentinel_for<Sent, InIter> &&
                hpx::traits::is_iterator_v<OutIter> &&
                hpx::is_invocable_v<Op,
                    typename std::iterator_traits<InIter>::value_type,
                    typename std::iterator_traits<InIter>::value_type
                >
            )
        // clang-format on
        static inclusive_scan_result<InIter, OutIter> invoke_default(
            InIter first, Sent last, OutIter dest, Op op, T init)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");
            static_assert(std::output_iterator<OutIter,
                              hpx::traits::iter_value_t<InIter>>,
                "Requires at least output iterator.");

            using result_type = inclusive_scan_result<InIter, OutIter>;

            return hpx::parallel::detail::inclusive_scan<result_type>().call(
                hpx::execution::seq, first, last, dest, HPX_MOVE(init),
                HPX_MOVE(op));
        }

        template <typename ExPolicy, typename FwdIter1, typename Sent,
            typename FwdIter2, typename Op,
            typename T = typename std::iterator_traits<FwdIter1>::value_type>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent, FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Op,
                    typename std::iterator_traits<FwdIter1>::value_type,
                    typename std::iterator_traits<FwdIter1>::value_type
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            inclusive_scan_result<FwdIter1, FwdIter2>>
        invoke_default(ExPolicy&& policy, FwdIter1 first, Sent last,
            FwdIter2 dest, Op op, T init)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            using result_type = inclusive_scan_result<FwdIter1, FwdIter2>;

            return hpx::parallel::detail::inclusive_scan<result_type>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, dest,
                HPX_MOVE(init), HPX_MOVE(op));
        }

        template <typename Rng, typename O, typename Op,
            typename T = typename std::iterator_traits<
                std::ranges::iterator_t<Rng>>::value_type>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::is_invocable_v<Op,
                    typename hpx::traits::range_traits<Rng>::value_type,
                    typename hpx::traits::range_traits<Rng>::value_type
                >
            )
        // clang-format on
        static inclusive_scan_result<std::ranges::iterator_t<Rng>, O>
        invoke_default(Rng&& rng, O dest, Op op, T init)
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::input_iterator<iterator_type>,
                "Requires at least input iterator.");

            using result_type =
                inclusive_scan_result<std::ranges::iterator_t<Rng>, O>;

            return hpx::parallel::detail::inclusive_scan<result_type>().call(
                hpx::execution::seq, std::begin(rng), std::end(rng), dest,
                HPX_MOVE(init), HPX_MOVE(op));
        }

        template <typename ExPolicy, typename Rng, typename O, typename Op,
            typename T = typename std::iterator_traits<
                std::ranges::iterator_t<Rng>>::value_type>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::is_invocable_v<Op,
                    typename hpx::traits::range_traits<Rng>::value_type,
                    typename hpx::traits::range_traits<Rng>::value_type
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            inclusive_scan_result<std::ranges::iterator_t<Rng>, O>>
        invoke_default(ExPolicy&& policy, Rng&& rng, O dest, Op op, T init)
        {
            using iterator_type =
                typename hpx::traits::range_traits<Rng>::iterator_type;

            static_assert(std::forward_iterator<iterator_type>,
                "Requires at least forward iterator.");

            using result_type =
                inclusive_scan_result<std::ranges::iterator_t<Rng>, O>;

            return hpx::parallel::detail::inclusive_scan<result_type>().call(
                HPX_FORWARD(ExPolicy, policy), std::begin(rng), std::end(rng),
                dest, HPX_MOVE(init), HPX_MOVE(op));
        }
    } inclusive_scan{};
}    // namespace hpx::ranges

#endif
