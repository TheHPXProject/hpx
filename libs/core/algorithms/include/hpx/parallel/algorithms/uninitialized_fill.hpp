//  Copyright (c) 2014-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/uninitialized_fill.hpp
/// \page hpx::uninitialized_fill, hpx::uninitialized_fill_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {

    /// Copies the given \a value to an uninitialized memory area, defined by
    /// the range [first, last). If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam T           The type of the value to be assigned (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param value        The value to be assigned.
    ///
    /// \returns  The \a uninitialized_fill algorithm returns nothing
    ///
    template <typename FwdIter, typename T>
    void uninitialized_fill(FwdIter first, FwdIter last, T const& value);

    /// Copies the given \a value to an uninitialized memory area, defined by
    /// the range [first, last). If an exception is thrown during the
    /// initialization, the function has no effects. Executed according to the
    /// policy.
    ///
    /// \note   Complexity: Linear in the distance between \a first and \a last
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
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
    /// The initializations in the parallel \a uninitialized_fill algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The initializations in the parallel \a uninitialized_fill algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified threads,
    /// and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a uninitialized_fill algorithm returns a
    ///           \a hpx::future<void>, if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns nothing
    ///           otherwise.
    ///
    template <typename ExPolicy, typename FwdIter, typename T>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy>
    uninitialized_fill(
        ExPolicy&& policy, FwdIter first, FwdIter last, T const& value);

    /// Copies the given \a value value to the first count elements in an
    /// uninitialized memory area beginning at first. If an exception is thrown
    /// during the initialization, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
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
    /// \returns  The \a uninitialized_fill_n algorithm returns a
    ///           returns \a FwdIter.
    ///           The \a uninitialized_fill_n algorithm returns the output
    ///           iterator to the element in the range, one past
    ///           the last element copied.
    ///
    template <typename FwdIter, typename Size, typename T>
    FwdIter uninitialized_fill_n(FwdIter first, Size count, T const& value);

    /// Copies the given \a value value to the first count elements in an
    /// uninitialized memory area beginning at first. If an exception is thrown
    /// during the initialization, the function has no effects. Executed
    /// according to the policy.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
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
    /// The initializations in the parallel \a uninitialized_fill_n algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The initializations in the parallel \a uninitialized_fill_n algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified threads,
    /// and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a uninitialized_fill_n algorithm returns a
    ///           \a hpx::future<FwdIter>, if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns FwdIter
    ///           otherwise.
    ///           The \a uninitialized_fill_n algorithm returns the output
    ///           iterator to the element in the range, one past
    ///           the last element copied.
    ///
    template <typename ExPolicy, typename FwdIter, typename Size, typename T>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    uninitialized_fill_n(
        ExPolicy&& policy, FwdIter first, Size count, T const& value);
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
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner_with_cleanup.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // uninitialized_fill
    namespace detail {
        /// \cond NOINTERNAL

        // provide our own implementation of std::uninitialized_fill as some
        // versions of MSVC horribly fail at compiling it for some types T
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
            typename Sent, typename T>
        InIter sequential_uninitialized_fill(
            ExPolicy&& policy, InIter first, Sent last, T const& value)
        {
            return util::loop_with_cleanup(
                HPX_FORWARD(ExPolicy, policy), first, last,
                [&value](InIter it) -> void {
                    hpx::construct_at(std::addressof(*it), value);
                },
                [](InIter it) -> void {
                    std::destroy_at(std::addressof(*it));
                });
        }

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
            typename T>
        InIter sequential_uninitialized_fill_n(
            ExPolicy&& policy, InIter first, std::size_t count, T const& value)
        {
            return util::loop_with_cleanup_n(
                HPX_FORWARD(ExPolicy, policy), first, count,
                [&value](InIter it) -> void {
                    hpx::construct_at(std::addressof(*it), value);
                },
                [](InIter it) -> void {
                    std::destroy_at(std::addressof(*it));
                });
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
            typename T>
        decltype(auto) parallel_uninitialized_fill_n(
            ExPolicy&& policy, Iter first, std::size_t count, T const& value)
        {
            bool const has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            if constexpr (!has_scheduler_executor)
            {
                if (count == 0)
                {
                    return util::detail::algorithm_result<ExPolicy, Iter>::get(
                        HPX_MOVE(first));
                }
            }

            using partition_result_type = std::pair<Iter, Iter>;

            return util::partitioner_with_cleanup<ExPolicy, Iter,
                partition_result_type>::
                call(
                    HPX_FORWARD(ExPolicy, policy), first, count,
                    [value, policy](Iter it, std::size_t part_size) mutable
                        -> partition_result_type {
                        return std::make_pair(it,
                            sequential_uninitialized_fill_n(
                                HPX_FORWARD(ExPolicy, policy), it, part_size,
                                value));
                    },
                    // finalize, called once if no error occurred
                    [first, count](auto&&... data) mutable -> Iter {
                        static_assert(sizeof...(data) < 2);

                        // make sure iterators embedded in function object that
                        // is attached to futures are invalidated
                        util::detail::clear_container(data...);

                        std::advance(first, count);
                        return first;
                    },
                    // cleanup function, called for each partition which didn't
                    // fail, but only if at least one failed
                    [](partition_result_type&& r) -> void {
                        while (r.first != r.second)
                        {
                            std::destroy_at(std::addressof(*r.first));
                            ++r.first;
                        }
                    });
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct uninitialized_fill
          : public algorithm<uninitialized_fill<Iter>, Iter>
        {
            constexpr uninitialized_fill() noexcept
              : algorithm<uninitialized_fill, Iter>("uninitialized_fill")
            {
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename T>
            static FwdIter sequential(
                ExPolicy&& policy, FwdIter first, Sent last, T const& value)
            {
                return sequential_uninitialized_fill(
                    HPX_FORWARD(ExPolicy, policy), first, last, value);
            }

            template <typename ExPolicy, typename FwdIter, typename Sent,
                typename T>
            static decltype(auto) parallel(
                ExPolicy&& policy, FwdIter first, Sent last, T const& value)
            {
                bool const has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return util::detail::algorithm_result<ExPolicy,
                            FwdIter>::get(HPX_MOVE(first));
                    }
                }

                return parallel_uninitialized_fill_n(
                    HPX_FORWARD(ExPolicy, policy), first,
                    detail::distance(first, last), value);
            }
        };
        /// \endcond
    }    // namespace detail

    /////////////////////////////////////////////////////////////////////////////
    // uninitialized_fill_n
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct uninitialized_fill_n
          : public algorithm<uninitialized_fill_n<Iter>, Iter>
        {
            constexpr uninitialized_fill_n() noexcept
              : algorithm<uninitialized_fill_n, Iter>("uninitialized_fill_n")
            {
            }

            template <typename ExPolicy, typename FwdIter, typename T>
            static FwdIter sequential(ExPolicy&& policy, FwdIter first,
                std::size_t count, T const& value)
            {
                return sequential_uninitialized_fill_n(
                    HPX_FORWARD(ExPolicy, policy), first, count, value);
            }

            template <typename ExPolicy, typename FwdIter, typename T>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter first,
                std::size_t count, T const& value)
            {
                return parallel_uninitialized_fill_n(
                    HPX_FORWARD(ExPolicy, policy), first, count, value);
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::uninitialized_fill
    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_fill_t final
      : hpx::detail::tag_parallel_algorithm<uninitialized_fill_t>
    {
        template <typename FwdIter, typename T>
        // clang-format off
            requires (
                std::forward_iterator<FwdIter>
            )
        // clang-format on
        friend void tag_fallback_invoke(hpx::uninitialized_fill_t,
            FwdIter first, FwdIter last, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            hpx::parallel::detail::uninitialized_fill<FwdIter>().call(
                hpx::execution::seq, first, last, value);
        }

        template <typename ExPolicy, typename FwdIter, typename T>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::uninitialized_fill_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            using result_type =
                typename hpx::parallel::util::detail::algorithm_result<
                    ExPolicy>::type;

            return hpx::util::void_guard<result_type>(),
                   hpx::parallel::detail::uninitialized_fill<FwdIter>().call(
                       HPX_FORWARD(ExPolicy, policy), first, last, value);
        }

    } uninitialized_fill{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::uninitialized_fill_n
    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_fill_n_t final
      : hpx::detail::tag_parallel_algorithm<uninitialized_fill_n_t>
    {
        template <typename FwdIter, typename Size, typename T>
        // clang-format off
            requires (
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(hpx::uninitialized_fill_n_t,
            FwdIter first, Size count, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                return first;
            }

            return hpx::parallel::detail::uninitialized_fill_n<FwdIter>().call(
                hpx::execution::seq, first, static_cast<std::size_t>(count),
                value);
        }

        template <typename ExPolicy, typename FwdIter, typename Size,
            typename T>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::uninitialized_fill_n_t,
            ExPolicy&& policy, FwdIter first, Size count, T const& value)
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
                    return parallel::util::detail::algorithm_result<ExPolicy,
                        FwdIter>::get(HPX_MOVE(first));
                }
            }

            return hpx::parallel::detail::uninitialized_fill_n<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first,
                static_cast<std::size_t>(count), value);
        }
    } uninitialized_fill_n{};
}    // namespace hpx

#endif    // DOXYGEN
