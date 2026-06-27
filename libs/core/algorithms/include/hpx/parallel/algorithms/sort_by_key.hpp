//  Copyright (c) 2016-2023 Hartmut Kaiser
//  Copyright (c) 2016 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/sort_by_key.hpp
/// \page hpx::experimental::sort_by_key
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx { namespace experimental {
    // clang-format off
    /// Sorts one range of data using keys supplied in another range.
    /// The key elements in the range [key_first, key_last) are sorted in
    /// ascending order with the corresponding elements in the value range
    /// moved to follow the sorted order.
    /// The algorithm is not stable, the order of equal elements is not guaranteed
    /// to be preserved.
    /// The function uses the given comparison function object comp (defaults
    /// to using operator<()). Executed according to the policy.
    ///
    /// \note   Complexity: O(N log(N)), where N = std::distance(first, last)
    ///                     comparisons.
    ///
    /// A sequence is sorted with respect to a comparator \a comp
    /// if for every iterator i pointing to the sequence and
    /// every non-negative integer n such that i + n is a valid iterator
    /// pointing to an element of the sequence, and
    /// INVOKE(comp, *(i + n), *i) == false.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam KeyIter     The type of the key iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam ValueIter   The type of the value iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Compare     The type of the function/function object to use
    ///                     (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param key_first    Refers to the beginning of the sequence of key
    ///                     elements the algorithm will be applied to.
    /// \param key_last     Refers to the end of the sequence of key elements the
    ///                     algorithm will be applied to.
    /// \param value_first  Refers to the beginning of the sequence of value
    ///                     elements the algorithm will be applied to, the range
    ///                     of elements must match [key_first, key_last)
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that comp
    ///                     will not apply any non-constant function through the
    ///                     dereferenced iterator.
    ///
    /// \a comp has to induce a strict weak ordering on the values.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a sort_by_key algorithm returns a
    ///           \a hpx::future<sort_by_key_result<KeyIter,ValueIter>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a
    ///           \a sort_by_key_result<KeyIter,ValueIter> otherwise.
    ///           The algorithm returns a pair holding an iterator pointing to
    ///           the first element after the last element in the input key
    ///           sequence and an iterator pointing to the first element after
    ///           the last element in the input value sequence.
    template <typename ExPolicy, typename KeyIter, typename ValueIter,
        typename Compare = detail::less>
    util::detail::algorithm_result_t<ExPolicy,
        sort_by_key_result<KeyIter, ValueIter>>
    sort_by_key(ExPolicy&& policy, KeyIter key_first, KeyIter key_last,
        ValueIter value_first, Compare&& comp = Compare());
    // clang-format on
}}    // namespace hpx::experimental

#else

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/sort.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    HPX_CXX_CORE_EXPORT template <typename KeyIter, typename ValueIter>
    using sort_by_key_result = std::pair<KeyIter, ValueIter>;

    ///////////////////////////////////////////////////////////////////////////
    // sort
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT struct extract_key
        {
            template <typename Tuple>
            auto operator()(Tuple&& t) const
                -> decltype(hpx::get<0>(HPX_FORWARD(Tuple, t)))
            {
                return hpx::get<0>(HPX_FORWARD(Tuple, t));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx::experimental {

    HPX_CXX_CORE_EXPORT template <typename KeyIter, typename ValueIter>
    using sort_by_key_result = std::pair<KeyIter, ValueIter>;

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename KeyIter,
        typename ValueIter, typename Compare = hpx::parallel::detail::less>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        sort_by_key_result<KeyIter, ValueIter>>
    sort_by_key(ExPolicy&& policy, KeyIter key_first, KeyIter key_last,
        ValueIter value_first, Compare comp = Compare())
    {
#if !defined(HPX_HAVE_TUPLE_RVALUE_SWAP)
        static_assert(hpx::util::detail::always_false<KeyIter>::value,
            "sort_by_key requires HPX_HAVE_TUPLE_RVALUE_SWAP. "
            "Enable tuple rvalue swap support to use this algorithm.");
#else
        static_assert(std::random_access_iterator<KeyIter>,
            "Requires a random access iterator.");
        static_assert(std::random_access_iterator<ValueIter>,
            "Requires a random access iterator.");

        ValueIter value_last = value_first;
        std::advance(
            value_last, hpx::parallel::detail::distance(key_first, key_last));

        using iterator_type = hpx::util::zip_iterator<KeyIter, ValueIter>;

        return hpx::parallel::detail::get_iter_pair<iterator_type>(
            hpx::parallel::detail::sort<iterator_type>().call(
                HPX_FORWARD(ExPolicy, policy),
                hpx::util::zip_iterator(key_first, value_first),
                hpx::util::zip_iterator(key_last, value_last), HPX_MOVE(comp),
                hpx::parallel::detail::extract_key()));
#endif
    }
}    // namespace hpx::experimental

namespace hpx::parallel {

    template <typename ExPolicy, typename KeyIter, typename ValueIter,
        typename Compare = detail::less>
    HPX_DEPRECATED_V(1, 9,
        "hpx::parallel::sort_by_key is deprecated. Please use "
        "hpx::experimental::sort_by_key instead.")
    constexpr decltype(auto) sort_by_key(ExPolicy&& policy, KeyIter key_first,
        KeyIter key_last, ValueIter value_first, Compare&& comp = Compare())
    {
        return hpx::experimental::sort_by_key(
            policy, key_first, key_last, value_first, comp);
    }
}    // namespace hpx::parallel

#endif
