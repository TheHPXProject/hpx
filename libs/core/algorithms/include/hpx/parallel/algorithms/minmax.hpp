//  Copyright (c) 2014-2026 Hartmut Kaiser
//  Copyright (c) 2024-2026 The STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// make inspect happy: hpxinspect:nominmax

/// \file parallel/algorithms/minmax.hpp
/// \page hpx::min_element, hpx::max_element, hpx::minmax_element
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {

    /////////////////////////////////////////////////////////////////////////////
    /// Finds the smallest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced).
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
    ///
    /// The comparisons in the parallel \a min_element algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a min_element algorithm returns \a FwdIter.
    ///           The \a min_element algorithm returns the iterator to the
    ///           smallest element in the range [first, last). If several
    ///           elements in the range are equivalent to the smallest element,
    ///           returns the iterator to the first such element. Returns last
    ///           if the range is empty.
    ///
    template <typename FwdIter, typename F = hpx::parallel::detail::less>
    FwdIter min_element(FwdIter first, FwdIter last, F&& f);

    /////////////////////////////////////////////////////////////////////////////
    /// Finds the smallest element in the range [first, last) using the given
    /// comparison function \a f. Executed according to the policy.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a min_element requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
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
    template <typename ExPolicy, typename FwdIter,
        typename F = hpx::parallel::detail::less>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    min_element(ExPolicy&& policy, FwdIter first, FwdIter last, F&& f);

    /////////////////////////////////////////////////////////////////////////////
    /// Finds the largest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced).
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
    ///
    /// The comparisons in the parallel \a min_element algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a max_element algorithm returns \a FwdIter.
    ///           The \a max_element algorithm returns the iterator to the
    ///           smallest element in the range [first, last). If several
    ///           elements in the range are equivalent to the smallest element,
    ///           returns the iterator to the first such element. Returns last
    ///           if the range is empty.
    ///
    template <typename FwdIter, typename F = hpx::parallel::detail::less>
    FwdIter max_element(FwdIter first, FwdIter last, F&& f);

    /////////////////////////////////////////////////////////////////////////////
    /// Removes all elements satisfying specific criteria from the range
    /// Finds the largest element in the range [first, last) using the given
    /// comparison function \a f. Executed according to the policy.
    ///
    /// \note   Complexity: Exactly \a max(N-1, 0) comparisons, where
    ///                     N = std::distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a max_element requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
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
    template <typename ExPolicy, typename FwdIter,
        typename F = hpx::parallel::detail::less>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        FwdIter>::type
    max_element(ExPolicy&& policy, FwdIter first, FwdIter last, F&& f);

    /////////////////////////////////////////////////////////////////////////////
    /// Finds the largest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: At most \a max(floor(3/2*(N-1)), 0) applications of
    ///                     the predicate, where N = std::distance(first, last).
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced).
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
    ///
    /// The comparisons in the parallel \a minmax_element algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a minmax_element algorithm returns a
    ///           \a minmax_element_result<FwdIter>
    ///           The \a minmax_element algorithm returns a pair consisting of
    ///           an iterator to the smallest element as the min element and
    ///           an iterator to the largest element as the max element. Returns
    ///           \a minmax_element_result<FwdIter>{first,first} if the range is empty. If
    ///           several elements are equivalent to the smallest element, the
    ///           iterator to the first such element is returned. If several
    ///           elements are equivalent to the largest element, the iterator
    ///           to the last such element is returned.
    ///
    template <typename FwdIter, typename F = hpx::parallel::detail::less>
    minmax_element_result<FwdIter> minmax_element(
        FwdIter first, FwdIter last, F&& f);

    /////////////////////////////////////////////////////////////////////////////
    /// Finds the largest element in the range [first, last) using the given
    /// comparison function \a f.
    ///
    /// \note   Complexity: At most \a max(floor(3/2*(N-1)), 0) applications of
    ///                     the predicate, where N = std::distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a minmax_element requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
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
    ///           \a hpx::future<minmax_element_result<FwdIter>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy
    ///           and returns \a minmax_element_result<FwdIter>
    ///           otherwise.
    ///           The \a minmax_element algorithm returns a pair consisting of
    ///           an iterator to the smallest element as the min element and
    ///           an iterator to the largest element as the max element. Returns
    ///           \a std::make_pair(first,first) if the range is empty. If
    ///           several elements are equivalent to the smallest element, the
    ///           iterator to the first such element is returned. If several
    ///           elements are equivalent to the largest element, the iterator
    ///           to the last such element is returned.
    ///
    template <typename ExPolicy, typename FwdIter,
        typename F = hpx::parallel::detail::less>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        minmax_element_result<FwdIter>>
    minmax_element(ExPolicy&& policy, FwdIter first, FwdIter last, F&& f);

}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/is_value_proxy.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    template <typename T>
    using minmax_element_result = hpx::parallel::util::min_max_result<T>;

    ///////////////////////////////////////////////////////////////////////////
    // min_element
    namespace detail {

        /// \cond NOINTERNAL
        template <typename ExPolicy, typename FwdIter, typename F,
            typename Proj>
        constexpr FwdIter sequential_min_element(ExPolicy&&, FwdIter it,
            std::size_t count, F const& f, Proj const& proj)
        {
            if (count == 0 || count == 1)
                return it;

            auto smallest = it;

            using element_type =
                hpx::traits::proxy_value_t<std::decay_t<std::invoke_result_t<
                    Proj, hpx::traits::iter_reference_t<FwdIter>>>>;

            element_type value = HPX_INVOKE(proj, *smallest);
            util::const_loop_n<std::decay_t<ExPolicy>>(
                ++it, count - 1, [&](FwdIter const& curr) -> void {
                    element_type curr_value = HPX_INVOKE(proj, *curr);
                    if (HPX_INVOKE(f, curr_value, value))
                    {
                        smallest = curr;
                        value = HPX_MOVE(curr_value);
                    }
                });

            return smallest;
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Iter>
        struct min_element : public algorithm<min_element<Iter>, Iter>
        {
            // this has to be a member of the algorithm type as we access this
            // generically from the segmented algorithms
            template <typename ExPolicy, typename FwdIter, typename F,
                typename Proj>
            static constexpr hpx::traits::proxy_value_t<
                typename std::iterator_traits<FwdIter>::value_type>
            sequential_minmax_element_ind(ExPolicy&&, FwdIter it,
                std::size_t count, F const& f, Proj const& proj)
            {
                HPX_ASSERT(count != 0);

                if (count == 1)
                    return *it;

                auto smallest = *it;

                using element_type = hpx::traits::proxy_value_t<
                    std::decay_t<std::invoke_result_t<Proj,
                        hpx::traits::iter_reference_t<Iter>>>>;

                element_type value = HPX_INVOKE(proj, *smallest);
                util::const_loop_n<std::decay_t<ExPolicy>>(
                    ++it, count - 1, [&](FwdIter const& curr) -> void {
                        element_type curr_value = HPX_INVOKE(proj, **curr);
                        if (HPX_INVOKE(f, curr_value, value))
                        {
                            smallest = *curr;
                            value = HPX_MOVE(curr_value);
                        }
                    });

                return smallest;
            }

            constexpr min_element() noexcept
              : algorithm<min_element, Iter>("min_element")
            {
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename F, typename Proj>
            static constexpr FwdIter sequential(
                ExPolicy&& policy, FwdIter first, Sent last, F&& f, Proj&& proj)
            {
                if (first == last)
                    return first;

                auto smallest = first;

                using element_type = hpx::traits::proxy_value_t<
                    std::decay_t<std::invoke_result_t<Proj,
                        hpx::traits::iter_reference_t<FwdIter>>>>;

                element_type value = HPX_INVOKE(proj, *smallest);
                util::const_loop(HPX_FORWARD(ExPolicy, policy), ++first, last,
                    [&](FwdIter const& curr) -> void {
                        element_type curr_value = HPX_INVOKE(proj, *curr);
                        if (HPX_INVOKE(f, curr_value, value))
                        {
                            smallest = curr;
                            value = HPX_MOVE(curr_value);
                        }
                    });

                return smallest;
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename F, typename Proj>
            static decltype(auto) parallel(
                ExPolicy&& policy, FwdIter first, Sent last, F&& f, Proj&& proj)
            {
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return util::detail::algorithm_result<ExPolicy,
                            FwdIter>::get(HPX_MOVE(first));
                    }
                }

                auto f1 = [f, proj, policy](
                              FwdIter it, std::size_t part_count) -> FwdIter {
                    return sequential_min_element(
                        policy, it, part_count, f, proj);
                };

                auto f2 = [policy, first, f = HPX_FORWARD(F, f),
                              proj = HPX_FORWARD(Proj, proj)](
                              auto&& positions) -> FwdIter {
                    if constexpr (has_scheduler_executor)
                    {
                        if (positions.size() == 0)
                        {
                            return first;
                        }
                    }
                    else
                    {
                        HPX_UNUSED(first);
                    }

                    return min_element::sequential_minmax_element_ind(
                        policy, positions.begin(), positions.size(), f, proj);
                };

                return util::partitioner<ExPolicy, FwdIter, FwdIter>::call(
                    HPX_FORWARD(ExPolicy, policy), first,
                    detail::distance(first, last), HPX_MOVE(f1),
                    hpx::unwrapping(HPX_MOVE(f2)));
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // max_element
    namespace detail {

        /// \cond NOINTERNAL
        template <typename ExPolicy, typename FwdIter, typename F,
            typename Proj>
        constexpr FwdIter sequential_max_element(ExPolicy&&, FwdIter it,
            std::size_t count, F const& f, Proj const& proj)
        {
            if (count == 0 || count == 1)
                return it;

            auto largest = it;

            using element_type =
                hpx::traits::proxy_value_t<std::decay_t<std::invoke_result_t<
                    Proj, hpx::traits::iter_reference_t<FwdIter>>>>;

            element_type value = HPX_INVOKE(proj, *largest);
            util::const_loop_n<std::decay_t<ExPolicy>>(
                ++it, count - 1, [&](FwdIter const& curr) -> void {
                    element_type curr_value = HPX_INVOKE(proj, *curr);
                    if (!HPX_INVOKE(f, curr_value, value))
                    {
                        largest = curr;
                        value = HPX_MOVE(curr_value);
                    }
                });

            return largest;
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Iter>
        struct max_element : public algorithm<max_element<Iter>, Iter>
        {
            // this has to be a member of the algorithm type as we access this
            // generically from the segmented algorithms
            template <typename ExPolicy, typename FwdIter, typename F,
                typename Proj>
            static constexpr hpx::traits::proxy_value_t<
                typename std::iterator_traits<FwdIter>::value_type>
            sequential_minmax_element_ind(ExPolicy&&, FwdIter it,
                std::size_t count, F const& f, Proj const& proj)
            {
                HPX_ASSERT(count != 0);

                if (count == 1)
                    return *it;

                auto largest = *it;

                using element_type = hpx::traits::proxy_value_t<
                    std::decay_t<std::invoke_result_t<Proj,
                        hpx::traits::iter_reference_t<Iter>>>>;

                element_type value = HPX_INVOKE(proj, *largest);
                util::const_loop_n<std::decay_t<ExPolicy>>(
                    ++it, count - 1, [&](FwdIter const& curr) -> void {
                        element_type curr_value = HPX_INVOKE(proj, **curr);
                        if (!HPX_INVOKE(f, curr_value, value))
                        {
                            largest = *curr;
                            value = HPX_MOVE(curr_value);
                        }
                    });

                return largest;
            }

            constexpr max_element() noexcept
              : algorithm<max_element, Iter>("max_element")
            {
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename F, typename Proj>
            static constexpr FwdIter sequential(
                ExPolicy&& policy, FwdIter first, Sent last, F&& f, Proj&& proj)
            {
                if (first == last)
                    return first;

                auto largest = first;

                using element_type = hpx::traits::proxy_value_t<
                    std::decay_t<std::invoke_result_t<Proj,
                        hpx::traits::iter_reference_t<FwdIter>>>>;

                element_type value = HPX_INVOKE(proj, *largest);
                util::const_loop(HPX_FORWARD(ExPolicy, policy), ++first, last,
                    [&](FwdIter const& curr) -> void {
                        element_type curr_value = HPX_INVOKE(proj, *curr);
                        if (!HPX_INVOKE(f, curr_value, value))
                        {
                            largest = curr;
                            value = HPX_MOVE(curr_value);
                        }
                    });

                return largest;
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename F, typename Proj>
            static decltype(auto) parallel(
                ExPolicy&& policy, FwdIter first, Sent last, F&& f, Proj&& proj)
            {
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return util::detail::algorithm_result<ExPolicy,
                            FwdIter>::get(HPX_MOVE(first));
                    }
                }

                auto f1 = [f, proj, policy](
                              FwdIter it, std::size_t part_count) -> FwdIter {
                    return sequential_max_element(
                        policy, it, part_count, f, proj);
                };

                auto f2 = [policy, first, f = HPX_FORWARD(F, f),
                              proj = HPX_FORWARD(Proj, proj)](
                              auto&& positions) -> FwdIter {
                    if constexpr (has_scheduler_executor)
                    {
                        if (positions.size() == 0)
                        {
                            return first;
                        }
                    }
                    else
                    {
                        HPX_UNUSED(first);
                    }

                    return max_element::sequential_minmax_element_ind(
                        policy, positions.begin(), positions.size(), f, proj);
                };

                return util::partitioner<ExPolicy, FwdIter, FwdIter>::call(
                    HPX_FORWARD(ExPolicy, policy), first,
                    detail::distance(first, last), HPX_MOVE(f1),
                    hpx::unwrapping(HPX_MOVE(f2)));
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // minmax_element
    namespace detail {

        /// \cond NOINTERNAL
        template <typename ExPolicy, typename FwdIter, typename F,
            typename Proj>
        minmax_element_result<FwdIter> sequential_minmax_element(ExPolicy&&,
            FwdIter it, std::size_t count, F const& f, Proj const& proj)
        {
            minmax_element_result<FwdIter> result = {it, it};

            if (count == 0 || count == 1)
                return result;

            using element_type =
                hpx::traits::proxy_value_t<std::decay_t<std::invoke_result_t<
                    Proj, hpx::traits::iter_reference_t<FwdIter>>>>;

            element_type min_value = HPX_INVOKE(proj, *it);
            element_type max_value = min_value;
            util::const_loop_n<std::decay_t<ExPolicy>>(
                ++it, count - 1, [&](FwdIter const& curr) -> void {
                    element_type curr_value = HPX_INVOKE(proj, *curr);
                    if (HPX_INVOKE(f, curr_value, min_value))
                    {
                        result.min = curr;
                        min_value = curr_value;
                    }

                    if (!HPX_INVOKE(f, curr_value, max_value))
                    {
                        result.max = curr;
                        max_value = HPX_MOVE(curr_value);
                    }
                });

            return result;
        }

        template <typename Iter>
        struct minmax_element
          : public algorithm<minmax_element<Iter>, minmax_element_result<Iter>>
        {
            // this has to be a member of the algorithm type as we access this
            // generically from the segmented algorithms
            template <typename ExPolicy, typename PairIter, typename F,
                typename Proj>
            static constexpr hpx::traits::proxy_value_t<
                typename std::iterator_traits<PairIter>::value_type>
            sequential_minmax_element_ind(ExPolicy&&, PairIter it,
                std::size_t count, F const& f, Proj const& proj)
            {
                HPX_ASSERT(count != 0);

                if (count == 1)
                    return *it;

                auto result = *it;

                using element_type = hpx::traits::proxy_value_t<
                    std::decay_t<std::invoke_result_t<Proj,
                        hpx::traits::iter_reference_t<Iter>>>>;

                element_type min_value = HPX_INVOKE(proj, *result.min);
                element_type max_value = HPX_INVOKE(proj, *result.max);
                util::const_loop_n<std::decay_t<ExPolicy>>(
                    ++it, count - 1, [&](PairIter const& curr) -> void {
                        element_type curr_min_value =
                            HPX_INVOKE(proj, *curr->min);
                        if (HPX_INVOKE(f, curr_min_value, min_value))
                        {
                            result.min = curr->min;
                            min_value = HPX_MOVE(curr_min_value);
                        }

                        element_type curr_max_value =
                            HPX_INVOKE(proj, *curr->max);
                        if (!HPX_INVOKE(f, curr_max_value, max_value))
                        {
                            result.max = curr->max;
                            max_value = HPX_MOVE(curr_max_value);
                        }
                    });

                return result;
            }

            constexpr minmax_element() noexcept
              : algorithm<minmax_element, minmax_element_result<Iter>>(
                    "minmax_element")
            {
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename F, typename Proj>
            static constexpr minmax_element_result<FwdIter> sequential(
                ExPolicy&& policy, FwdIter first, Sent last, F&& f, Proj&& proj)
            {
                auto min = first, max = first;

                // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
                if (first == last || ++first == last)
                {
                    return minmax_element_result<FwdIter>{min, max};
                }

                using element_type = hpx::traits::proxy_value_t<
                    std::decay_t<std::invoke_result_t<Proj,
                        hpx::traits::iter_reference_t<FwdIter>>>>;

                element_type min_value = HPX_INVOKE(proj, *min);
                element_type max_value = HPX_INVOKE(proj, *max);
                util::const_loop(HPX_FORWARD(ExPolicy, policy), first, last,
                    [&](FwdIter const& curr) -> void {
                        element_type curr_value = HPX_INVOKE(proj, *curr);
                        if (HPX_INVOKE(f, curr_value, min_value))
                        {
                            min = curr;
                            min_value = curr_value;
                        }

                        if (!HPX_INVOKE(f, curr_value, max_value))
                        {
                            max = curr;
                            max_value = HPX_MOVE(curr_value);
                        }
                    });

                return minmax_element_result<FwdIter>{min, max};
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename F, typename Proj>
            static decltype(auto) parallel(
                ExPolicy&& policy, FwdIter first, Sent last, F&& f, Proj&& proj)
            {
                using result_type = minmax_element_result<FwdIter>;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                result_type result = {first, first};

                if constexpr (!has_scheduler_executor)
                {
                    // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
                    if (first == last || ++first == last)
                    {
                        return util::detail::algorithm_result<ExPolicy,
                            result_type>::get(HPX_MOVE(result));
                    }
                }

                auto f1 = [f, proj, policy](FwdIter it, std::size_t part_count)
                    -> minmax_element_result<FwdIter> {
                    return sequential_minmax_element(
                        policy, it, part_count, f, proj);
                };

                auto f2 = [policy, first, f = HPX_FORWARD(F, f),
                              proj = HPX_FORWARD(Proj, proj)](
                              auto&& positions) -> result_type {
                    if constexpr (has_scheduler_executor)
                    {
                        if (positions.size() == 0)
                        {
                            return result_type{first, first};
                        }
                    }
                    else
                    {
                        HPX_UNUSED(first);
                    }

                    return minmax_element::sequential_minmax_element_ind(
                        policy, positions.begin(), positions.size(), f, proj);
                };

                return util::partitioner<ExPolicy, result_type,
                    result_type>::call(HPX_FORWARD(ExPolicy, policy),
                    result.min, detail::distance(result.min, last),
                    HPX_MOVE(f1), hpx::unwrapping(HPX_MOVE(f2)));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    template <typename T>
    using minmax_element_result = hpx::parallel::util::min_max_result<T>;

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::min_element
    HPX_CXX_CORE_EXPORT inline constexpr struct min_element_t final
      : hpx::detail::tag_parallel_algorithm<min_element_t>
    {
        template <typename FwdIter, typename F = hpx::parallel::detail::less>
        // clang-format off
            requires(hpx::traits::is_iterator_v<FwdIter>)
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            hpx::min_element_t, FwdIter first, FwdIter last, F f = F())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::min_element<FwdIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter,
            typename F = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::min_element_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, F f = F())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::min_element<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                hpx::identity_v);
        }
    } min_element{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::max_element
    HPX_CXX_CORE_EXPORT inline constexpr struct max_element_t final
      : hpx::detail::tag_parallel_algorithm<max_element_t>
    {
        template <typename FwdIter, typename F = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            hpx::max_element_t, FwdIter first, FwdIter last, F f = F())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::max_element<FwdIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter,
            typename F = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::max_element_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, F f = F())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::max_element<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                hpx::identity_v);
        }
    } max_element{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::minmax_element
    HPX_CXX_CORE_EXPORT inline constexpr struct minmax_element_t final
      : hpx::detail::tag_parallel_algorithm<minmax_element_t>
    {
        template <typename FwdIter, typename F = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend minmax_element_result<FwdIter> tag_fallback_invoke(
            hpx::minmax_element_t, FwdIter first, FwdIter last, F f = F())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::minmax_element<FwdIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter,
            typename F = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::minmax_element_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, F f = F())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::minmax_element<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                hpx::identity_v);
        }
    } minmax_element{};
}    // namespace hpx

#endif    // DOXYGEN
