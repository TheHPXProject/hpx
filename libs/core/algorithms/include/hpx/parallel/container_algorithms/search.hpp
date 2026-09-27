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

    /// \brief Execution-policy overload of \c search.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I1,
        std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
        std::sized_sentinel_for<I2> S2, typename Pred = std::ranges::equal_to,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I1>>
    search(ExPolicy&& policy, I1 first, S1 last, I2 s_first, S2 s_last,
        Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {});

    /// \brief Execution-policy overload of \c search.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R1,
        std::ranges::random_access_range R2,
        typename Pred = std::ranges::equal_to, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R1> && std::ranges::sized_range<R2> &&
        std::indirectly_comparable<std::ranges::iterator_t<R1>,
            std::ranges::iterator_t<R2>, Pred, Proj1, Proj2>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R1>>
    search(ExPolicy&& policy, R1&& rng1, R2&& rng2, Pred pred = {},
        Proj1 proj1 = {}, Proj2 proj2 = {});

    /// \brief Execution-policy overload of \c search_n.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename Pred = std::ranges::equal_to,
        typename Proj = hpx::identity,
        typename T = std::remove_cvref_t<
            std::invoke_result_t<Proj&, std::iter_value_t<I>&>>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::indirectly_comparable<I, T const*, Pred, Proj>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I>>
    search_n(ExPolicy&& policy, I first, S last,
        std::iter_difference_t<I> count, T const& value, Pred pred = {},
        Proj proj = {});

    /// \brief Execution-policy overload of \c search_n.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        typename Pred = std::ranges::equal_to, typename Proj = hpx::identity,
        typename T = std::remove_cvref_t<
            std::invoke_result_t<Proj&, std::ranges::range_value_t<R>&>>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::indirectly_comparable<std::ranges::iterator_t<R>, T const*, Pred,
            Proj>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R>>
    search_n(ExPolicy&& policy, R&& rng,
        std::ranges::range_difference_t<R> count, T const& value,
        Pred pred = {}, Proj proj = {});
}}    // namespace hpx::ranges

#else

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/algorithm_value.hpp>
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

        template <typename ExPolicy, std::random_access_iterator I1,
            std::sized_sentinel_for<I1> S1, std::random_access_iterator I2,
            std::sized_sentinel_for<I2> S2,
            typename Pred = std::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
        static decltype(auto) invoke_default(ExPolicy&& policy, I1 first,
            S1 last, I2 s_first, S2 s_last, Pred pred = {}, Proj1 proj1 = {},
            Proj2 proj2 = {})
        {
            auto end = first + (last - first);
            auto const size = s_last - s_first;
            return parallel::util::detail::convert_to_result(
                parallel::detail::search<I1, I1>().call(
                    HPX_FORWARD(ExPolicy, policy), first, end, s_first,
                    s_first + size, HPX_MOVE(pred), HPX_MOVE(proj1),
                    HPX_MOVE(proj2)),
                [end, size](I1 found) -> std::ranges::subrange<I1> {
                    return {found, found == end ? end : found + size};
                });
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
            using iterator = std::ranges::iterator_t<R1>;
            auto first1 = std::ranges::begin(rng1);
            auto first2 = std::ranges::begin(rng2);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first1,
                    first1 + std::ranges::distance(rng1), first2,
                    first2 + std::ranges::distance(rng2), HPX_MOVE(pred),
                    HPX_MOVE(proj1), HPX_MOVE(proj2)),
                [](std::ranges::subrange<iterator> result)
                    -> std::ranges::borrowed_subrange_t<R1> { return result; });
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
            return hpx::parallel::detail::search<FwdIter, FwdIter>().call(
                hpx::execution::seq, first, std::ranges::next(first, count),
                s_first, s_last, HPX_MOVE(op), HPX_MOVE(proj1),
                HPX_MOVE(proj2));
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
            return hpx::parallel::detail::search<FwdIter, FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first,
                std::ranges::next(first, count), s_first, s_last, HPX_MOVE(op),
                HPX_MOVE(proj1), HPX_MOVE(proj2));
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

            return hpx::parallel::detail::search<fwditer_type, fwditer_type>()
                .call(hpx::execution::seq, hpx::util::begin(rng1),
                    std::ranges::next(hpx::util::begin(rng1), count),
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

            return hpx::parallel::detail::search<fwditer_type, fwditer_type>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng1),
                    std::ranges::next(hpx::util::begin(rng1), count),
                    hpx::util::begin(rng2), hpx::util::end(rng2), HPX_MOVE(op),
                    HPX_MOVE(proj1), HPX_MOVE(proj2));
        }

        /// \brief Find the first run of count elements matching value.
        /// \returns The matching subrange, or an empty subrange at last.
        /// Task policies wrap the result in a future.
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Pred = std::ranges::equal_to,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::iter_value_t<I>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_comparable<I, T const*, Pred, Proj>
        static decltype(auto) invoke_default(ExPolicy&& policy, I first, S last,
            std::iter_difference_t<I> count, T const& value, Pred pred = {},
            Proj proj = {})
        {
            return parallel::detail::search_n_range<I>().call(
                HPX_FORWARD(ExPolicy, policy), first, first + (last - first),
                count, parallel::detail::algorithm_value<T>(value),
                HPX_MOVE(pred), HPX_MOVE(proj));
        }

        /// \brief Find a run in a sized random access range.
        /// \returns The borrowed matching subrange, or its future.
        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Pred = std::ranges::equal_to,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::ranges::range_value_t<R>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::indirectly_comparable<std::ranges::iterator_t<R>, T const*,
                Pred, Proj>
        static decltype(auto) invoke_default(ExPolicy&& policy, R&& rng,
            std::ranges::range_difference_t<R> count, T const& value,
            Pred pred = {}, Proj proj = {})
        {
            using iterator = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), count, value,
                    HPX_MOVE(pred), HPX_MOVE(proj)),
                [](std::ranges::subrange<iterator> result)
                    -> std::ranges::borrowed_subrange_t<R> { return result; });
        }

        using base_type = hpx::detail::tag_dispatch<search_n_t,
            hpx::detail::tag_parallel_algorithm<search_n_t>>;
        using base_type::operator();

        /// \brief Support list-initialized values with iterator arguments.
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Pred = std::ranges::equal_to,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::iter_value_t<I>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::indirectly_comparable<I, T const*, Pred, Proj>
        decltype(auto) operator()(ExPolicy&& policy, I first, S last,
            std::iter_difference_t<I> count, T const& value, Pred pred = {},
            Proj proj = {}) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy), first,
                last, count, value, HPX_MOVE(pred), HPX_MOVE(proj));
        }

        /// \brief Support list-initialized values with a range argument.
        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Pred = std::ranges::equal_to,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::ranges::range_value_t<R>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::indirectly_comparable<std::ranges::iterator_t<R>, T const*,
                Pred, Proj>
        decltype(auto) operator()(ExPolicy&& policy, R&& rng,
            std::ranges::range_difference_t<R> count, T const& value,
            Pred pred = {}, Proj proj = {}) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy),
                HPX_FORWARD(R, rng), count, value, HPX_MOVE(pred),
                HPX_MOVE(proj));
        }
    } search_n{};
}    // namespace hpx::ranges

#endif
