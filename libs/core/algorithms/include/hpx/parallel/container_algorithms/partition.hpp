//  Copyright (c) 2017 Taeguk Kwon
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/partition.hpp
/// \page hpx::ranges::partition, hpx::ranges::stable_partition, hpx::ranges::partition_copy
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Reorders the elements in the range \a rng in such a way that
    /// all elements for which the predicate \a pred returns true precede
    /// the elements for which the predicate \a pred returns false.
    /// Relative order of the elements is not preserved.
    ///
    /// \note   Complexity: Performs at most 2 * N swaps,
    ///         exactly N applications of the predicate and projection,
    ///         where N = std::distance(begin(rng), end(rng)).
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a partition requires \a Pred to meet
    ///                     the requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by the range \a rng. This is an
    ///                     unary predicate for partitioning the source
    ///                     iterators. The signature of this predicate should
    ///                     be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a partition algorithm invoked without
    /// an execution policy object execute in sequential order in the calling
    /// thread.
    ///
    /// \returns  The \a partition algorithm returns
    ///           \a subrange_t<std::ranges::iterator_t<Rng>>
    ///           The \a partition algorithm returns a subrange starting with
    ///           an iterator to the first element of the second group and
    ///           finishing with an iterator equal to last.
    ///
    template <typename Rng, typename Pred,
        typename Proj = hpx::identity>
    subrange_t<std::ranges::iterator_t<Rng>>
    partition(Rng&& rng, Pred&& pred, Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Reorders the elements in the range [first, last) in such a way that
    /// all elements for which the predicate \a pred returns true precede
    /// the elements for which the predicate \a pred returns false.
    /// Relative order of the elements is not preserved.
    ///
    /// \note   Complexity: At most 2 * (last - first) swaps.
    ///         Exactly \a last - \a first applications of the predicate and
    ///         projection.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a partition requires \a Pred to meet
    ///                     the requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is an
    ///                     unary predicate for partitioning the source
    ///                     iterators. The signature of
    ///                     this predicate should be equivalent to:
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
    /// The assignments in the parallel \a partition algorithm invoked without
    /// an execution policy object execute in sequential order in the calling
    /// thread.
    ///
    /// \returns  The \a partition algorithm returns returns \a
    ///           subrange_t<FwdIter>.
    ///           The \a partition algorithm returns a subrange starting with
    ///           an iterator to the first element of the second group and
    ///           finishing with an iterator equal to last.
    ///
    template <typename FwdIter, typename Sent, typename Pred,
        typename Proj = hpx::identity>
    subrange_t<FwdIter> partition(FwdIter first, Sent last, Pred&& pred,
        Proj&& proj = Proj());

     ///////////////////////////////////////////////////////////////////////////
    /// Permutes the elements in the range [first, last) such that there exists
    /// an iterator i such that for every iterator j in the range [first, i)
    /// INVOKE(f, INVOKE (proj, *j)) != false, and for every iterator k in the
    /// range [i, last), INVOKE(f, INVOKE (proj, *k)) == false
    ///
    /// \note   Complexity: At most (last - first) * log(last - first) swaps,
    ///         but only linear number of swaps if there is enough extra memory
    ///         Exactly \a last - \a first applications of the predicate and
    ///         projection.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an birdirectional iterator
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a partition requires \a Pred to meet
    ///                     the requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param pred         Unary predicate which returns true if the element
    ///                     should be ordered before other elements.
    ///                     Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). The signature
    ///                     of this predicate should be equivalent to:
    ///                     \code
    ///                     bool fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The type \a Type must be such that an object of
    ///                     type \a BidirIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a f is invoked.
    ///
    /// The invocations of \a f in the parallel \a stable_partition algorithm
    /// invoked without an execution policy object executes in sequential order
    /// in the calling thread.
    ///
    /// \returns  The \a stable_partition algorithm returns an iterator i such
    ///           that for every iterator j in the range [first, i), f(*j) !=
    ///           false INVOKE(f, INVOKE(proj, *j)) != false, and for every
    ///           iterator k in the range [i, last), f(*k) == false
    ///           INVOKE(f, INVOKE (proj, *k)) == false. The relative order of
    ///           the elements in both groups is preserved.
    ///
    template <typename Rng, typename Pred,
        typename Proj = hpx::identity>
    subrange_t<std::ranges::iterator_t<Rng>> stable_partition(Rng&& rng,
        Pred&& pred, Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Permutes the elements in the range [first, last) such that there exists
    /// an iterator i such that for every iterator j in the range [first, i)
    /// INVOKE(f, INVOKE (proj, *j)) != false, and for every iterator k in the
    /// range [i, last), INVOKE(f, INVOKE (proj, *k)) == false
    ///
    /// \note   Complexity: At most (last - first) * log(last - first) swaps,
    ///         but only linear number of swaps if there is enough extra memory
    ///         Exactly \a last - \a first applications of the predicate and
    ///         projection.
    ///
    /// \tparam BidirIter   The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for BidirIter.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a partition requires \a Pred to meet
    ///                     the requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param pred         Unary predicate which returns true if the element
    ///                     should be ordered before other elements.
    ///                     Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). The signature
    ///                     of this predicate should be equivalent to:
    ///                     \code
    ///                     bool fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The type \a Type must be such that an object of
    ///                     type \a BidirIter can be dereferenced and then
    ///                     implicitly converted to \a Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a f is invoked.
    ///
    /// The invocations of \a f in the parallel \a stable_partition algorithm
    /// invoked without an execution policy object executes in sequential order
    /// in the calling thread.
    ///
    /// \returns  The \a stable_partition algorithm returns an iterator i such
    ///           that for every iterator j in the range [first, i), f(*j) !=
    ///           false INVOKE(f, INVOKE(proj, *j)) != false, and for every
    ///           iterator k in the range [i, last), f(*k) == false
    ///           INVOKE(f, INVOKE (proj, *k)) == false. The relative order of
    ///           the elements in both groups is preserved.
    ///
    template <typename BidirIter, typename Sent, typename Pred,
        typename Proj = hpx::identity>
    subrange_t<BidirIter> stable_partition(BidirIter first, Sent last,
        Pred&& pred, Proj&& proj = Proj());

///////////////////////////////////////////////////////////////////////////
    /// Copies the elements in the range \a rng,
    /// to two different ranges depending on the value returned by
    /// the predicate \a pred. The elements, that satisfy the predicate \a pred
    /// are copied to the range beginning at \a dest_true. The rest of
    /// the elements are copied to the range beginning at \a dest_false.
    /// The order of the elements is preserved.
    ///
    /// \note   Complexity: Performs not more than N assignments,
    ///         exactly N applications of the predicate \a pred,
    ///         where N = std::distance(begin(rng), end(rng)).
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam OutIter2    The type of the iterator representing the
    ///                     destination range for the elements that satisfy
    ///                     the predicate \a pred (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam OutIter3    The type of the iterator representing the
    ///                     destination range for the elements that don't
    ///                     satisfy the predicate \a pred (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a partition_copy requires \a Pred to
    ///                     meet the requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest_true    Refers to the beginning of the destination range
    ///                     for the elements that satisfy the predicate \a pred
    /// \param dest_false   Refers to the beginning of the destination range
    ///                     for the elements that don't satisfy the predicate
    ///                     \a pred.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is an
    ///                     unary predicate for partitioning the source
    ///                     iterators. The signature of
    ///                     this predicate should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a partition_copy algorithm invoked
    /// without an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a partition_copy algorithm returns a
    ///           partition_copy_result<std::ranges::iterator_t<Rng>,
    ///           FwdIter2, FwdIter3>>.
    ///           The \a partition_copy algorithm returns the tuple of
    ///           the source iterator \a last,
    ///           the destination iterator to the end of the \a dest_true
    ///           range, and the destination iterator to the end of the \a
    ///           dest_false range.
    ///
    template <typename Rng, typename OutIter2,
        typename OutIter3, typename Pred,
        typename Proj = hpx::identity>
    partition_copy_result<std::ranges::iterator_t<Rng>,
        OutIter2, OutIter3>
    partition_copy(Rng&& rng, OutIter2 dest_true, OutIter3 dest_false,
        Pred&& pred, Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Copies the elements in the range \a rng,
    /// to two different ranges depending on the value returned by
    /// the predicate \a pred. The elements, that satisfy the predicate \a pred
    /// are copied to the range beginning at \a dest_true. The rest of
    /// the elements are copied to the range beginning at \a dest_false.
    /// The order of the elements is preserved.
    ///
    /// \note   Complexity: Performs not more than N assignments,
    ///         exactly N applications of the predicate \a pred,
    ///         where N = std::distance(begin(rng), end(rng)).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range for the elements that satisfy
    ///                     the predicate \a pred (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter3    The type of the iterator representing the
    ///                     destination range for the elements that don't
    ///                     satisfy the predicate \a pred (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a partition_copy requires \a Pred to
    ///                     meet the requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest_true    Refers to the beginning of the destination range
    ///                     for the elements that satisfy the predicate \a pred
    /// \param dest_false   Refers to the beginning of the destination range
    ///                     for the elements that don't satisfy the predicate
    ///                     \a pred.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is an
    ///                     unary predicate for partitioning the source
    ///                     iterators. The signature of
    ///                     this predicate should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a partition_copy algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a partition_copy algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a partition_copy algorithm returns a
    ///           \a hpx::future<partition_copy_result
    ///           <std::ranges::iterator_t<Rng>,
    ///           FwdIter2, FwdIter3>>
    ///           if the execution policy is of type \a parallel_task_policy
    ///           and returns
    ///           partition_copy_result<std::ranges::iterator_t<Rng>,
    ///           FwdIter2, FwdIter3>
    ///           otherwise.
    ///           The \a partition_copy algorithm returns the tuple of
    ///           the source iterator \a last,
    ///           the destination iterator to the end of the \a dest_true
    ///           range, and the destination iterator to the end of the \a
    ///           dest_false range.
    ///
    template <typename ExPolicy, typename Rng, typename FwdIter2,
        typename FwdIter3, typename Pred,
        typename Proj = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        partition_copy_result<std::ranges::iterator_t<Rng>, FwdIter2,
            FwdIter3>>::type
    partition_copy(ExPolicy&& policy, Rng&& rng, FwdIter2 dest_true,
        FwdIter3 dest_false, Pred&& pred, Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Copies the elements in the range, defined by [first, last),
    /// to two different ranges depending on the value returned by
    /// the predicate \a pred. The elements, that satisfy the predicate \a pred
    /// are copied to the range beginning at \a dest_true. The rest of
    /// the elements are copied to the range beginning at \a dest_false.
    /// The order of the elements is preserved.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a f.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam OutIter2    The type of the iterator representing the
    ///                     destination range for the elements that satisfy
    ///                     the predicate \a pred (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam OutIter3    The type of the iterator representing the
    ///                     destination range for the elements that don't
    ///                     satisfy the predicate \a pred (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a partition_copy requires \a Pred to
    ///                     meet the requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param dest_true    Refers to the beginning of the destination range
    ///                     for the elements that satisfy the predicate \a pred
    /// \param dest_false   Refers to the beginning of the destination range
    ///                     for the elements that don't satisfy the predicate
    ///                     \a pred.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is an
    ///                     unary predicate for partitioning the source
    ///                     iterators. The signature of
    ///                     this predicate should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a partition_copy algorithm invoked
    /// without an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a partition_copy algorithm returns a
    ///           \a partition_copy_result<FwdIter, OutIter2, OutIter3>.
    ///           The \a partition_copy algorithm returns the tuple of
    ///           the source iterator \a last,
    ///           the destination iterator to the end of the \a
    ///           dest_true range, and the destination iterator to the end of
    ///           the \a dest_false range.
    ///
    template <typename InIter, typename Sent, typename OutIter2,
        typename OutIter3, typename Pred,
        typename Proj = hpx::identity>
    partition_copy_result<InIter, OutIter2, OutIter3>
    partition_copy(InIter first,
        Sent last, OutIter2 dest_true, OutIter3 dest_false, Pred&& pred,
        Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Copies the elements in the range, defined by [first, last),
    /// to two different ranges depending on the value returned by
    /// the predicate \a pred. The elements, that satisfy the predicate \a pred
    /// are copied to the range beginning at \a dest_true. The rest of
    /// the elements are copied to the range beginning at \a dest_false.
    /// The order of the elements is preserved.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a f.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam OutIter2    The type of the iterator representing the
    ///                     destination range for the elements that satisfy
    ///                     the predicate \a pred (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam OutIter3    The type of the iterator representing the
    ///                     destination range for the elements that don't
    ///                     satisfy the predicate \a pred (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a partition_copy requires \a Pred to
    ///                     meet the requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to sentinel value denoting the end of the
    ///                     sequence of elements the algorithm will be applied.
    /// \param dest_true    Refers to the beginning of the destination range
    ///                     for the elements that satisfy the predicate \a pred
    /// \param dest_false   Refers to the beginning of the destination range
    ///                     for the elements that don't satisfy the predicate
    ///                     \a pred.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is an
    ///                     unary predicate for partitioning the source
    ///                     iterators. The signature of
    ///                     this predicate should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a partition_copy algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a partition_copy algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a partition_copy algorithm returns a
    ///           hpx::future<partition_copy_result<FwdIter, OutIter2,
    ///           OutIter3>>
    ///           if the execution policy is of type \a parallel_task_policy
    ///           and returns
    ///           \a partition_copy_result<FwdIter, OutIter2, OutIter3>
    ///           otherwise.
    ///           The \a partition_copy algorithm returns the tuple of
    ///           the source iterator \a last,
    ///           the destination iterator to the end of the \a
    ///           dest_true range, and the destination iterator to the end of
    ///           the \a dest_false range.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent,
        typename OutIter2, typename OutIter3, typename Pred,
        typename Proj = hpx::identity>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        partition_copy_result<FwdIter, OutIter2, OutIter3>>::type
    partition_copy(ExPolicy&& policy,
        FwdIter first, Sent last, OutIter2 dest_true, OutIter3 dest_false,
        Pred&& pred, Proj&& proj = Proj());

    // clang-format on

    /// \brief Execution-policy overload of \c partition.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        typename Pred, typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::permutable<std::ranges::iterator_t<R>> &&
        std::indirect_unary_predicate<Pred,
            std::projected<std::ranges::iterator_t<R>, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R>>
    partition(ExPolicy&& policy, R&& rng, Pred pred, Proj proj = {});

    /// \brief Execution-policy overload of \c partition.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename Pred,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> && std::permutable<I> &&
        std::indirect_unary_predicate<Pred, std::projected<I, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I>>
    partition(ExPolicy&& policy, I first, S last, Pred pred, Proj proj = {});

    /// \brief Execution-policy overload of \c stable_partition.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        typename Pred, typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::permutable<std::ranges::iterator_t<R>> &&
        std::indirect_unary_predicate<Pred,
            std::projected<std::ranges::iterator_t<R>, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R>>
    stable_partition(ExPolicy&& policy, R&& rng, Pred pred, Proj proj = {});

    /// \brief Execution-policy overload of \c stable_partition.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename Pred,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> && std::permutable<I> &&
        std::indirect_unary_predicate<Pred, std::projected<I, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I>>
    stable_partition(
        ExPolicy&& policy, I first, S last, Pred pred, Proj proj = {});

    /// \brief Execution-policy overload of \c partition_copy.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// The operation is bounded by the supplied output range(s). Returned
    /// input positions identify where processing can resume.
    /// \returns The algorithm result, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, std::random_access_iterator O1,
        std::sized_sentinel_for<O1> S1, std::random_access_iterator O2,
        std::sized_sentinel_for<O2> S2, typename Pred,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_copyable<I, O1> && std::indirectly_copyable<I, O2> &&
        std::indirect_unary_predicate<Pred, std::projected<I, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        partition_copy_result<I, O1, O2>>
    partition_copy(ExPolicy&& policy, I first, S last, O1 yes, S1 yes_last,
        O2 no, S2 no_last, Pred pred, Proj proj = {});

    /// \brief Execution-policy overload of \c partition_copy.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The algorithm result, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        std::ranges::random_access_range R1,
        std::ranges::random_access_range R2, typename Pred,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> && std::ranges::sized_range<R1> &&
        std::ranges::sized_range<R2> &&
        std::indirectly_copyable<std::ranges::iterator_t<R>,
            std::ranges::iterator_t<R1>> &&
        std::indirectly_copyable<std::ranges::iterator_t<R>,
            std::ranges::iterator_t<R2>> &&
        std::indirect_unary_predicate<Pred,
            std::projected<std::ranges::iterator_t<R>, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        partition_copy_result<std::ranges::borrowed_iterator_t<R>,
            std::ranges::borrowed_iterator_t<R1>,
            std::ranges::borrowed_iterator_t<R2>>>
    partition_copy(ExPolicy&& policy, R&& rng, R1&& output_yes, R2&& output_no,
        Pred pred, Proj proj = {});
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/bounded_copy.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/partition.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT template <typename I, typename O1, typename O2>
    using partition_copy_result = parallel::util::in_out_out_result<I, O1, O2>;

    HPX_CXX_CORE_EXPORT inline constexpr struct partition_t final
      : hpx::detail::tag_dispatch<partition_t,
            hpx::detail::tag_parallel_algorithm<partition_t>>
    {
        template <typename Rng, typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                parallel::traits::is_projected_range_v<Proj, Rng> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static subrange_t<std::ranges::iterator_t<Rng>,
            std::ranges::sentinel_t<Rng>>
        invoke_default(Rng&& rng, Pred pred, Proj proj = Proj())
        {
            using iterator = std::ranges::iterator_t<Rng>;

            static_assert(std::forward_iterator<iterator>,
                "Requires at least forward iterator.");

            return hpx::parallel::util::make_subrange<
                std::ranges::iterator_t<Rng>, std::ranges::sentinel_t<Rng>>(
                hpx::parallel::detail::partition<iterator>().call(
                    hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), HPX_MOVE(pred), HPX_MOVE(proj)),
                hpx::util::end(rng));
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Pred, typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::permutable<std::ranges::iterator_t<R>> &&
            std::indirect_unary_predicate<Pred,
                std::projected<std::ranges::iterator_t<R>, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, Pred pred, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                [](std::ranges::subrange<I> result)
                    -> std::ranges::borrowed_subrange_t<R> { return result; });
        }

        template <typename FwdIter, typename Sent, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        static subrange_t<FwdIter> invoke_default(
            FwdIter first, Sent last, Pred pred, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::util::make_subrange<FwdIter, FwdIter>(
                hpx::parallel::detail::partition<FwdIter>().call(
                    hpx::execution::seq, first, last, HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                parallel::detail::advance_to_sentinel(first, last));
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Pred,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::permutable<I> &&
            std::indirect_unary_predicate<Pred, std::projected<I, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, Pred pred, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::util::detail::convert_to_result(
                parallel::detail::partition<I>().call(
                    HPX_FORWARD(ExPolicy, policy), first, end, HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                [end](I position) -> std::ranges::subrange<I> {
                    return {position, end};
                });
        }
    } partition{};

    HPX_CXX_CORE_EXPORT inline constexpr struct stable_partition_t final
      : hpx::detail::tag_dispatch<stable_partition_t,
            hpx::detail::tag_parallel_algorithm<stable_partition_t>>
    {
        template <typename Rng, typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                parallel::traits::is_projected_range_v<Proj, Rng> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static subrange_t<std::ranges::iterator_t<Rng>,
            std::ranges::sentinel_t<Rng>>
        invoke_default(Rng&& rng, Pred pred, Proj proj = Proj())
        {
            using iterator = std::ranges::iterator_t<Rng>;

            static_assert(std::bidirectional_iterator<iterator>,
                "Requires at least bidirectional iterator.");

            return hpx::parallel::util::make_subrange<
                std::ranges::iterator_t<Rng>, std::ranges::sentinel_t<Rng>>(
                hpx::parallel::detail::stable_partition<iterator>().call2(
                    hpx::execution::seq, std::true_type{},
                    hpx::util::begin(rng), hpx::util::end(rng), HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                hpx::util::end(rng));
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Pred, typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::permutable<std::ranges::iterator_t<R>> &&
            std::indirect_unary_predicate<Pred,
                std::projected<std::ranges::iterator_t<R>, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, Pred pred, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                [](std::ranges::subrange<I> result)
                    -> std::ranges::borrowed_subrange_t<R> { return result; });
        }

        template <typename BidirIter, typename Sent, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<BidirIter> &&
                std::sentinel_for<Sent, BidirIter> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    parallel::traits::projected<Proj, BidirIter>
                >
            )
        // clang-format on
        static subrange_t<BidirIter> invoke_default(
            BidirIter first, Sent last, Pred pred, Proj proj = Proj())
        {
            static_assert(std::bidirectional_iterator<BidirIter>,
                "Requires at least bidirectional iterator.");

            return hpx::parallel::util::make_subrange<BidirIter, BidirIter>(
                hpx::parallel::detail::stable_partition<BidirIter>().call2(
                    hpx::execution::seq, std::true_type{}, first, last,
                    HPX_MOVE(pred), HPX_MOVE(proj)),
                parallel::detail::advance_to_sentinel(first, last));
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Pred,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::permutable<I> &&
            std::indirect_unary_predicate<Pred, std::projected<I, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, Pred pred, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::util::detail::convert_to_result(
                parallel::detail::stable_partition<I>().call(
                    HPX_FORWARD(ExPolicy, policy), first, end, HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                [end](I position) -> std::ranges::subrange<I> {
                    return {position, end};
                });
        }
    } stable_partition{};

    HPX_CXX_CORE_EXPORT inline constexpr struct partition_copy_t final
      : hpx::detail::tag_dispatch<partition_copy_t,
            hpx::detail::tag_parallel_algorithm<partition_copy_t>>
    {
        template <typename Rng, typename OutIter2, typename OutIter3,
            typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<OutIter2> &&
                hpx::traits::is_iterator_v<OutIter3> &&
                parallel::traits::is_projected_range_v<Proj, Rng> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static partition_copy_result<std::ranges::iterator_t<Rng>, OutIter2,
            OutIter3>
        invoke_default(Rng&& rng, OutIter2 dest_true, OutIter3 dest_false,
            Pred pred, Proj proj = Proj())
        {
            using iterator = std::ranges::iterator_t<Rng>;
            using result_type = hpx::tuple<iterator, OutIter2, OutIter3>;

            static_assert(std::input_iterator<iterator>,
                "Requires at least input iterator.");

            return parallel::util::make_in_out_out_result(
                parallel::detail::partition_copy<result_type>().call(
                    hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), dest_true, dest_false, HPX_MOVE(pred),
                    HPX_MOVE(proj)));
        }

        template <typename ExPolicy, typename Rng, typename FwdIter2,
            typename FwdIter3, typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::traits::is_iterator_v<FwdIter3> &&
                parallel::traits::is_projected_range_v<Proj, Rng> &&
                parallel::traits::is_indirect_callable_v<ExPolicy, Pred,
                    parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            partition_copy_result<std::ranges::iterator_t<Rng>, FwdIter2,
                FwdIter3>>
        invoke_default(ExPolicy&& policy, Rng&& rng, FwdIter2 dest_true,
            FwdIter3 dest_false, Pred pred, Proj proj = Proj())
        {
            using iterator = std::ranges::iterator_t<Rng>;
            using result_type = hpx::tuple<iterator, FwdIter2, FwdIter3>;

            static_assert(std::forward_iterator<iterator>,
                "Requires at least forward iterator.");

            return parallel::util::make_in_out_out_result(
                parallel::detail::partition_copy<result_type>().call(
                    HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                    hpx::util::end(rng), dest_true, dest_false, HPX_MOVE(pred),
                    HPX_MOVE(proj)));
        }

        template <typename InIter, typename Sent, typename OutIter2,
            typename OutIter3, typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<InIter> &&
                std::sentinel_for<Sent, InIter> &&
                hpx::traits::is_iterator_v<OutIter2> &&
                hpx::traits::is_iterator_v<OutIter3> &&
                parallel::traits::is_projected_v<Proj, InIter> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    parallel::traits::projected<Proj, InIter>
                >
            )
        // clang-format on
        static partition_copy_result<InIter, OutIter2, OutIter3> invoke_default(
            InIter first, Sent last, OutIter2 dest_true, OutIter3 dest_false,
            Pred pred, Proj proj = Proj())
        {
            using result_type = hpx::tuple<InIter, OutIter2, OutIter3>;

            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            return parallel::util::make_in_out_out_result(
                parallel::detail::partition_copy<result_type>().call(
                    hpx::execution::seq, first, last, dest_true, dest_false,
                    HPX_MOVE(pred), HPX_MOVE(proj)));
        }

        template <typename ExPolicy, typename FwdIter, typename Sent,
            typename OutIter2, typename OutIter3, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                hpx::traits::is_iterator_v<OutIter2> &&
                hpx::traits::is_iterator_v<OutIter3> &&
                parallel::traits::is_projected_v<Proj, FwdIter> &&
                parallel::traits::is_indirect_callable_v<
                    ExPolicy, Pred,
                    parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            partition_copy_result<FwdIter, OutIter2, OutIter3>>
        invoke_default(ExPolicy&& policy, FwdIter first, Sent last,
            OutIter2 dest_true, OutIter3 dest_false, Pred pred,
            Proj proj = Proj())
        {
            using result_type = hpx::tuple<FwdIter, OutIter2, OutIter3>;

            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return parallel::util::make_in_out_out_result(
                parallel::detail::partition_copy<result_type>().call(
                    HPX_FORWARD(ExPolicy, policy), first, last, dest_true,
                    dest_false, HPX_MOVE(pred), HPX_MOVE(proj)));
        }

        /// \brief Partition into two bounded output ranges.
        /// \returns The first input that does not fit its selected output,
        /// and both output positions, wrapped in a future for task policies.
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, std::random_access_iterator O1,
            std::sized_sentinel_for<O1> S1, std::random_access_iterator O2,
            std::sized_sentinel_for<O2> S2, typename Pred,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_copyable<I, O1> &&
            std::indirectly_copyable<I, O2> &&
            std::indirect_unary_predicate<Pred, std::projected<I, Proj>>
        static decltype(auto) invoke_default(ExPolicy&& policy, I first, S last,
            O1 yes, S1 yes_last, O2 no, S2 no_last, Pred pred, Proj proj = {})
        {
            return parallel::detail::bounded_partition_copy<I, O1, O2>().call(
                HPX_FORWARD(ExPolicy, policy), first, first + (last - first),
                yes, yes + (yes_last - yes), no, no + (no_last - no),
                HPX_MOVE(pred), HPX_MOVE(proj));
        }

        /// \brief Partition a range into two bounded output ranges.
        /// \returns Borrowed input and output positions, or their future.
        template <typename ExPolicy, std::ranges::random_access_range R,
            std::ranges::random_access_range R1,
            std::ranges::random_access_range R2, typename Pred,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> && std::ranges::sized_range<R1> &&
            std::ranges::sized_range<R2> &&
            std::indirectly_copyable<std::ranges::iterator_t<R>,
                std::ranges::iterator_t<R1>> &&
            std::indirectly_copyable<std::ranges::iterator_t<R>,
                std::ranges::iterator_t<R2>> &&
            std::indirect_unary_predicate<Pred,
                std::projected<std::ranges::iterator_t<R>, Proj>>
        static decltype(auto) invoke_default(ExPolicy&& policy, R&& rng,
            R1&& output_yes, R2&& output_no, Pred pred, Proj proj = {})
        {
            using iterator_result =
                partition_copy_result<std::ranges::iterator_t<R>,
                    std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>>;
            using result_type =
                partition_copy_result<std::ranges::borrowed_iterator_t<R>,
                    std::ranges::borrowed_iterator_t<R1>,
                    std::ranges::borrowed_iterator_t<R2>>;
            auto first = std::ranges::begin(rng);
            auto yes = std::ranges::begin(output_yes);
            auto no = std::ranges::begin(output_no);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), yes,
                    yes + std::ranges::distance(output_yes), no,
                    no + std::ranges::distance(output_no), HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                [](iterator_result result) -> result_type {
                    return {result.in, result.out1, result.out2};
                });
        }
    } partition_copy{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
