//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2018 Christopher Ogle
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/search.hpp
/// \page hpx::ranges::search, hpx::ranges::search_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent2       The type of the source sentinel used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_find requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of type dereferenced \a FwdIter.
    /// \tparam Proj2       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of type dereferenced \a FwdIter2.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param s_first      Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param s_last       Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a FwdIter1 as a projection operation
    ///                     before the actual predicate \a is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a FwdIter2 as a projection operation
    ///                     before the actual predicate \a is invoked.
    ///
    /// The comparison operations in the parallel \a search algorithm execute
    /// in sequential order in the calling thread.
    ///
    /// \returns  The \a search algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type \a task_execution_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a search algorithm returns an iterator to the beginning of
    ///           the first subsequence [s_first, s_last) in range [first, last).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, last), \a last is returned.
    ///           Additionally if the size of the subsequence is empty \a first is
    ///           returned. If no subsequence is found, \a last is returned.
    ///
    template <typename FwdIter, typename Sent, typename FwdIter2,
        typename Sent2, typename Pred = hpx::ranges::equal_to,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
    FwdIter search(FwdIter first, Sent last, FwdIter2 s_first, Sent2 s_last,
        Pred&& op = Pred(), Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent        The type of the source sentinel used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent2       The type of the source sentinel used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_find requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of type dereferenced \a FwdIter.
    /// \tparam Proj2       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of type dereferenced \a FwdIter2.
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
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a FwdIter1 as a projection operation
    ///                     before the actual predicate \a is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a FwdIter2 as a projection operation
    ///                     before the actual predicate \a is invoked.
    ///
    /// The comparison operations in the parallel \a search algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a search algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a search algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type \a task_execution_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a search algorithm returns an iterator to the beginning of
    ///           the first subsequence [s_first, s_last) in range [first, last).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, last), \a last is returned.
    ///           Additionally if the size of the subsequence is empty \a first is
    ///           returned. If no subsequence is found, \a last is returned.
    ///
    template <typename ExPolicy, typename FwdIter, typename Sent,
        typename FwdIter2, typename Sent2,
        typename Pred = hpx::ranges::equal_to, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    typename typename parallel::util::detail::algorithm_result<ExPolicy,
        FwdIter>::type
    search(ExPolicy&& policy, FwdIter first, Sent last, FwdIter2 s_first,
        Sent2 s_last, Pred&& op = Pred(), Proj1&& proj1 = Proj1(),
        Proj2&& proj2 = Proj2());

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam Rng1        The type of the examine range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Rng2        The type of the search range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_find requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of \a Rng1.
    /// \tparam Proj2       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of \a Rng2.
    ///
    /// \param rng1         Refers to the sequence of elements the algorithm
    ///                     will be examining.
    /// \param rng2         Refers to the sequence of elements the algorithm
    ///                     will be searching for.
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of \a rng1
    ///                     as a projection operation before the actual
    ///                     predicate \a is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of \a rng2
    ///                     as a projection operation before the actual
    ///                     predicate \a is invoked.
    ///
    /// The comparison operations in the parallel \a search algorithm execute
    /// in sequential order in the calling thread.
    ///
    /// \returns  The \a search algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type \a task_execution_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a search algorithm returns an iterator to the beginning of
    ///           the first subsequence [s_first, s_last) in range [first, last).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, last), \a last is returned.
    ///           Additionally if the size of the subsequence is empty \a first is
    ///           returned. If no subsequence is found, \a last is returned.
    ///
    template <typename Rng1, typename Rng2,
        typename Pred = hpx::ranges::equal_to, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    std::ranges::iterator_t<Rng1> search(Rng1&& rng1, Rng2&& rng2,
        Pred&& op = Pred(), Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng1        The type of the examine range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Rng2        The type of the search range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_find requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of \a Rng1.
    /// \tparam Proj2       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of \a Rng2.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng1         Refers to the sequence of elements the algorithm
    ///                     will be examining.
    /// \param rng2         Refers to the sequence of elements the algorithm
    ///                     will be searching for.
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of \a rng1
    ///                     as a projection operation before the actual
    ///                     predicate \a is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of \a rng2
    ///                     as a projection operation before the actual
    ///                     predicate \a is invoked.
    ///
    /// The comparison operations in the parallel \a search algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a search algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a search algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type \a task_execution_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a search algorithm returns an iterator to the beginning of
    ///           the first subsequence [s_first, s_last) in range [first, last).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, last), \a last is returned.
    ///           Additionally if the size of the subsequence is empty \a first is
    ///           returned. If no subsequence is found, \a last is returned.
    ///
    template <typename ExPolicy, typename Rng1, typename Rng2,
        typename Pred = hpx::ranges::equal_to, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        std::ranges::iterator_t<Rng1>>
    search(ExPolicy&& policy, Rng1&& rng1, Rng2&& rng2, Pred&& op = Pred(),
        Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = count.
    ///
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent2       The type of the source sentinel used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_find requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of type dereferenced \a FwdIter.
    /// \tparam Proj2       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of type dereferenced \a FwdIter2.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param count        Refers to the range of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param s_first      Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param s_last       Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a FwdIter1 as a projection operation
    ///                     before the actual predicate \a is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a FwdIter2 as a projection operation
    ///                     before the actual predicate \a is invoked.
    ///
    /// The comparison operations in the parallel \a search_n algorithm execute
    /// in sequential order in the calling thread.
    ///
    /// \returns  The \a search_n algorithm returns \a FwdIter.
    ///           The \a search_n algorithm returns an iterator to the beginning of
    ///           the last subsequence [s_first, s_last) in range [first, first+count).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, first+count),
    ///           \a first is returned.
    ///           Additionally, if the size of the subsequence is empty or no subsequence
    ///           is found, \a first is also returned.
    ///
    template <typename FwdIter, typename FwdIter2, typename Sent2,
        typename Pred = hpx::ranges::equal_to, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    FwdIter search_n(FwdIter first, std::size_t count, FwdIter2 s_first,
        Sent s_last, Pred&& op = Pred(), Proj1&& proj1 = Proj1(),
        Proj2&& proj2 = Proj2());

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = count.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent2       The type of the source sentinel used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_find requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of type dereferenced \a FwdIter.
    /// \tparam Proj2       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of type dereferenced \a FwdIter2.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param count        Refers to the range of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param s_first      Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param s_last       Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a FwdIter1 as a projection operation
    ///                     before the actual predicate \a is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of type
    ///                     dereferenced \a FwdIter2 as a projection operation
    ///                     before the actual predicate \a is invoked.
    ///
    /// The comparison operations in the parallel \a search_n algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a search_n algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a search_n algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type \a task_execution_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a search_n algorithm returns an iterator to the beginning of
    ///           the last subsequence [s_first, s_last) in range [first, first+count).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, first+count),
    ///           \a first is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a first is also returned.
    ///
    template <typename ExPolicy, typename FwdIter, typename FwdIter2,
        typename Sent2, typename Pred = hpx::ranges::equal_to,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        FwdIter>::type
    search_n(ExPolicy&& policy, FwdIter first, std::size_t count,
        FwdIter2 s_first, Sent2 s_last, Pred&& op = Pred(),
        Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam Rng1        The type of the examine range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Rng2        The type of the search range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_find requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of \a Rng1.
    /// \tparam Proj2       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of \a Rng2.
    ///
    /// \param rng1         Refers to the sequence of elements the algorithm
    ///                     will be examining.
    /// \param count        The number of elements to apply the algorithm on.
    /// \param rng2         Refers to the sequence of elements the algorithm
    ///                     will be searching for.
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of \a rng1
    ///                     as a projection operation before the actual
    ///                     predicate \a is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of \a rng2
    ///                     as a projection operation before the actual
    ///                     predicate \a is invoked.
    ///
    /// The comparison operations in the parallel \a search algorithm execute
    /// in sequential order in the calling thread.
    ///
    /// \returns  The \a search algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type \a task_execution_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a search algorithm returns an iterator to the beginning of
    ///           the first subsequence [s_first, s_last) in range [first, last).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, last), \a last is returned.
    ///           Additionally if the size of the subsequence is empty \a first is
    ///           returned. If no subsequence is found, \a last is returned.
    ///
    template <typename Rng1, typename Rng2,
        typename Pred = hpx::ranges::equal_to, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    std::ranges::iterator_t<Rng1> search_n(Rng1&& rng1, std::size_t count,
        Rng2&& rng2, Pred&& op = Pred(), Proj1&& proj1 = Proj1(),
        Proj2&& proj2 = Proj2());

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng1        The type of the examine range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Rng2        The type of the search range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_find requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    /// \tparam Proj1       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of \a Rng1.
    /// \tparam Proj2       The type of an optional projection function. This
    ///                     defaults to \a hpx::identity and is applied
    ///                     to the elements of \a Rng2.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng1         Refers to the sequence of elements the algorithm
    ///                     will be examining.
    /// \param count        The number of elements to apply the algorithm on.
    /// \param rng2         Refers to the sequence of elements the algorithm
    ///                     will be searching for.
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    /// \param proj1        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of \a rng1
    ///                     as a projection operation before the actual
    ///                     predicate \a is invoked.
    /// \param proj2        Specifies the function (or function object) which
    ///                     will be invoked for each of the elements of \a rng2
    ///                     as a projection operation before the actual
    ///                     predicate \a is invoked.
    ///
    /// The comparison operations in the parallel \a search algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a search algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a search algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type \a task_execution_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a search algorithm returns an iterator to the beginning of
    ///           the first subsequence [s_first, s_last) in range [first, last).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, last), \a last is returned.
    ///           Additionally if the size of the subsequence is empty \a first is
    ///           returned. If no subsequence is found, \a last is returned.
    ///
    template <typename ExPolicy, typename Rng1, typename Rng2,
        typename Pred = hpx::ranges::equal_to, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        std::ranges::iterator_t<Rng1>>
    search_n(ExPolicy&& policy, Rng1&& rng1, std::size_t count, Rng2&& rng2,
        Pred&& op = Pred(), Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2());

}}    // namespace hpx::ranges

#else

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/search.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT inline constexpr struct search_t final
      : hpx::detail::tag_dispatch<search_t,
            hpx::detail::tag_parallel_algorithm<search_t>>
    {
        template <typename FwdIter, typename Sent, typename FwdIter2,
            typename Sent2, typename Pred = hpx::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                std::forward_iterator<FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                parallel::traits::is_projected_v<Proj1, FwdIter> &&
                std::forward_iterator<FwdIter2> &&
                std::sentinel_for<Sent2, FwdIter2> &&
                parallel::traits::is_projected_v<Proj2, FwdIter2> &&
                parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    parallel::traits::projected<Proj1, FwdIter>,
                    parallel::traits::projected<Proj2, FwdIter2>
                >
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, Sent last,
            FwdIter2 s_first, Sent2 s_last, Pred op = Pred(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            return hpx::parallel::detail::search<FwdIter, Sent>().call(
                hpx::execution::seq, first, last, s_first, s_last, HPX_MOVE(op),
                HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename ExPolicy, typename FwdIter, typename Sent,
            typename FwdIter2, typename Sent2,
            typename Pred = hpx::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                std::sentinel_for<Sent, FwdIter> &&
                parallel::traits::is_projected_v<Proj1, FwdIter> &&
                std::forward_iterator<FwdIter2> &&
                std::sentinel_for<Sent2, FwdIter2> &&
                parallel::traits::is_projected_v<Proj2, FwdIter2> &&
                parallel::traits::is_indirect_callable_v<
                    ExPolicy, Pred,
                    parallel::traits::projected<Proj1, FwdIter>,
                    parallel::traits::projected<Proj2, FwdIter2>
                >
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
        invoke_default(ExPolicy&& policy, FwdIter first, Sent last,
            FwdIter2 s_first, Sent2 s_last, Pred op = Pred(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            return hpx::parallel::detail::search<FwdIter, Sent>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, s_first, s_last,
                HPX_MOVE(op), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename Rng1, typename Rng2,
            typename Pred = hpx::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng1> &&
                hpx::parallel::traits::is_projected_range_v<Proj1, Rng1> &&
                std::ranges::range<Rng2> &&
                hpx::parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy,
                    Pred, hpx::parallel::traits::projected_range<Proj1, Rng1>,
                    hpx::parallel::traits::projected_range<Proj2, Rng2>
                >
            )
        // clang-format on
        static std::ranges::iterator_t<Rng1> invoke_default(Rng1&& rng1,
            Rng2&& rng2, Pred op = Pred(), Proj1 proj1 = Proj1(),
            Proj2 proj2 = Proj2())
        {
            using fwditer_type = std::ranges::iterator_t<Rng1>;
            using sent_type = std::ranges::sentinel_t<Rng1>;

            return hpx::parallel::detail::search<fwditer_type, sent_type>()
                .call(hpx::execution::seq, hpx::util::begin(rng1),
                    hpx::util::end(rng1), hpx::util::begin(rng2),
                    hpx::util::end(rng2), HPX_MOVE(op), HPX_MOVE(proj1),
                    HPX_MOVE(proj2));
        }

        template <typename ExPolicy, typename Rng1, typename Rng2,
            typename Pred = hpx::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng1> &&
                hpx::parallel::traits::is_projected_range_v<Proj1, Rng1> &&
                std::ranges::range<Rng2> &&
                hpx::parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                hpx::parallel::traits::is_indirect_callable_v<ExPolicy,
                    Pred, hpx::parallel::traits::projected_range<Proj1, Rng1>,
                    hpx::parallel::traits::projected_range<Proj2, Rng2>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            std::ranges::iterator_t<Rng1>>
        invoke_default(ExPolicy&& policy, Rng1&& rng1, Rng2&& rng2,
            Pred op = Pred(), Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            using fwditer_type = std::ranges::iterator_t<Rng1>;
            using sent_type = std::ranges::sentinel_t<Rng1>;

            return hpx::parallel::detail::search<fwditer_type, sent_type>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng1),
                    hpx::util::end(rng1), hpx::util::begin(rng2),
                    hpx::util::end(rng2), HPX_MOVE(op), HPX_MOVE(proj1),
                    HPX_MOVE(proj2));
        }
    } search{};

    HPX_CXX_CORE_EXPORT inline constexpr struct search_n_t final
      : hpx::detail::tag_dispatch<search_n_t,
            hpx::detail::tag_parallel_algorithm<search_n_t>>
    {
        template <typename FwdIter, typename FwdIter2, typename Sent2,
            typename Pred = hpx::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                std::forward_iterator<FwdIter> &&
                parallel::traits::is_projected_v<Proj1, FwdIter> &&
                std::forward_iterator<FwdIter2> &&
                std::sentinel_for<Sent2, FwdIter2> &&
                parallel::traits::is_projected_v<Proj2, FwdIter2> &&
                parallel::traits::is_indirect_callable<
                    hpx::execution::sequenced_policy, Pred,
                    parallel::traits::projected<Proj1, FwdIter>,
                    parallel::traits::projected<Proj2, FwdIter2>
                >::value
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, std::size_t count,
            FwdIter2 s_first, Sent2 s_last, Pred op = Pred(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            return hpx::parallel::detail::search_n<FwdIter, FwdIter>().call(
                hpx::execution::seq, first, count, s_first, s_last,
                HPX_MOVE(op), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename ExPolicy, typename FwdIter, typename FwdIter2,
            typename Sent2, typename Pred = hpx::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                parallel::traits::is_projected_v<Proj1, FwdIter> &&
                std::forward_iterator<FwdIter2> &&
                std::sentinel_for<Sent2, FwdIter2> &&
                parallel::traits::is_projected_v<Proj2, FwdIter2>&&
                parallel::traits::is_indirect_callable_v<
                    ExPolicy, Pred,
                    parallel::traits::projected<Proj1, FwdIter>,
                    parallel::traits::projected<Proj2, FwdIter2>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            FwdIter>
        invoke_default(ExPolicy&& policy, FwdIter first, std::size_t count,
            FwdIter2 s_first, Sent2 s_last, Pred op = Pred(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            return hpx::parallel::detail::search_n<FwdIter, FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, count, s_first, s_last,
                HPX_MOVE(op), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename Rng1, typename Rng2,
            typename Pred = hpx::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng1> &&
                hpx::parallel::traits::is_projected_range_v<Proj1, Rng1> &&
                std::ranges::range<Rng2> &&
                hpx::parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected_range<Proj1, Rng1>,
                    hpx::parallel::traits::projected_range<Proj2, Rng2>
                >
            )
        // clang-format on
        static std::ranges::iterator_t<Rng1> invoke_default(Rng1&& rng1,
            std::size_t count, Rng2&& rng2, Pred op = Pred(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            using fwditer_type = std::ranges::iterator_t<Rng1>;
            using sent_type = std::ranges::sentinel_t<Rng1>;

            return hpx::parallel::detail::search_n<fwditer_type, sent_type>()
                .call(hpx::execution::seq, hpx::util::begin(rng1), count,
                    hpx::util::begin(rng2), hpx::util::end(rng2), HPX_MOVE(op),
                    HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        template <typename ExPolicy, typename Rng1, typename Rng2,
            typename Pred = hpx::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng1> &&
                hpx::parallel::traits::is_projected_range_v<Proj1, Rng1> &&
                std::ranges::range<Rng2> &&
                hpx::parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    ExPolicy, Pred,
                    hpx::parallel::traits::projected_range<Proj1, Rng1>,
                    hpx::parallel::traits::projected_range<Proj2, Rng2>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            std::ranges::iterator_t<Rng1>>
        invoke_default(ExPolicy&& policy, Rng1&& rng1, std::size_t count,
            Rng2&& rng2, Pred op = Pred(), Proj1 proj1 = Proj1(),
            Proj2 proj2 = Proj2())
        {
            using fwditer_type = std::ranges::iterator_t<Rng1>;
            using sent_type = std::ranges::sentinel_t<Rng1>;

            return hpx::parallel::detail::search_n<fwditer_type, sent_type>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng1),
                    count, hpx::util::begin(rng2), hpx::util::end(rng2),
                    HPX_MOVE(op), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }
    } search_n{};
}    // namespace hpx::ranges

#endif
