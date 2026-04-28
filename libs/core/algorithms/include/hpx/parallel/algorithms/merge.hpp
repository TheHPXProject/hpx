//  Copyright (c) 2017 Taeguk Kwon
//  Copyright (c) 2017-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::merge, hpx::inplace_merge
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Merges two sorted ranges [first1, last1) and [first2, last2)
    /// into one sorted range beginning at \a dest. The order of
    /// equivalent elements in each of the original two ranges is preserved.
    /// For equivalent elements in the original two ranges, the elements from
    /// the first range precede the elements from the second range.
    /// The destination range cannot overlap with either of the input ranges.
    /// Executed according to the policy.
    ///
    /// \note   Complexity: Performs
    ///         O(std::distance(first1, last1) + std::distance(first2, last2))
    ///         applications of the comparison \a comp and each projection.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam RandIter1   The type of the source iterators used (deduced)
    ///                     representing the first sorted range.
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam RandIter2   The type of the source iterators used (deduced)
    ///                     representing the second sorted range.
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam RandIter3   The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a merge requires \a Comp to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::less<>
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first1       Refers to the beginning of the first range of elements
    ///                     the algorithm will be applied to.
    /// \param last1        Refers to the end of the first range of elements
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the second range of elements
    ///                     the algorithm will be applied to.
    /// \param last2        Refers to the end of the second range of elements
    ///                     the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param comp         \a comp is a callable object which returns true if
    ///                     the first argument is less than the second,
    ///                     and false otherwise. The signature of this
    ///                     comparison should be equivalent to:
    ///                     \code
    ///                     bool comp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such that
    ///                     objects of types \a RandIter1 and \a RandIter2 can be
    ///                     dereferenced and then implicitly converted to
    ///                     both \a Type1 and \a Type2
    ///
    /// The assignments in the parallel \a merge algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a merge algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a merge algorithm returns a
    ///           \a hpx::future<RandIter3> >
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns
    ///           \a RandIter3 otherwise.
    ///           The \a merge algorithm returns the destination iterator to
    ///           the end of the \a dest range.
    ///
    template <typename ExPolicy, typename RandIter1, typename RandIter2,
        typename RandIter3, typename Comp = hpx::parallel::detail::less>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, RandIter3>
    merge(ExPolicy&& policy, RandIter1 first1, RandIter1 last1,
        RandIter2 first2, RandIter2 last2, RandIter3 dest, Comp&& comp = Comp());

    /// Merges two sorted ranges [first1, last1) and [first2, last2)
    /// into one sorted range beginning at \a dest. The order of
    /// equivalent elements in each of the original two ranges is preserved.
    /// For equivalent elements in the original two ranges, the elements from
    /// the first range precede the elements from the second range.
    /// The destination range cannot overlap with either of the input ranges.
    ///
    /// \note   Complexity: Performs
    ///         O(std::distance(first1, last1) + std::distance(first2, last2))
    ///         applications of the comparison \a comp and each projection.
    ///
    /// \tparam RandIter1   The type of the source iterators used (deduced)
    ///                     representing the first sorted range.
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam RandIter2   The type of the source iterators used (deduced)
    ///                     representing the second sorted range.
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam RandIter3   The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a merge requires \a Comp to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::less<>
    ///
    /// \param first1       Refers to the beginning of the first range of elements
    ///                     the algorithm will be applied to.
    /// \param last1        Refers to the end of the first range of elements
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the second range of elements
    ///                     the algorithm will be applied to.
    /// \param last2        Refers to the end of the second range of elements
    ///                     the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param comp         \a comp is a callable object which returns true if
    ///                     the first argument is less than the second,
    ///                     and false otherwise. The signature of this
    ///                     comparison should be equivalent to:
    ///                     \code
    ///                     bool comp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such that
    ///                     objects of types \a RandIter1 and \a RandIter2 can be
    ///                     dereferenced and then implicitly converted to
    ///                     both \a Type1 and \a Type2
    ///
    /// \returns  The \a merge algorithm returns a \a RandIter3.
    ///           The \a merge algorithm returns the destination iterator to
    ///           the end of the \a dest range.
    ///
    template <typename RandIter1, typename RandIter2,
        typename RandIter3, typename Comp = hpx::parallel::detail::less>
    RandIter3 merge(RandIter1 first1, RandIter1 last1,
        RandIter2 first2, RandIter2 last2, RandIter3 dest, Comp&& comp = Comp());

    /// Merges two consecutive sorted ranges [first, middle) and
    /// [middle, last) into one sorted range [first, last). The order of
    /// equivalent elements in each of the original two ranges is preserved.
    /// For equivalent elements in the original two ranges, the elements from
    /// the first range precede the elements from the second range. Executed
    /// according to the policy.
    ///
    ///
    /// \note   Complexity: Performs O(std::distance(first, last))
    ///         applications of the comparison \a comp and each projection.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam RandIter    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a inplace_merge requires \a Comp
    ///                     to meet the requirements of \a CopyConstructible.
    ///                     This defaults to std::less<>
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the first sorted range
    ///                     the algorithm will be applied to.
    /// \param middle       Refers to the end of the first sorted range and
    ///                     the beginning of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param comp         \a comp is a callable object which returns true if
    ///                     the first argument is less than the second,
    ///                     and false otherwise. The signature of this
    ///                     comparison should be equivalent to:
    ///                     \code
    ///                     bool comp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be
    ///                     such that objects of types \a RandIter can be
    ///                     dereferenced and then implicitly converted to both
    ///                     \a Type1 and \a Type2
    ///
    /// The assignments in the parallel \a inplace_merge algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a inplace_merge algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a inplace_merge algorithm returns a
    ///           \a hpx::future<void> if the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy
    ///           and returns void otherwise.
    ///           The \a inplace_merge algorithm returns
    ///           the source iterator \a last.
    ///
    template <typename ExPolicy, typename RandIter,
        typename Comp = hpx::parallel::detail::less>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy>
    inplace_merge(ExPolicy&& policy, RandIter first, RandIter middle,
        RandIter last, Comp&& comp = Comp());

    /// Merges two consecutive sorted ranges [first, middle) and
    /// [middle, last) into one sorted range [first, last). The order of
    /// equivalent elements in each of the original two ranges is preserved.
    /// For equivalent elements in the original two ranges, the elements from
    /// the first range precede the elements from the second range.
    ///
    /// \note   Complexity: Performs O(std::distance(first, last))
    ///         applications of the comparison \a comp and each projection.
    ///
    /// \tparam RandIter    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a inplace_merge requires \a Comp
    ///                     to meet the requirements of \a CopyConstructible.
    ///                     This defaults to std::less<>
    ///
    /// \param first        Refers to the beginning of the first sorted range
    ///                     the algorithm will be applied to.
    /// \param middle       Refers to the end of the first sorted range and
    ///                     the beginning of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the second sorted range
    ///                     the algorithm will be applied to.
    /// \param comp         \a comp is a callable object which returns true if
    ///                     the first argument is less than the second,
    ///                     and false otherwise. The signature of this
    ///                     comparison should be equivalent to:
    ///                     \code
    ///                     bool comp(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be
    ///                     such that objects of types \a RandIter can be
    ///                     dereferenced and then implicitly converted to both
    ///                     \a Type1 and \a Type2
    ///
    /// \returns  The \a inplace_merge algorithm returns a \a void.
    ///           The \a inplace_merge algorithm returns
    ///           the source iterator \a last.
    ///
    template <typename RandIter, typename Comp = hpx::parallel::detail::less>
    void inplace_merge(RandIter first, RandIter middle,
        RandIter last, Comp&& comp = Comp());
    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/itt_notify.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/copy.hpp>
#include <hpx/parallel/algorithms/detail/advance_and_get_distance.hpp>
#include <hpx/parallel/algorithms/detail/advance_to_sentinel.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/rotate.hpp>
#include <hpx/parallel/algorithms/detail/upper_lower_bound.hpp>
#include <hpx/parallel/util/compare_projected.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/handle_local_exceptions.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/foreach_partitioner.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <list>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel {

    /////////////////////////////////////////////////////////////////////////////
    // merge
    namespace detail {
        /// \cond NOINTERNAL

        ///////////////////////////////////////////////////////////////////////
        enum class partition_phase : std::uint8_t
        {
            start = 1,
            end = 2
        };

        HPX_CXX_CORE_EXPORT struct lower_bound_helper;

        HPX_CXX_CORE_EXPORT struct upper_bound_helper
        {
            template <typename Iter, typename Sent, typename T, typename Comp,
                typename Proj>
            static constexpr Iter call(
                Iter first, Sent last, T const& value, Comp&& comp, Proj&& proj)
            {
                return detail::upper_bound(first, last, value,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }

            template <typename Iter, typename T, typename Comp, typename Proj>
            static constexpr Iter call_n(Iter first,
                typename std::iterator_traits<Iter>::difference_type count,
                T const& value, Comp&& comp, Proj&& proj)
            {
                return detail::upper_bound_n(first, count, value,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }

            using another_type = lower_bound_helper;
        };

        HPX_CXX_CORE_EXPORT struct lower_bound_helper
        {
            template <typename Iter, typename Sent, typename T, typename Comp,
                typename Proj>
            static constexpr Iter call(
                Iter first, Sent last, T const& value, Comp&& comp, Proj&& proj)
            {
                return detail::lower_bound(first, last, value,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }

            template <typename Iter, typename T, typename Comp, typename Proj>
            static constexpr Iter call_n(Iter first,
                typename std::iterator_traits<Iter>::difference_type count,
                T const& value, Comp&& comp, Proj&& proj)
            {
                return detail::lower_bound_n(first, count, value,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }

            using another_type = upper_bound_helper;
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        HPX_FORCEINLINE decltype(auto) init_value([[maybe_unused]] T&& val)
        {
            if constexpr (std::is_default_constructible_v<std::decay_t<T>>)
            {
                return std::decay_t<T>{};
            }
            else
            {
                return HPX_FORWARD(T, val);
            }
        }

        // sequential merge helper with projection function.
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
            typename Iter2, typename Sent2, typename OutIter, typename Comp,
            typename Proj1, typename Proj2>
        constexpr util::in_in_out_result<Iter1, Iter2, OutIter>
        sequential_merge_helper(Iter1 first1, Sent1 last1, Iter2 first2,
            Sent2 last2, OutIter dest, Comp&& comp, Proj1&& proj1,
            Proj2&& proj2)
        {
            using value_type1 =
                typename std::iterator_traits<Iter1>::value_type;
            using value_type2 =
                typename std::iterator_traits<Iter2>::value_type;

            if (first1 != last1 && first2 != last2)
            {
                auto val1 = HPX_INVOKE(proj1, *first1);
                auto val2 = init_value(HPX_INVOKE(proj2, *first2));
                while (true)
                {
                    while (first2 != last2)
                    {
                        value_type2 elem2 = *first2;
                        val2 = HPX_INVOKE(proj2, elem2);
                        if (static_cast<bool>(HPX_INVOKE(comp, val2, val1)))
                        {
                            *dest = HPX_MOVE(elem2);
                            ++dest;
                            ++first2;
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (first2 == last2)
                    {
                        break;
                    }

                    while (first1 != last1)
                    {
                        value_type1 elem1 = *first1;
                        val1 = HPX_INVOKE(proj1, elem1);
                        if (!static_cast<bool>(HPX_INVOKE(comp, val2, val1)))
                        {
                            *dest = HPX_MOVE(elem1);
                            ++dest;
                            ++first1;
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (first1 == last1)
                    {
                        break;
                    }
                }
            }

            return {first1, first2, dest};
        }

        // sequential merge helper without projection function.
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
            typename Iter2, typename Sent2, typename OutIter, typename Comp>
        constexpr util::in_in_out_result<Iter1, Iter2, OutIter>
        sequential_merge_helper(Iter1 first1, Sent1 last1, Iter2 first2,
            Sent2 last2, OutIter dest, Comp&& comp)
        {
            using value_type1 =
                typename std::iterator_traits<Iter1>::value_type;
            using value_type2 =
                typename std::iterator_traits<Iter2>::value_type;

#if !defined(HPX_MSVC)
            if constexpr (std::is_same_v<value_type1, value_type2>)
            {
                while (HPX_LIKELY(first1 != last1 && first2 != last2))
                {
                    // This code enables the compiler to generate conditional
                    // mov operations instead of branches
                    Iter1 const next1 = std::next(first1);
                    Iter2 const next2 = std::next(first2);

                    value_type1 val1 = *first1;
                    value_type2 const val2 = *first2;

                    if (static_cast<bool>(HPX_INVOKE(comp, val2, val1)))
                    {
                        val1 = HPX_MOVE(val2);
                        first2 = next2;
                    }
                    else
                    {
                        first1 = next1;
                    }

                    *dest = HPX_MOVE(val1);
                    ++dest;
                }
            }
            else
#endif
            {
                if (HPX_LIKELY(first1 != last1 && first2 != last2))
                {
                    value_type1 val1 = *first1;
                    value_type2 val2 = init_value(*first2);
                    while (true)
                    {
                        while (first2 != last2)
                        {
                            val2 = *first2;
                            if (static_cast<bool>(HPX_INVOKE(comp, val2, val1)))
                            {
                                *dest = HPX_MOVE(val2);
                                ++dest;
                                ++first2;
                            }
                            else
                            {
                                break;
                            }
                        }
                        if (HPX_UNLIKELY(first2 == last2))
                        {
                            break;
                        }

                        while (first1 != last1)
                        {
                            val1 = *first1;
                            if (!static_cast<bool>(
                                    HPX_INVOKE(comp, val2, val1)))
                            {
                                *dest = HPX_MOVE(val1);
                                ++dest;
                                ++first1;
                            }
                            else
                            {
                                break;
                            }
                        }
                        if (HPX_UNLIKELY(first1 == last1))
                        {
                            break;
                        }
                    }
                }
            }

            return {first1, first2, dest};
        }

        // sequential merge with projection function.
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
            typename Iter2, typename Sent2, typename OutIter, typename Comp,
            typename Proj1, typename Proj2>
        constexpr util::in_in_out_result<Iter1, Iter2, OutIter>
        sequential_merge(Iter1 start1, Sent1 sent1, Iter2 start2, Sent2 sent2,
            OutIter out, Comp&& comp, Proj1&& proj1, Proj2&& proj2)
        {
            if constexpr (std::random_access_iterator<Iter1> &&
                std::random_access_iterator<Iter2>)
            {
                auto first1 = hpx::util::get_unwrapped(start1);
                auto first2 = hpx::util::get_unwrapped(start2);
                auto dest = hpx::util::get_unwrapped(out);

                auto end1 = start1;
                auto const len1 = detail::advance_and_get_distance(end1, sent1);
                auto const last1 = hpx::util::get_unwrapped(end1);

                auto end2 = start2;
                auto const len2 = detail::advance_and_get_distance(end2, sent2);
                auto const last2 = hpx::util::get_unwrapped(end2);

                auto merge_result = sequential_merge_helper(first1, last1,
                    first2, last2, dest, HPX_FORWARD(Comp, comp),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));

                auto copy_result1 =
                    util::copy(merge_result.in1, last1, merge_result.out);
                [[maybe_unused]] auto copy_result2 =
                    util::copy(merge_result.in2, last2, copy_result1.out);

                if constexpr (!std::input_iterator<OutIter>)
                {
                    return {end1, end2, copy_result2.out};
                }
                else
                {
                    return {end1, end2, std::next(out, len1 + len2)};
                }
            }
            else
            {
                auto merge_result = sequential_merge_helper(start1, sent1,
                    start2, sent2, out, HPX_FORWARD(Comp, comp),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));

                auto copy_result1 =
                    util::copy(merge_result.in1, sent1, merge_result.out);
                [[maybe_unused]] auto copy_result2 =
                    util::copy(merge_result.in2, sent2, copy_result1.out);

                return {copy_result1.in, copy_result2.in, copy_result2.out};
            }
        }

        // sequential merge without projection function.
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Sent1,
            typename Iter2, typename Sent2, typename OutIter, typename Comp>
        constexpr util::in_in_out_result<Iter1, Iter2, OutIter>
        sequential_merge(Iter1 start1, Sent1 sent1, Iter2 start2, Sent2 sent2,
            OutIter out, Comp&& comp, hpx::identity, hpx::identity)
        {
            if constexpr (std::random_access_iterator<Iter1> &&
                std::random_access_iterator<Iter2>)
            {
                auto first1 = hpx::util::get_unwrapped(start1);
                auto first2 = hpx::util::get_unwrapped(start2);
                auto dest = hpx::util::get_unwrapped(out);

                auto end1 = start1;
                auto const len1 = detail::advance_and_get_distance(end1, sent1);
                auto const last1 = hpx::util::get_unwrapped(end1);

                auto end2 = start2;
                auto const len2 = detail::advance_and_get_distance(end2, sent2);
                auto const last2 = hpx::util::get_unwrapped(end2);

                auto merge_result = sequential_merge_helper(first1, last1,
                    first2, last2, dest, HPX_FORWARD(Comp, comp));

                auto copy_result1 =
                    util::copy(merge_result.in1, last1, merge_result.out);
                [[maybe_unused]] auto copy_result2 =
                    util::copy(merge_result.in2, last2, copy_result1.out);

                if constexpr (!std::input_iterator<OutIter>)
                {
                    return {end1, end2, copy_result2.out};
                }
                else
                {
                    return {end1, end2, std::next(out, len1 + len2)};
                }
            }
            else
            {
                auto merge_result = sequential_merge_helper(
                    start1, sent1, start2, sent2, out, HPX_FORWARD(Comp, comp));

                auto copy_result1 =
                    util::copy(merge_result.in1, sent1, merge_result.out);
                [[maybe_unused]] auto copy_result2 =
                    util::copy(merge_result.in2, sent2, copy_result1.out);

                return {copy_result1.in, copy_result2.in, copy_result2.out};
            }
        }

        class const_index_value_iterator
          : public hpx::util::iterator_facade<const_index_value_iterator,
                hpx::tuple<std::size_t, std::size_t>,
                std::random_access_iterator_tag,
                hpx::tuple<std::size_t, std::size_t>>
        {
        private:
            hpx::tuple<std::size_t, std::size_t> data_ = hpx::make_tuple(0, 0);

        public:
            const_index_value_iterator() = default;
            explicit constexpr const_index_value_iterator(
                std::size_t value, std::size_t start = 0) noexcept
              : data_(start, value)
            {
            }

            using use_brackets_proxy = std::false_type;

        private:
            friend class hpx::util::iterator_core_access;

            hpx::tuple<std::size_t, std::size_t> dereference() const noexcept
            {
                return data_;
            }

            void increment() noexcept
            {
                ++hpx::get<0>(data_);
            }
            void decrement() noexcept
            {
                --hpx::get<0>(data_);
            }

            void advance(std::ptrdiff_t n) noexcept
            {
                hpx::get<0>(data_) = static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(hpx::get<0>(data_)) + n);
            }

            std::ptrdiff_t distance_to(
                const_index_value_iterator const& other) const noexcept
            {
                return static_cast<std::ptrdiff_t>(hpx::get<0>(other.data_)) -
                    static_cast<std::ptrdiff_t>(hpx::get<0>(data_));
            }

            bool equal(const_index_value_iterator const& other) const noexcept
            {
                // Two iterators are equal when indices match and they share
                // the same value.
                return data_ == other.data_;
            }
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        auto get_diagonal_index(T const n)
        {
            auto diagonal_index = [n = static_cast<std::size_t>(n)](
                                      auto&& shape, std::size_t cores) {

#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
                static hpx::util::itt::event notify_event("get diagonal index");
                hpx::util::itt::mark_event e(notify_event);
#endif
                hpx::tracing::mark_event evt("get diagonal index");
                auto const shape_size = std::size(shape);

                static_assert(
                    std::random_access_iterator<const_index_value_iterator>);

                if (n == 0)
                    return hpx::util::iterator_range<
                        const_index_value_iterator>();

                if (cores == 1)
                    return hpx::util::iterator_range{
                        const_index_value_iterator(n),
                        const_index_value_iterator(n, 1)};

                std::size_t seg = (cores == 0 ? 1 : shape_size);
                seg = (std::min) (seg, n);

                std::size_t const chunk = (n + seg - 1) / seg;

                auto begin = const_index_value_iterator(chunk);
                auto end = const_index_value_iterator(chunk, seg);

                return hpx::util::iterator_range{begin, end};
            };

            return diagonal_index;
        }

        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2,
            typename Comp, typename Proj1, typename Proj2>
        std::pair<std::size_t, std::size_t> diagonal_intersection(Iter1 first1,
            std::size_t len1, Iter2 first2, std::size_t len2, std::size_t k,
            Comp&& comp, Proj1 proj1, Proj2 proj2)
        {

#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
            static hpx::util::itt::event notify_event(
                "get diagonal intersection");
            hpx::util::itt::mark_event e(notify_event);
#endif
            hpx::tracing::mark_event evt("get diagonal intersection");
            if (len1 == 0)
                return {0, (std::min) (k, len2)};
            if (len2 == 0)
                return {(std::min) (k, len1), 0};
            auto a_low = (k > len2) ? (k - len2) : 0;
            auto a_high = (k < len1) ? k : len1;
            if (a_low == a_high)
            {
                auto a = a_low;
                return {a, k - a};    // Only one valid position
            }

            while (a_low <= a_high)
            {
                auto a = (a_low + a_high) / 2;
                auto b = k - a;

                // cond1: a==0 || b==len2 || A[a-1] <= B[b]
                bool cond1 = (a == 0) || (b == len2) ||
                    !HPX_INVOKE(comp, HPX_INVOKE(proj2, *std::next(first2, b)),
                        HPX_INVOKE(proj1, *std::next(first1, a - 1)));

                // cond2: b==0 || a==len1 || B[b-1] < A[a]
                bool cond2 = (b == 0) || (a == len1) ||
                    HPX_INVOKE(comp,
                        HPX_INVOKE(proj2, *std::next(first2, b - 1)),
                        HPX_INVOKE(proj1, *std::next(first1, a)));

                if (cond1 && cond2)
                    return {a, b};

                if (!cond1)
                {
                    a_high = a - 1;
                }
                else
                {
                    a_low = a + 1;
                }
            }
            return {a_high, k - a_high};
        }

        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2,
            typename Comp>
        std::pair<std::size_t, std::size_t> diagonal_intersection(Iter1 first1,
            std::size_t len1, Iter2 first2, std::size_t len2, std::size_t k,
            Comp&& comp, hpx::identity, hpx::identity)
        {

#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
            static hpx::util::itt::event notify_event(
                "get diagonal intersection");
            hpx::util::itt::mark_event e(notify_event);
#endif
            hpx::tracing::mark_event evt("get diagonal intersection");
            if (len1 == 0)
                return {0, (std::min) (k, len2)};
            if (len2 == 0)
                return {(std::min) (k, len1), 0};
            auto a_low = (k > len2) ? (k - len2) : 0;
            auto a_high = (k < len1) ? k : len1;
            if (a_low == a_high)
            {
                auto a = a_low;
                return {a, k - a};    // Only one valid position
            }

            while (a_low <= a_high)
            {
                auto a = (a_low + a_high) / 2;
                auto b = k - a;

                // cond1: a==0 || b==len2 || A[a-1] <= B[b]
                bool cond1 = (a == 0) || (b == len2) ||
                    !HPX_INVOKE(
                        comp, *std::next(first2, b), *std::next(first1, a - 1));

                // cond2: b==0 || a==len1 || B[b-1] < A[a]
                bool cond2 = (b == 0) || (a == len1) ||
                    HPX_INVOKE(
                        comp, *std::next(first2, b - 1), *std::next(first1, a));

                if (cond1 && cond2)
                    return {a, b};

                if (!cond1)
                {
                    a_high = a - 1;
                }
                else
                {
                    a_low = a + 1;
                }
            }
            return {a_high, k - a_high};
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter1,
            typename Sent1, typename Iter2, typename Sent2, typename Iter3,
            typename Comp, typename Proj1, typename Proj2>
        decltype(auto) parallel_merge(ExPolicy&& policy, Iter1 first1,
            Sent1 last1, Iter2 first2, Sent2 last2, Iter3 dest, Comp&& comp,
            Proj1&& proj1, Proj2&& proj2)
        {
            auto end1 = first1;
            auto const len1 = detail::advance_and_get_distance(end1, last1);

            auto end2 = first2;
            auto const len2 = detail::advance_and_get_distance(end2, last2);

            using result_type = util::in_in_out_result<Iter1, Iter2, Iter3>;

            auto params = policy.parameters();
            auto exec = policy.executor();

            auto f1 = [dest, comp, proj1, proj2, first1, first2, len1, len2,
                          params, exec](std::size_t idx, std::size_t chunk) {
                if (len1 != 0 || len2 != 0)
                {
                    std::size_t N = len1 + len2;
                    std::size_t k0 = (std::min) (idx * chunk, N);
                    std::size_t k1 = (std::min) (k0 + chunk, N);

                    auto [a0, b0] = diagonal_intersection(
                        first1, len1, first2, len2, k0, comp, proj1, proj2);
                    auto [a1, b1] = diagonal_intersection(
                        first1, len1, first2, len2, k1, comp, proj1, proj2);

                    auto tid = hpx::this_thread::get_id();
                    hpx::execution::experimental::mark_partition(
                        params, exec, idx, partition_phase::start, a0, b0, tid);
                    sequential_merge(std::next(first1, a0),
                        std::next(first1, a1), std::next(first2, b0),
                        std::next(first2, b1), std::next(dest, k0), comp, proj1,
                        proj2);
                    hpx::execution::experimental::mark_partition(
                        params, exec, idx, partition_phase::end, tid);
                }
            };

            if (len1 > len2)
            {
                auto f2 = [first2, len1, len2, dest](Iter1 l1) {
                    return result_type{l1, std::next(first2, len2),
                        std::next(dest, len1 + len2)};
                };

                auto chunks = get_diagonal_index(len1 + len2);

                return util::foreach_partitioner<std::decay_t<ExPolicy>>::call(
                    HPX_FORWARD(ExPolicy, policy), first1, len1, HPX_MOVE(f1),
                    HPX_MOVE(f2), HPX_MOVE(chunks));
            }

            auto f2 = [first1, len1, len2, dest](Iter2 l2) {
                return result_type{
                    std::next(first1, len1), l2, std::next(dest, len1 + len2)};
            };

            auto chunks = get_diagonal_index(len1 + len2);

            return util::foreach_partitioner<std::decay_t<ExPolicy>>::call(
                HPX_FORWARD(ExPolicy, policy), first2, len2, HPX_MOVE(f1),
                HPX_MOVE(f2), HPX_MOVE(chunks));
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename IterTuple>
        struct merge : public algorithm<merge<IterTuple>, IterTuple>
        {
            constexpr merge() noexcept
              : algorithm<merge, IterTuple>("merge")
            {
            }

            template <typename ExPolicy, typename Iter1, typename Sent1,
                typename Iter2, typename Sent2, typename Iter3, typename Comp,
                typename Proj1, typename Proj2>
            static constexpr util::in_in_out_result<Iter1, Iter2, Iter3>
            sequential(ExPolicy, Iter1 first1, Sent1 last1, Iter2 first2,
                Sent2 last2, Iter3 dest, Comp&& comp, Proj1&& proj1,
                Proj2&& proj2)
            {
                return sequential_merge(first1, last1, first2, last2, dest,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj1, proj1),
                    HPX_FORWARD(Proj2, proj2));
            }

            template <typename ExPolicy, typename Iter1, typename Sent1,
                typename Iter2, typename Sent2, typename Iter3, typename Comp,
                typename Proj1, typename Proj2>
            static typename util::detail::algorithm_result<ExPolicy,
                util::in_in_out_result<Iter1, Iter2, Iter3>>::type
            parallel(ExPolicy&& policy, Iter1 first1, Sent1 last1, Iter2 first2,
                Sent2 last2, Iter3 dest, Comp&& comp, Proj1&& proj1,
                Proj2&& proj2)
            {
                using result_type = util::in_in_out_result<Iter1, Iter2, Iter3>;
                using algorithm_result =
                    util::detail::algorithm_result<ExPolicy, result_type>;

                try
                {
                    return algorithm_result::get(parallel_merge(
                        HPX_FORWARD(ExPolicy, policy), first1, last1, first2,
                        last2, dest, HPX_FORWARD(Comp, comp),
                        HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2)));
                }
                catch (...)
                {
                    return algorithm_result::get(
                        detail::handle_exception<ExPolicy, result_type>::call(
                            std::current_exception()));
                }
            }
        };
        /// \endcond
    }    // namespace detail

    /////////////////////////////////////////////////////////////////////////////
    // inplace_merge
    namespace detail {

        // sequential inplace_merge with projection function.
        HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent,
            typename Comp, typename Proj>
        constexpr Iter sequential_inplace_merge(
            Iter first, Iter middle, Sent last, Comp&& comp, Proj&& proj)
        {
            std::inplace_merge(first, middle,
                detail::advance_to_sentinel(middle, last),
                util::compare_projected<Comp, Proj>(
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj)));
            return last;
        }

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
            typename Sent, typename Comp, typename Proj>
        void parallel_inplace_merge_helper(ExPolicy&& policy, Iter first,
            Iter middle, Sent last, Comp&& comp, Proj&& proj)
        {
            constexpr std::size_t threshold = 65536ul;
            static_assert(threshold >= 5ul);

            std::size_t const left_size = middle - first;
            std::size_t const right_size = last - middle;

            // Perform sequential inplace_merge
            //   if data size is smaller than threshold.
            if (left_size + right_size <= threshold)
            {
                sequential_inplace_merge(first, middle, last,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
                return;
            }

            if (left_size >= right_size)
            {
                // Means that always 'pivot' < 'middle'.
                HPX_ASSERT(left_size >= 3ul);

                // Select pivot in left-side range.
                Iter pivot = first + left_size / 2;
                Iter boundary = lower_bound_helper::call(
                    middle, last, HPX_INVOKE(proj, *pivot), comp, proj);
                Iter target = pivot + (boundary - middle);

                // Swap two blocks, [pivot, middle) and [middle, boundary).
                // After this, [first, last) will be divided into three blocks,
                //   [first, target), target, and [target+1, last).
                // And all elements of [first, target) are less than
                //   the thing of target.
                // And all elements of [target+1, last) are greater or equal than
                //   the thing of target.
                detail::sequential_rotate(pivot, middle, boundary);

                hpx::future<void> fut =
                    execution::async_execute(policy.executor(), [&]() -> void {
                        // Process the range which is left-side of 'target'.
                        parallel_inplace_merge_helper(
                            policy, first, pivot, target, comp, proj);
                    });

                try
                {
                    // Process the range which is right-side of 'target'.
                    parallel_inplace_merge_helper(
                        policy, target + 1, boundary, last, comp, proj);
                }
                catch (...)
                {
                    fut.wait();

                    std::vector<hpx::future<void>> futures;
                    futures.reserve(2);
                    futures.emplace_back(HPX_MOVE(fut));
                    futures.emplace_back(hpx::make_exceptional_future<void>(
                        std::current_exception()));

                    std::list<std::exception_ptr> errors;
                    util::detail::handle_local_exceptions<ExPolicy>::call(
                        futures, errors);

                    HPX_UNREACHABLE;
                }

                if (fut.valid())    // NOLINT
                {
                    fut.get();
                }
            }
            else    // left_size < right_size
            {
                // Means that always 'pivot' < 'last'.
                HPX_ASSERT(right_size >= 3ul);

                // Select pivot in right-side range.
                Iter pivot = middle + right_size / 2;
                Iter boundary = upper_bound_helper::call(
                    first, middle, HPX_INVOKE(proj, *pivot), comp, proj);
                Iter target = boundary + (pivot - middle);

                // Swap two blocks, [boundary, middle) and [middle, pivot+1).
                // After this, [first, last) will be divided into three blocks,
                //   [first, target), target, and [target+1, last).
                // And all elements of [first, target) are less than
                //   the thing of target.
                // And all elements of [target+1, last) are greater or equal than
                //   the thing of target.
                detail::sequential_rotate(boundary, middle, pivot + 1);

                hpx::future<void> fut =
                    execution::async_execute(policy.executor(), [&]() -> void {
                        // Process the range which is left-side of 'target'.
                        parallel_inplace_merge_helper(
                            policy, first, boundary, target, comp, proj);
                    });

                try
                {
                    // Process the range which is right-side of 'target'.
                    parallel_inplace_merge_helper(
                        policy, target + 1, pivot + 1, last, comp, proj);
                }
                catch (...)
                {
                    fut.wait();

                    std::vector<hpx::future<void>> futures;
                    futures.reserve(2);
                    futures.emplace_back(HPX_MOVE(fut));
                    futures.emplace_back(hpx::make_exceptional_future<void>(
                        std::current_exception()));

                    std::list<std::exception_ptr> errors;
                    util::detail::handle_local_exceptions<ExPolicy>::call(
                        futures, errors);

                    HPX_UNREACHABLE;
                }

                if (fut.valid())    // NOLINT
                {
                    fut.get();
                }
            }
        }

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
            typename Sent, typename Comp, typename Proj>
        hpx::future<Iter> parallel_inplace_merge(ExPolicy&& policy, Iter first,
            Iter middle, Sent last, Comp&& comp, Proj&& proj)
        {
            return execution::async_execute(policy.executor(),
                [policy, first, middle, last, comp = HPX_FORWARD(Comp, comp),
                    proj = HPX_FORWARD(Proj, proj)]() mutable -> Iter {
                    try
                    {
                        parallel_inplace_merge_helper(policy, first, middle,
                            last, HPX_MOVE(comp), HPX_MOVE(proj));
                        return last;
                    }
                    catch (...)
                    {
                        util::detail::handle_local_exceptions<ExPolicy>::call(
                            std::current_exception());

                        HPX_UNREACHABLE;
                    }
                });
        }

        HPX_CXX_CORE_EXPORT template <typename Result>
        struct inplace_merge : public algorithm<inplace_merge<Result>, Result>
        {
            constexpr inplace_merge() noexcept
              : algorithm<inplace_merge, Result>("inplace_merge")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename Comp, typename Proj>
            static constexpr Iter sequential(ExPolicy, Iter first, Iter middle,
                Sent last, Comp&& comp, Proj&& proj)
            {
                return sequential_inplace_merge(first, middle, last,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename Comp, typename Proj>
            static util::detail::algorithm_result_t<ExPolicy, Iter> parallel(
                ExPolicy&& policy, Iter first, Iter middle, Sent last,
                Comp&& comp, Proj&& proj)
            {
                using result = util::detail::algorithm_result<ExPolicy, Iter>;

                try
                {
                    return result::get(parallel_inplace_merge(
                        HPX_FORWARD(ExPolicy, policy), first, middle, last,
                        HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj)));
                }
                catch (...)
                {
                    return result::get(
                        detail::handle_exception<ExPolicy, Iter>::call(
                            std::current_exception()));
                }
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter>
        constexpr void get_void_result(Iter) noexcept
        {
        }

        HPX_CXX_CORE_EXPORT template <typename Iter>
        hpx::future<void> get_void_result(hpx::future<Iter>&& f) noexcept
        {
            return hpx::future<void>(HPX_MOVE(f));
        }
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::merge
    HPX_CXX_CORE_EXPORT inline constexpr struct merge_t final
      : hpx::detail::tag_parallel_algorithm<merge_t>
    {
    private:
        template <typename ExPolicy, typename RandIter1, typename RandIter2,
            typename RandIter3, typename Comp = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<RandIter1> &&
                hpx::traits::is_iterator_v<RandIter2> &&
                hpx::traits::is_iterator_v<RandIter3> &&
                hpx::is_invocable_v<Comp,
                    typename std::iterator_traits<RandIter1>::value_type,
                    typename std::iterator_traits<RandIter2>::value_type
                >
            )
        friend hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            RandIter3>
        tag_fallback_invoke(merge_t, ExPolicy&& policy,
            RandIter1 first1, RandIter1 last1, RandIter2 first2,
            RandIter2 last2, RandIter3 dest, Comp comp = Comp())
        // clang-format on
        {
            static_assert(std::random_access_iterator<RandIter1>,
                "Required at least random access iterator.");
            static_assert(std::random_access_iterator<RandIter2>,
                "Requires at least random access iterator.");
            static_assert(std::random_access_iterator<RandIter3>,
                "Requires at least random access iterator.");

            using result_type = hpx::parallel::util::in_in_out_result<RandIter1,
                RandIter2, RandIter3>;

            return hpx::parallel::util::get_third_element(
                hpx::parallel::detail::merge<result_type>().call(
                    HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2,
                    dest, HPX_MOVE(comp), hpx::identity_v, hpx::identity_v));
        }

        template <typename RandIter1, typename RandIter2, typename RandIter3,
            typename Comp = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<RandIter1> &&
                hpx::traits::is_iterator_v<RandIter2> &&
                hpx::traits::is_iterator_v<RandIter3> &&
                hpx::is_invocable_v<Comp,
                    typename std::iterator_traits<RandIter1>::value_type,
                    typename std::iterator_traits<RandIter2>::value_type
                >
            )
        // clang-format on
        friend RandIter3 tag_fallback_invoke(merge_t, RandIter1 first1,
            RandIter1 last1, RandIter2 first2, RandIter2 last2, RandIter3 dest,
            Comp comp = Comp())
        {
            static_assert(std::random_access_iterator<RandIter1>,
                "Required at least random access iterator.");
            static_assert(std::random_access_iterator<RandIter2>,
                "Requires at least random access iterator.");
            static_assert(std::random_access_iterator<RandIter3>,
                "Requires at least random access iterator.");

            using result_type = hpx::parallel::util::in_in_out_result<RandIter1,
                RandIter2, RandIter3>;

            return hpx::parallel::util::get_third_element(
                hpx::parallel::detail::merge<result_type>().call(
                    hpx::execution::seq, first1, last1, first2, last2, dest,
                    HPX_MOVE(comp), hpx::identity_v, hpx::identity_v));
        }
    } merge{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::inplace_merge
    HPX_CXX_CORE_EXPORT inline constexpr struct inplace_merge_t final
      : hpx::detail::tag_parallel_algorithm<inplace_merge_t>
    {
    private:
        template <typename ExPolicy, typename RandIter,
            typename Comp = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<RandIter> &&
                hpx::is_invocable_v<Comp,
                    typename std::iterator_traits<RandIter>::value_type,
                    typename std::iterator_traits<RandIter>::value_type
                >
            )
        // clang-format on
        friend hpx::parallel::util::detail::algorithm_result_t<ExPolicy>
        tag_fallback_invoke(inplace_merge_t, ExPolicy&& policy, RandIter first,
            RandIter middle, RandIter last, Comp comp = Comp())
        {
            static_assert(std::random_access_iterator<RandIter>,
                "Required at least random access iterator.");

            return hpx::parallel::detail::get_void_result(
                hpx::parallel::detail::inplace_merge<RandIter>().call(
                    HPX_FORWARD(ExPolicy, policy), first, middle, last,
                    HPX_MOVE(comp), hpx::identity_v));
        }

        template <typename RandIter,
            typename Comp = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<RandIter> &&
                hpx::is_invocable_v<Comp,
                    typename std::iterator_traits<RandIter>::value_type,
                    typename std::iterator_traits<RandIter>::value_type
                >
            )
        // clang-format on
        friend void tag_fallback_invoke(inplace_merge_t, RandIter first,
            RandIter middle, RandIter last, Comp comp = Comp())
        {
            static_assert(std::random_access_iterator<RandIter>,
                "Required at least random access iterator.");

            return hpx::parallel::detail::get_void_result(
                hpx::parallel::detail::inplace_merge<RandIter>().call(
                    hpx::execution::seq, first, middle, last, HPX_MOVE(comp),
                    hpx::identity_v));
        }
    } inplace_merge{};
}    // namespace hpx

#endif    // DOXYGEN
