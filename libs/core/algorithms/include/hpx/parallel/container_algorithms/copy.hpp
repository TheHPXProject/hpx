//  Copyright (c) 2022 Dimitra Karatza
//  Copyright (c) 2015-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/copy.hpp
/// \page hpx::ranges::copy, hpx::ranges::copy_n, hpx::ranges::copy_if
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace ranges {
    // clang-format off

    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the begin source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent1       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter1.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param iter         Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a copy algorithm invoked with an
    /// execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a copy algorithm returns a
    ///           \a hpx::future<ranges::copy_result<FwdIter1, FwdIter> > if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy and
    ///           returns \a ranges::copy_result<FwdIter1, FwdIter> otherwise.
    ///           The \a copy algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element copied.
    template <typename ExPolicy, typename FwdIter1, typename Sent1,
        typename FwdIter>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        ranges::copy_result<FwdIter1, FwdIter>>::type
    copy(ExPolicy&& policy, FwdIter1 iter, Sent1 sent, FwdIter dest);

    /// Copies the elements in the range \a rng to another
    /// range beginning at \a dest.
    ///
    /// \note   Complexity: Performs exactly
    ///         std::distance(begin(rng), end(rng)) assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a copy algorithm invoked with an
    /// execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a copy algorithm returns a
    ///           \a hpx::future<ranges::copy_result<iterator_t<Rng>, FwdIter2>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy and
    ///           returns \a ranges::copy_result<iterator_t<Rng>, FwdIter2>
    ///           otherwise.
    ///           The \a copy algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element copied.
    template <typename ExPolicy, typename Rng, typename FwdIter>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        ranges::copy_result<
            typename hpx::traits::range_traits<Rng>::iterator_type,
            FwdIter>>::type
    copy(ExPolicy&& policy, Rng&& rng, FwdIter dest);

    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam FwdIter1    The type of the begin source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent1       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for Iter1.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param iter         Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// \returns  The \a copy algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element copied.
    template <typename FwdIter1, typename Sent1, typename FwdIter>
    ranges::copy_result<FwdIter1, FwdIter>
    copy(FwdIter1 iter, Sent1 sent, FwdIter dest);

    /// Copies the elements in the range \a rng to another
    /// range beginning at \a dest.
    ///
    /// \note   Complexity: Performs exactly
    ///         std::distance(begin(rng), end(rng)) assignments.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// \returns  The \a copy algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element copied.
    template <typename Rng, typename FwdIter>
    ranges::copy_result<typename hpx::traits::range_traits<Rng>::iterator_type,
        FwdIter> copy(Rng&& rng, FwdIter dest);

    /// Copies the elements in the range [first, first + count), starting from
    /// first and proceeding to first + count - 1., to another range beginning
    /// at dest.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a copy_n algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a copy_n algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a copy_n algorithm returns a
    ///           \a hpx::future<ranges::copy_n_result<FwdIter1, FwdIter2> >
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a ranges::copy_n_result<FwdIter1, FwdIter2>
    ///           otherwise.
    ///           The \a copy algorithm returns the pair of the input iterator
    ///           forwarded to the first element after the last in the input
    ///           sequence and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Size,
        typename FwdIter2>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        ranges::copy_n_result<FwdIter1, FwdIter2>>::type
    copy_n(ExPolicy&& policy, FwdIter1 first, Size count, FwdIter2 dest);

    /// Copies the elements in the range [first, first + count), starting from
    /// first and proceeding to first + count - 1., to another range beginning
    /// at dest.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// \returns  The \a copy algorithm returns the pair of the input iterator
    ///           forwarded to the first element after the last in the input
    ///           sequence and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename FwdIter1, typename Size, typename FwdIter2>
    ranges::copy_n_result<FwdIter1, FwdIter2>
    copy_n(FwdIter1 first, Size count, FwdIter2 dest);

    /// Copies the elements in the range, defined by [first, last) to another
    /// range beginning at \a dest. The order of the elements that are not
    /// removed is preserved.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the begin source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent1       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for FwdIter1.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param iter         Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The
    ///                     signature should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 must be such
    ///                     that objects of type \a FwdIter
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 .
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a copy_if algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a copy_if algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a copy_if algorithm returns a
    ///           \a hpx::future<ranges::copy_if_result<iterator_t<Rng>, FwdIter2>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy and
    ///           returns \a ranges::copy_if_result<iterator_t<Rng>, FwdIter2>
    ///           otherwise.
    ///           The \a copy_if algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Sent1,
        typename FwdIter, typename Pred,
        typename Proj = hpx::identity>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        ranges::copy_if_result<FwdIter1, FwdIter>>::type
    copy_if(ExPolicy&& policy, FwdIter1 iter, Sent1 sent, FwdIter dest, Pred&& pred,
        Proj&& proj = Proj());

    /// Copies the elements in the range, defined by \a rng to another
    /// range beginning at \a dest. The order of the elements that are not
    /// removed is preserved.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The
    ///                     signature should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 must be such
    ///                     that objects of type \a FwdIter
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 .
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a copy_if algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a copy_if algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a copy_if algorithm returns a
    ///           \a hpx::future<ranges::copy_if_result<iterator_t<Rng>, FwdIter2>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy and
    ///           returns \a ranges::copy_if_result<iterator_t<Rng>, FwdIter2>
    ///           otherwise.
    ///           The \a copy_if algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element copied.
    ///
    template <typename ExPolicy, typename Rng, typename FwdIter,
        typename Pred,
        typename Proj = hpx::identity>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        ranges::copy_if_result<
            typename hpx::traits::range_traits<Rng>::iterator_type,
            FwdIter>>::type
    copy_if(ExPolicy&& policy, Rng&& rng, FwdIter dest, Pred&& pred,
        Proj&& proj = Proj());

    /// Copies the elements in the range, defined by [first, last) to another
    /// range beginning at \a dest. The order of the elements that are not
    /// removed is preserved.
    ///
    /// \tparam FwdIter1    The type of the begin source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Sent1       The type of the end source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     sentinel for FwdIter1.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param iter         Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The
    ///                     signature should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 must be such
    ///                     that objects of type \a FwdIter
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 .
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a copy_if algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element copied.
    ///
    template <typename FwdIter1, typename Sent1, typename FwdIter,
        typename Pred,
        typename Proj = hpx::identity>
    ranges::copy_if_result<FwdIter1, FwdIter>
    copy_if(FwdIter1 iter, Sent1 sent, FwdIter dest, Pred&& pred,
        Proj&& proj = Proj());

    /// Copies the elements in the range, defined by \a rng to another
    /// range beginning at \a dest. The order of the elements that are not
    /// removed is preserved.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The
    ///                     signature should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 must be such
    ///                     that objects of type \a FwdIter
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 .
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// \returns  The \a copy_if algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the element in the
    ///           destination range, one past the last element copied.
    ///
    template <typename Rng, typename FwdIter, typename Pred,
        typename Proj = hpx::identity>
    ranges::copy_if_result<
        typename hpx::traits::range_traits<Rng>::iterator_type, FwdIter>
    copy_if(Rng&& rng, FwdIter dest, Pred&& pred,
        Proj&& proj = Proj());

    // clang-format on
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/copy.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using copy_result = parallel::util::in_out_result<I, O>;

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using copy_n_result = parallel::util::in_out_result<I, O>;

    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    using copy_if_result = parallel::util::in_out_result<I, O>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::copy
    HPX_CXX_CORE_EXPORT inline constexpr struct copy_t final
      : hpx::detail::tag_dispatch<copy_t,
            hpx::detail::tag_parallel_algorithm<copy_t>>
    {
        template <typename ExPolicy, typename FwdIter1, typename Sent1,
            typename FwdIter>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent1, FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            ranges::copy_result<FwdIter1, FwdIter>>
        invoke_default(
            ExPolicy&& policy, FwdIter1 iter, Sent1 sent, FwdIter dest)
        {
            using copy_iter_t =
                hpx::parallel::detail::copy_iter<FwdIter1, FwdIter>;

            return hpx::parallel::detail::transfer<copy_iter_t>(
                HPX_FORWARD(ExPolicy, policy), iter, sent, dest);
        }

        template <typename ExPolicy, typename Rng, typename FwdIter>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            ranges::copy_result<
                typename hpx::traits::range_traits<Rng>::iterator_type,
                FwdIter>>
        invoke_default(ExPolicy&& policy, Rng&& rng, FwdIter dest)
        {
            using copy_iter_t = hpx::parallel::detail::copy_iter<
                typename hpx::traits::range_traits<Rng>::iterator_type,
                FwdIter>;

            return hpx::parallel::detail::transfer<copy_iter_t>(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::end(rng), dest);
        }

        template <typename FwdIter1, typename Sent1, typename FwdIter>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent1, FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static ranges::copy_result<FwdIter1, FwdIter> invoke_default(
            FwdIter1 iter, Sent1 sent, FwdIter dest)
        {
            using copy_iter_t =
                hpx::parallel::detail::copy_iter<FwdIter1, FwdIter>;

            return hpx::parallel::detail::transfer<copy_iter_t>(
                hpx::execution::seq, iter, sent, dest);
        }

        template <typename Rng, typename FwdIter>
        // clang-format off
            requires (
                std::ranges::range<Rng> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static ranges::copy_result<
            typename hpx::traits::range_traits<Rng>::iterator_type, FwdIter>
        invoke_default(Rng&& rng, FwdIter dest)
        {
            using copy_iter_t = hpx::parallel::detail::copy_iter<
                typename hpx::traits::range_traits<Rng>::iterator_type,
                FwdIter>;

            return hpx::parallel::detail::transfer<copy_iter_t>(
                hpx::execution::seq, hpx::util::begin(rng), hpx::util::end(rng),
                dest);
        }
    } copy{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::copy_n
    HPX_CXX_CORE_EXPORT inline constexpr struct copy_n_t final
      : hpx::detail::tag_dispatch<copy_n_t,
            hpx::detail::tag_parallel_algorithm<copy_n_t>>
    {
        template <typename ExPolicy, typename FwdIter1, typename Size,
            typename FwdIter2>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            ranges::copy_n_result<FwdIter1, FwdIter2>>
        invoke_default(
            ExPolicy&& policy, FwdIter1 first, Size count, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2> ||
                    hpx::is_sequenced_execution_policy_v<ExPolicy>,
                "Requires at least forward iterator or sequential execution.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                return hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    ranges::copy_n_result<FwdIter1, FwdIter2>>::
                    get(ranges::copy_n_result<FwdIter1, FwdIter2>{
                        HPX_MOVE(first), HPX_MOVE(dest)});
            }

            return hpx::parallel::detail::copy_n<
                ranges::copy_n_result<FwdIter1, FwdIter2>>()
                .call(HPX_FORWARD(ExPolicy, policy), first,
                    static_cast<std::size_t>(count), dest);
        }

        template <typename FwdIter1, typename Size, typename FwdIter2>
        // clang-format off
            requires(hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                std::is_integral_v<Size>)
        // clang-format on
        static ranges::copy_n_result<FwdIter1, FwdIter2> invoke_default(
            FwdIter1 first, Size count, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");
            static_assert(std::output_iterator<FwdIter2,
                              hpx::traits::iter_value_t<FwdIter1>>,
                "Requires at least output iterator.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                return ranges::copy_n_result<FwdIter1, FwdIter2>{
                    HPX_MOVE(first), HPX_MOVE(dest)};
            }

            return hpx::parallel::detail::copy_n<
                ranges::copy_n_result<FwdIter1, FwdIter2>>()
                .call(hpx::execution::seq, first,
                    static_cast<std::size_t>(count), dest);
        }
    } copy_n{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::copy_if
    HPX_CXX_CORE_EXPORT inline constexpr struct copy_if_t final
      : hpx::detail::tag_dispatch<copy_if_t,
            hpx::detail::tag_parallel_algorithm<copy_if_t>>
    {
        template <typename ExPolicy, typename FwdIter1, typename Sent1,
            typename FwdIter, typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent1, FwdIter1> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<ExPolicy, Pred,
                    hpx::parallel::traits::projected<Proj, FwdIter1>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            ranges::copy_if_result<FwdIter1, FwdIter>>
        invoke_default(ExPolicy&& policy, FwdIter1 iter, Sent1 sent,
            FwdIter dest, Pred pred, Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter> ||
                    (hpx::is_sequenced_execution_policy_v<ExPolicy> &&
                        std::output_iterator<FwdIter,
                            hpx::traits::iter_value_t<FwdIter1>>),
                "Requires at least forward iterator or sequential execution.");

            return hpx::parallel::detail::copy_if<
                hpx::parallel::util::in_out_result<FwdIter1, FwdIter>>()
                .call(HPX_FORWARD(ExPolicy, policy), iter, sent, dest,
                    HPX_MOVE(pred), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng, typename FwdIter,
            typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<ExPolicy, Pred,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            ranges::copy_if_result<
                typename hpx::traits::range_traits<Rng>::iterator_type,
                FwdIter>>
        invoke_default(ExPolicy&& policy, Rng&& rng, FwdIter dest, Pred pred,
            Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter> ||
                    (hpx::is_sequenced_execution_policy_v<ExPolicy> &&
                        std::output_iterator<FwdIter,
                            hpx::traits::iter_value_t<
                                std::ranges::iterator_t<Rng>>>),
                "Requires at least forward iterator or sequential execution.");

            return hpx::parallel::detail::copy_if<
                hpx::parallel::util::in_out_result<
                    typename hpx::traits::range_traits<Rng>::iterator_type,
                    FwdIter>>()
                .call(HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                    hpx::util::end(rng), dest, HPX_MOVE(pred), HPX_MOVE(proj));
        }

        template <typename FwdIter1, typename Sent1, typename FwdIter,
            typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent1, FwdIter1> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj, FwdIter1>
                >
            )
        // clang-format on
        static ranges::copy_if_result<FwdIter1, FwdIter> invoke_default(
            FwdIter1 iter, Sent1 sent, FwdIter dest, Pred pred,
            Proj proj = Proj())
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::output_iterator<FwdIter,
                              hpx::traits::iter_value_t<FwdIter1>>,
                "Required at least output iterator.");

            return hpx::parallel::detail::copy_if<
                hpx::parallel::util::in_out_result<FwdIter1, FwdIter>>()
                .call(hpx::execution::seq, iter, sent, dest, HPX_MOVE(pred),
                    HPX_MOVE(proj));
        }

        template <typename Rng, typename FwdIter, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires (
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected_range<Proj, Rng>
                >
            )
        // clang-format on
        static ranges::copy_if_result<
            typename hpx::traits::range_traits<Rng>::iterator_type, FwdIter>
        invoke_default(Rng&& rng, FwdIter dest, Pred pred, Proj proj = Proj())
        {
            static_assert(
                std::output_iterator<FwdIter,
                    hpx::traits::iter_value_t<std::ranges::iterator_t<Rng>>>,
                "Required at least output iterator.");

            return hpx::parallel::detail::copy_if<
                hpx::parallel::util::in_out_result<
                    typename hpx::traits::range_traits<Rng>::iterator_type,
                    FwdIter>>()
                .call(hpx::execution::seq, hpx::util::begin(rng),
                    hpx::util::end(rng), dest, HPX_MOVE(pred), HPX_MOVE(proj));
        }
    } copy_if{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
