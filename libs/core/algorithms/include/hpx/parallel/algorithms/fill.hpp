//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2017-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/fill.hpp
/// \page hpx::fill, hpx::fill_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Assigns the given value to the elements in the range [first, last).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// The comparisons in the parallel \a fill algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparisons in the parallel \a fill algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a fill algorithm returns a \a hpx::future<void> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a difference_type otherwise (where \a difference_type
    ///           is defined by \a void.
    ///
    template <typename ExPolicy, typename FwdIter, typename T>
    util::detail::algorithm_result_t<ExPolicy>
    fill(ExPolicy&& policy, FwdIter first, FwdIter last, T value);

    /// Assigns the given value to the elements in the range [first, last).
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// \returns  The \a fill algorithm returns a \a void.
    ///
    template <typename FwdIter, typename T>
    void fill(FwdIter first, FwdIter last, T value);


    /// Assigns the given value value to the first count elements in the range
    /// beginning at first if count > 0. Does nothing otherwise. Executed
    /// according to the policy.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, for
    ///         count > 0.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// The comparisons in the parallel \a fill_n algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparisons in the parallel \a fill_n algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a fill_n algorithm returns a \a hpx::future<void> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a difference_type otherwise (where \a difference_type
    ///           is defined by \a void.
    ///
    template <typename ExPolicy, typename FwdIter, typename Size, typename T>
    util::detail::algorithm_result_t<ExPolicy, FwdIter>
    fill_n(ExPolicy&& policy, FwdIter first, Size count, T value);

    /// Assigns the given value value to the first count elements in the range
    /// beginning at first if count > 0. Does nothing otherwise.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, for
    ///         count > 0.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// \returns  The \a fill_n algorithm returns a \a FwdIter.
    ///
    template <typename FwdIter, typename Size, typename T>
    FwdIter fill_n(FwdIter first, Size count, T value);

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/fill.hpp>
#include <hpx/parallel/algorithms/for_each.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // fill
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename T>
        struct fill_iteration
        {
            std::decay_t<T> val_;

            template <typename U>
            HPX_HOST_DEVICE void operator()(U&& u) const
            {
                u = val_;
            }
        };

        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct fill : public algorithm<fill<Iter>, Iter>
        {
            constexpr fill() noexcept
              : algorithm<fill, Iter>("fill")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent,
                typename T>
            HPX_HOST_DEVICE static constexpr InIter sequential(
                ExPolicy&& policy, InIter first, Sent last, T const& val)
            {
                return detail::sequential_fill(
                    HPX_FORWARD(ExPolicy, policy), first, last, val);
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename T>
            static decltype(auto) parallel(
                ExPolicy&& policy, FwdIter first, Sent last, T const& val)
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

                return for_each_n<FwdIter>().call(HPX_FORWARD(ExPolicy, policy),
                    first, detail::distance(first, last),
                    fill_iteration<T>{val}, hpx::identity_v);
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // fill_n
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename FwdIter>
        struct fill_n : public algorithm<fill_n<FwdIter>, FwdIter>
        {
            constexpr fill_n() noexcept
              : algorithm<fill_n, FwdIter>("fill_n")
            {
            }

            template <typename ExPolicy, typename InIter, typename T>
            static constexpr InIter sequential(ExPolicy&& policy, InIter first,
                std::size_t count, T const& val)
            {
                return detail::sequential_fill_n(
                    HPX_FORWARD(ExPolicy, policy), first, count, val);
            }

            template <typename ExPolicy, typename FwdIter_, typename T>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter_ first,
                std::size_t count, T const& val)
            {
                return for_each_n<FwdIter_>().call(
                    HPX_FORWARD(ExPolicy, policy), first, count,
                    [val](auto&& v) -> void { v = val; }, hpx::identity_v);
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::fill
    HPX_CXX_CORE_EXPORT inline constexpr struct fill_t final
      : hpx::detail::tag_parallel_algorithm<fill_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires(hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>)
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(fill_t, ExPolicy&& policy,
            FwdIter first, FwdIter last, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            using result_type =
                typename hpx::parallel::util::detail::algorithm_result<
                    ExPolicy>::type;

            return hpx::util::void_guard<result_type>(),
                   hpx::parallel::detail::fill<FwdIter>().call(
                       HPX_FORWARD(ExPolicy, policy), first, last, value);
        }

        template <typename FwdIter,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires (
                std::forward_iterator<FwdIter>
            )
        // clang-format on
        friend void tag_fallback_invoke(
            fill_t, FwdIter first, FwdIter last, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            hpx::parallel::detail::fill<FwdIter>().call(
                hpx::execution::seq, first, last, value);
        }
    } fill{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::fill_n
    HPX_CXX_CORE_EXPORT inline constexpr struct fill_n_t final
      : hpx::detail::tag_parallel_algorithm<fill_n_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter, typename Size,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(fill_n_t, ExPolicy&& policy,
            FwdIter first, Size count, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                if constexpr (has_scheduler_executor)
                {
                    count = static_cast<Size>(0);
                }
                else
                {
                    return hpx::parallel::util::detail::algorithm_result<
                        ExPolicy, FwdIter>::get(HPX_MOVE(first));
                }
            }

            return hpx::parallel::detail::fill_n<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first,
                static_cast<std::size_t>(count), value);
        }

        template <typename FwdIter, typename Size,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires (
                std::forward_iterator<FwdIter>
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            fill_n_t, FwdIter first, Size count, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                return first;
            }

            return hpx::parallel::detail::fill_n<FwdIter>().call(
                hpx::execution::seq, first, static_cast<std::size_t>(count),
                value);
        }
    } fill_n{};
}    // namespace hpx

#endif    // DOXYGEN
