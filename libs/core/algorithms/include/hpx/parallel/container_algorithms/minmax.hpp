//  Copyright (c) 2014-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// make inspect happy: hpxinspect:nominmax

/// \file parallel/container_algorithms/minmax.hpp
/// \page hpx::ranges::min_element, hpx::ranges::max_element, hpx::ranges::minmax_element
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx::ranges {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the smallest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam FwdIter     The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            The binary predicate which returns true if the
    ///                     the left argument is less than the right element.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparisons in the parallel \a min_element algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a min_element algorithm  returns \a FwdIter.
    ///           The \a min_element algorithm returns the iterator to the
    ///           smallest element in the range [first, last). If several
    ///           elements in the range are equivalent to the smallest element,
    ///           returns the iterator to the first such element. Returns last
    ///           if the range is empty.
    ///
    template <typename FwdIter, typename Sent,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    FwdIter min_element(
        FwdIter first, Sent last, F&& f = F(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the smallest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param f            The binary predicate which returns true if the
    ///                     the left argument is less than the right element.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparisons in the parallel \a min_element algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a min_element algorithm returns a \a hpx::traits::range_iterator<Rng>::type otherwise.
    ///           The \a min_element algorithm returns the iterator to the
    ///           smallest element in the range [first, last). If several
    ///           elements in the range are equivalent to the smallest element,
    ///           returns the iterator to the first such element. Returns last
    ///           if the range is empty.
    ///
    template <typename Rng,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    std::ranges::iterator_t<Rng> min_element(
        Rng&& rng, F&& f = F(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the smallest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a min_element requires \a F to meet the
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
    /// \param f            The binary predicate which returns true if the
    ///                     the left argument is less than the right element.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparisons in the parallel \a min_element algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparisons in the parallel \a min_element algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a min_element algorithm returns a \a hpx::future<FwdIter>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy
    ///           and returns \a FwdIter otherwise.
    ///           The \a min_element algorithm returns the iterator to the
    ///           smallest element in the range [first, last). If several
    ///           elements in the range are equivalent to the smallest element,
    ///           returns the iterator to the first such element. Returns last
    ///           if the range is empty.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    min_element(ExPolicy&& policy, FwdIter first, Sent last, F&& f = F(),
        Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the smallest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a min_element requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param f            The binary predicate which returns true if the
    ///                     the left argument is less than the right element.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparisons in the parallel \a min_element algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparisons in the parallel \a min_element algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a min_element algorithm returns a \a hpx::future<hpx::traits::range_iterator<Rng>::type>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy
    ///           and returns \a FwdIter otherwise.
    ///           The \a min_element algorithm returns the iterator to the
    ///           smallest element in the range [first, last). If several
    ///           elements in the range are equivalent to the smallest element,
    ///           returns the iterator to the first such element. Returns last
    ///           if the range is empty.
    ///
    template <typename ExPolicy, typename Rng,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::iterator_t<Rng>>
    min_element(
        ExPolicy&& policy, Rng&& rng, F&& f = F(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the greatest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam FwdIter     The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            The binary predicate which returns true if the
    ///                     This argument is optional and defaults to std::less.
    ///                     the left argument is less than the right element.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparisons in the parallel \a max_element algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a max_element algorithm returns a \a FwdIter.
    ///           The \a max_element algorithm returns the iterator to the
    ///           smallest element in the range [first, last). If several
    ///           elements in the range are equivalent to the smallest element,
    ///           returns the iterator to the first such element. Returns last
    ///           if the range is empty.
    ///
    template <typename FwdIter, typename Sent,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    FwdIter max_element(
        FwdIter first, Sent last, F&& f = F(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the greatest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param f            The binary predicate which returns true if the
    ///                     This argument is optional and defaults to std::less.
    ///                     the left argument is less than the right element.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparisons in the parallel \a max_element algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a max_element algorithm returns a \a hpx::traits::range_iterator<Rng>::type otherwise.
    ///           The \a max_element algorithm returns the iterator to the
    ///           smallest element in the range [first, last). If several
    ///           elements in the range are equivalent to the smallest element,
    ///           returns the iterator to the first such element. Returns last
    ///           if the range is empty.
    ///
    template <typename Rng,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    std::ranges::iterator_t<Rng> max_element(
        Rng&& rng, F&& f = F(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the greatest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a max_element requires \a F to meet the
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
    /// \param f            The binary predicate which returns true if the
    ///                     This argument is optional and defaults to std::less.
    ///                     the left argument is less than the right element.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparisons in the parallel \a max_element algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparisons in the parallel \a max_element algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a max_element algorithm returns a \a hpx::future<FwdIter>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy
    ///           and returns \a FwdIter otherwise.
    ///           The \a max_element algorithm returns the iterator to the
    ///           smallest element in the range [first, last). If several
    ///           elements in the range are equivalent to the smallest element,
    ///           returns the iterator to the first such element. Returns last
    ///           if the range is empty.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    max_element(ExPolicy&& policy, FwdIter first, Sent last, F&& f = F(),
        Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the greatest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a max_element requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param f            The binary predicate which returns true if the
    ///                     This argument is optional and defaults to std::less.
    ///                     the left argument is less than the right element.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparisons in the parallel \a max_element algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparisons in the parallel \a max_element algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a max_element algorithm returns a \a hpx::future<hpx::traits::range_iterator<Rng>::type>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy
    ///           and returns \a FwdIter otherwise.
    ///           The \a max_element algorithm returns the iterator to the
    ///           smallest element in the range [first, last). If several
    ///           elements in the range are equivalent to the smallest element,
    ///           returns the iterator to the first such element. Returns last
    ///           if the range is empty.
    ///
    template <typename ExPolicy, typename Rng,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::iterator_t<Rng>>
    max_element(ExPolicy&& policy, Rng&& rng, F&& f = F(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the greatest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: At most \a max(floor(3/2*(N-1)), 0) applications of
    ///                     the predicate, where N = std::distance(first, last).
    ///
    /// \tparam FwdIter     The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            The binary predicate which returns true if the
    ///                     the left argument is less than the right element.
    ///                     This argument is optional and defaults to std::less.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a minmax_element algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a minmax_element algorithm returns a
    ///           \a minmax_element_result<FwdIter, FwdIter>
    ///           The \a minmax_element algorithm returns a min_max_result consisting of
    ///           an iterator to the smallest element as the min element and
    ///           an iterator to the greatest element as the max element. Returns
    ///           minmax_element_result{first, first} if the range is empty. If
    ///           several elements are equivalent to the smallest element, the
    ///           iterator to the first such element is returned. If several
    ///           elements are equivalent to the largest element, the iterator
    ///           to the last such element is returned.
    ///
    template <typename FwdIter, typename Sent,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    minmax_element_result<FwdIter> minmax_element(
        FwdIter first, Sent last, F&& f = F(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the greatest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: At most \a max(floor(3/2*(N-1)), 0) applications of
    ///                     the predicate, where N = std::distance(first, last).
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param f            The binary predicate which returns true if the
    ///                     the left argument is less than the right element.
    ///                     This argument is optional and defaults to std::less.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a minmax_element algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a minmax_element algorithm returns a
    /// \a minmax_element_result<hpx::traits::range_iterator<Rng>::type
    /// , hpx::traits::range_iterator<Rng>::type>
    ///           The \a minmax_element algorithm returns a min_max_result consisting of
    ///           an range iterator to the smallest element as the min element and
    ///           an range iterator to the greatest element as the max element. If
    ///           several elements are equivalent to the smallest element, the
    ///           iterator to the first such element is returned. If several
    ///           elements are equivalent to the largest element, the iterator
    ///           to the last such element is returned.
    ///
    template <typename Rng,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    minmax_element_result<std::ranges::iterator_t<Rng>>
    minmax_element(Rng&& rng, F&& f = F(), Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the greatest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: At most \a max(floor(3/2*(N-1)), 0) applications of
    ///                     the predicate, where N = std::distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterator used (deduced).
    ///                     The iterator type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a minmax_element requires \a F to meet the
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
    /// \param f            The binary predicate which returns true if the
    ///                     the left argument is less than the right element.
    ///                     This argument is optional and defaults to std::less.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparisons in the parallel \a minmax_element algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparisons in the parallel \a minmax_element algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a minmax_element algorithm returns a \a minmax_element_result<FwdIter, FwdIter>
    ///           The \a minmax_element algorithm returns a min_max_result consisting of
    ///           an iterator to the smallest element as the min element and
    ///           an iterator to the greatest element as the max element. Returns
    ///           minmax_element_result{first, first} if the range is empty. If
    ///           several elements are equivalent to the smallest element, the
    ///           iterator to the first such element is returned. If several
    ///           elements are equivalent to the largest element, the iterator
    ///           to the last such element is returned.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        minmax_element_result<FwdIter>>
    minmax_element(ExPolicy&& policy, FwdIter first, Sent last, F&& f = F(),
        Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Finds the greatest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: At most \a max(floor(3/2*(N-1)), 0) applications of
    ///                     the predicate, where N = std::distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a minmax_element requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param f            The binary predicate which returns true if the
    ///                     the left argument is less than the right element.
    ///                     This argument is optional and defaults to std::less.
    ///                     The signature
    ///                     of the predicate function should be equivalent to
    ///                     the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type1 must be such that objects of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to \a Type1.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The comparisons in the parallel \a minmax_element algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparisons in the parallel \a minmax_element algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a minmax_element algorithm returns a
    ///           \a minmax_element_result<hpx::traits::range_iterator<Rng>::type,
    ///           hpx::traits::range_iterator<Rng>::type>
    ///           The \a minmax_element algorithm returns a min_max_result consisting of
    ///           an range iterator to the smallest element as the min element and
    ///           an range iterator to the greatest element as the max element. If
    ///           several elements are equivalent to the smallest element, the
    ///           iterator to the first such element is returned. If several
    ///           elements are equivalent to the largest element, the iterator
    ///           to the last such element is returned.
    ///
    template <typename ExPolicy, typename Rng,
        typename F = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        minmax_element_result<std::ranges::iterator_t<Rng>>>
    minmax_element(ExPolicy&& policy, Rng&& rng, F&& f = F(), Proj&& proj = Proj());
    // clang-format on
}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/minmax.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    /// `minmax_element_result` is equivalent to
    /// `hpx::parallel::util::min_max_result`
    HPX_CXX_CORE_EXPORT template <typename T>
    using minmax_element_result = hpx::parallel::util::min_max_result<T>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::min_element
    HPX_CXX_CORE_EXPORT inline constexpr struct min_element_t final
      : hpx::detail::tag_dispatch<min_element_t,
            hpx::detail::tag_parallel_algorithm<min_element_t>>
    {
        template <typename FwdIter, typename Sent,
            typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, F,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        static FwdIter invoke_default(
            FwdIter first, Sent last, F f = F(), Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::min_element<FwdIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), HPX_MOVE(proj));
        }

        template <typename Rng, typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, F,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static std::ranges::iterator_t<Rng> invoke_default(
            Rng&& rng, F f = F(), Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::min_element<
                std::ranges::iterator_t<Rng>>()
                .call(hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), HPX_MOVE(f), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename FwdIter, typename Sent,
            typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, F,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            FwdIter>
        invoke_default(ExPolicy&& policy, FwdIter first, Sent last, F f = F(),
            Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::min_element<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng,
            typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, F,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            std::ranges::iterator_t<Rng>>
        invoke_default(
            ExPolicy&& policy, Rng&& rng, F f = F(), Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::min_element<
                std::ranges::iterator_t<Rng>>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                    hpx::util::end(rng), HPX_MOVE(f), HPX_MOVE(proj));
        }
    } min_element{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::max_element
    HPX_CXX_CORE_EXPORT inline constexpr struct max_element_t final
      : hpx::detail::tag_dispatch<max_element_t,
            hpx::detail::tag_parallel_algorithm<max_element_t>>
    {
        template <typename FwdIter, typename Sent,
            typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, F,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        static FwdIter invoke_default(
            FwdIter first, Sent last, F f = F(), Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::max_element<FwdIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), HPX_MOVE(proj));
        }

        template <typename Rng, typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, F,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static std::ranges::iterator_t<Rng> invoke_default(
            Rng&& rng, F f = F(), Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::max_element<
                std::ranges::iterator_t<Rng>>()
                .call(hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), HPX_MOVE(f), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename FwdIter, typename Sent,
            typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, F,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            FwdIter>
        invoke_default(ExPolicy&& policy, FwdIter first, Sent last, F f = F(),
            Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::max_element<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng,
            typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, F,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            std::ranges::iterator_t<Rng>>
        invoke_default(
            ExPolicy&& policy, Rng&& rng, F f = F(), Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::max_element<
                std::ranges::iterator_t<Rng>>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                    hpx::util::end(rng), HPX_MOVE(f), HPX_MOVE(proj));
        }
    } max_element{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::minmax_element
    HPX_CXX_CORE_EXPORT inline constexpr struct minmax_element_t final
      : hpx::detail::tag_dispatch<minmax_element_t,
            hpx::detail::tag_parallel_algorithm<minmax_element_t>>
    {
        template <typename FwdIter, typename Sent,
            typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, F,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        static minmax_element_result<FwdIter> invoke_default(
            FwdIter first, Sent last, F f = F(), Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::minmax_element<FwdIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), HPX_MOVE(proj));
        }

        template <typename Rng, typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, F,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static minmax_element_result<std::ranges::iterator_t<Rng>>
        invoke_default(Rng&& rng, F f = F(), Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::minmax_element<
                std::ranges::iterator_t<Rng>>()
                .call(hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), HPX_MOVE(f), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename FwdIter, typename Sent,
            typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, F,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            minmax_element_result<FwdIter>>
        invoke_default(ExPolicy&& policy, FwdIter first, Sent last, F f = F(),
            Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::minmax_element<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng,
            typename F = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, F,
                    hpx::parallel::traits::projected_range<Proj, Rng>,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            minmax_element_result<std::ranges::iterator_t<Rng>>>
        invoke_default(
            ExPolicy&& policy, Rng&& rng, F f = F(), Proj proj = Proj())
        {
            static_assert(std::forward_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::minmax_element<
                std::ranges::iterator_t<Rng>>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                    hpx::util::end(rng), HPX_MOVE(f), HPX_MOVE(proj));
        }
    } minmax_element{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
