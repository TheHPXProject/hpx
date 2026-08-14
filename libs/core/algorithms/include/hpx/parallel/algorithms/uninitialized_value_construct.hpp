//  Copyright (c) 2014-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/uninitialized_value_construct.hpp
/// \page hpx::uninitialized_value_construct, hpx::uninitialized_value_construct_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Constructs objects of type typename iterator_traits<ForwardIt>
    /// ::value_type in the uninitialized storage designated by the range
    /// by value-initialization. If an exception is thrown during the
    /// initialization, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    ///
    /// \returns  The \a uninitialized_value_construct algorithm
    ///           returns nothing
    ///
    template <typename FwdIter>
    void uninitialized_value_construct(FwdIter first, FwdIter last);

    /// Constructs objects of type typename iterator_traits<ForwardIt>
    /// ::value_type in the uninitialized storage designated by the range
    /// by value-initialization. If an exception is thrown during the
    /// initialization, the function has no effects. Executed according
    /// to the policy.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct
    /// algorithm invoked with an execution policy object of type \a
    /// sequenced_policy execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct
    /// algorithm invoked with an execution policy object of type \a
    /// parallel_policy or \a parallel_task_policy are permitted to execute
    /// in an unordered fashion in unspecified threads, and indeterminately
    /// sequenced within each thread.
    ///
    /// \returns  The \a uninitialized_value_construct algorithm returns a
    ///           \a hpx::future<void>, if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns nothing
    ///           otherwise.
    ///
    template <typename ExPolicy, typename FwdIter>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy>
    uninitialized_value_construct(
        ExPolicy&& policy, FwdIter first, FwdIter last);

    /// Constructs objects of type typename iterator_traits<ForwardIt>
    /// ::value_type in the uninitialized storage designated by the range
    /// [first, first + count) by value-initialization. If an exception
    /// is thrown during the initialization, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    ///
    /// \returns  The \a uninitialized_value_construct_n algorithm returns a
    ///           returns \a FwdIter.
    ///           The \a uninitialized_value_construct_n algorithm returns
    ///           the iterator to the element in the source range, one past
    ///           the last element constructed.
    ///
    template <typename FwdIter, typename Size>
    FwdIter uninitialized_value_construct_n(FwdIter first, Size count);

    /// Constructs objects of type typename iterator_traits<ForwardIt>
    /// ::value_type in the uninitialized storage designated by the range
    /// [first, first + count) by value-initialization. If an exception
    /// is thrown during the initialization, the function has no effects.
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
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct_n
    /// algorithm invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_value_construct_n
    /// algorithm invoked with an execution policy object of type
    /// \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a uninitialized_value_construct_n algorithm returns a
    ///           \a hpx::future<FwdIter> if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a uninitialized_value_construct_n algorithm returns
    ///           the iterator to the element in the source range, one past
    ///           the last element constructed.
    ///
    template <typename ExPolicy, typename FwdIter, typename Size>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    uninitialized_value_construct_n(
        ExPolicy&& policy, FwdIter first, Size count);
    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
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
    // uninitialized_value_construct
    namespace detail {

        // provide our own implementation of std::uninitialized_value_construct
        // as some versions of MSVC horribly fail at compiling it for some types
        // T
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
            typename Sent>
        InIter sequential_uninitialized_value_construct(
            ExPolicy&& policy, InIter first, Sent last)
        {
            return util::loop_with_cleanup(
                HPX_FORWARD(ExPolicy, policy), first, last,
                [](InIter it) -> void {
                    hpx::construct_at(std::addressof(*it));
                },
                [](InIter it) -> void {
                    std::destroy_at(std::addressof(*it));
                });
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter>
        InIter sequential_uninitialized_value_construct_n(
            ExPolicy&& policy, InIter first, std::size_t count)
        {
            return util::loop_with_cleanup_n(
                HPX_FORWARD(ExPolicy, policy), first, count,
                [](InIter it) -> void {
                    hpx::construct_at(std::addressof(*it));
                },
                [](InIter it) -> void {
                    std::destroy_at(std::addressof(*it));
                });
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename FwdIter>
        decltype(auto) parallel_uninitialized_value_construct_n(
            ExPolicy&& policy, FwdIter first, std::size_t count)
        {
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            if constexpr (!has_scheduler_executor)
            {
                if (count == 0)
                {
                    return util::detail::algorithm_result<ExPolicy,
                        FwdIter>::get(HPX_MOVE(first));
                }
            }

            using partition_result_type = std::pair<FwdIter, FwdIter>;

            return util::partitioner_with_cleanup<ExPolicy, FwdIter,
                partition_result_type>::
                call(
                    HPX_FORWARD(ExPolicy, policy), first, count,
                    [policy](FwdIter it, std::size_t part_size) mutable
                        -> partition_result_type {
                        return std::make_pair(it,
                            sequential_uninitialized_value_construct_n(
                                HPX_FORWARD(ExPolicy, policy), it, part_size));
                    },
                    // finalize, called once if no error occurred
                    [first, count](auto&&... data) mutable -> FwdIter {
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
        HPX_CXX_CORE_EXPORT template <typename FwdIter>
        struct uninitialized_value_construct
          : public algorithm<uninitialized_value_construct<FwdIter>, FwdIter>
        {
            constexpr uninitialized_value_construct() noexcept
              : algorithm<uninitialized_value_construct, FwdIter>(
                    "uninitialized_value_construct")
            {
            }

            template <typename ExPolicy, typename Sent>
            static FwdIter sequential(
                ExPolicy&& policy, FwdIter first, Sent last)
            {
                return sequential_uninitialized_value_construct(
                    HPX_FORWARD(ExPolicy, policy), first, last);
            }

            template <typename ExPolicy, typename Sent>
            static decltype(auto) parallel(
                ExPolicy&& policy, FwdIter first, Sent last)
            {
                return parallel_uninitialized_value_construct_n(
                    HPX_FORWARD(ExPolicy, policy), first,
                    detail::distance(first, last));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // uninitialized_value_construct_n
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename FwdIter>
        struct uninitialized_value_construct_n
          : public algorithm<uninitialized_value_construct_n<FwdIter>, FwdIter>
        {
            constexpr uninitialized_value_construct_n() noexcept
              : algorithm<uninitialized_value_construct_n, FwdIter>(
                    "uninitialized_value_construct_n")
            {
            }

            template <typename ExPolicy>
            static FwdIter sequential(
                ExPolicy&& policy, FwdIter first, std::size_t count)
            {
                return sequential_uninitialized_value_construct_n(
                    HPX_FORWARD(ExPolicy, policy), first, count);
            }

            template <typename ExPolicy>
            static decltype(auto) parallel(
                ExPolicy&& policy, FwdIter first, std::size_t count)
            {
                return parallel_uninitialized_value_construct_n(
                    HPX_FORWARD(ExPolicy, policy), first, count);
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::uninitialized_value_construct
    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_value_construct_t
        final
      : hpx::detail::tag_dispatch<uninitialized_value_construct_t,
            hpx::detail::tag_parallel_algorithm<
                uninitialized_value_construct_t>>
    {
        template <typename FwdIter>
        // clang-format off
            requires (
                std::forward_iterator<FwdIter>
            )
        // clang-format on
        static void invoke_default(FwdIter first, FwdIter last)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            hpx::parallel::detail::uninitialized_value_construct<FwdIter>()
                .call(hpx::execution::seq, first, last);
        }

        template <typename ExPolicy, typename FwdIter>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter>
            )
        // clang-format on
        static decltype(auto) invoke_default(
            ExPolicy&& policy, FwdIter first, FwdIter last)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            using result_type =
                typename hpx::parallel::util::detail::algorithm_result<
                    ExPolicy>::type;

            return hpx::util::void_guard<result_type>(),
                   hpx::parallel::detail::uninitialized_value_construct<
                       FwdIter>()
                       .call(HPX_FORWARD(ExPolicy, policy), first, last);
        }
    } uninitialized_value_construct{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::uninitialized_value_construct_n
    HPX_CXX_CORE_EXPORT inline constexpr struct
        uninitialized_value_construct_n_t final
      : hpx::detail::tag_dispatch<uninitialized_value_construct_n_t,
            hpx::detail::tag_parallel_algorithm<
                uninitialized_value_construct_n_t>>
    {
        template <typename FwdIter, typename Size>
        // clang-format off
            requires (
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, Size count)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                return first;
            }

            return hpx::parallel::detail::uninitialized_value_construct_n<
                FwdIter>()
                .call(hpx::execution::seq, first,
                    static_cast<std::size_t>(count));
        }

        template <typename ExPolicy, typename FwdIter, typename Size>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        static decltype(auto) invoke_default(
            ExPolicy&& policy, FwdIter first, Size count)
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

            return hpx::parallel::detail::uninitialized_value_construct_n<
                FwdIter>()
                .call(HPX_FORWARD(ExPolicy, policy), first,
                    static_cast<std::size_t>(count));
        }
    } uninitialized_value_construct_n{};
}    // namespace hpx

#endif    // DOXYGEN
