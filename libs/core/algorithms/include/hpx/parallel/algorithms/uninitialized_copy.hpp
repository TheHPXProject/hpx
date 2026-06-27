//  Copyright (c) 2014-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/uninitialized_copy.hpp
/// \page hpx::uninitialized_copy, hpx::uninitialized_copy_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {

    /// Copies the elements in the range, defined by [first, last), to an
    /// uninitialized memory area beginning at \a dest. If an exception is
    /// thrown during the copy operation, the function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a uninitialized_copy algorithm invoked
    /// without an execution policy object will execute in sequential order in
    /// the calling thread.
    ///
    /// \returns  The \a uninitialized_copy algorithm returns \a FwdIter.
    ///           The \a uninitialized_copy algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element copied.
    ///
    template <typename InIter, typename FwdIter>
    FwdIter uninitialized_copy(InIter first, InIter last, FwdIter dest);

    /// Copies the elements in the range, defined by [first, last), to an
    /// uninitialized memory area beginning at \a dest. If an exception is
    /// thrown during the copy operation, the function has no effects.
    /// Executed according to the policy.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a uninitialized_copy algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_copy algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a uninitialized_copy algorithm returns a
    ///           \a hpx::future<FwdIter2>, if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2 otherwise.
    ///           The \a uninitialized_copy algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    uninitialized_copy(
        ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest);

    /// Copies the elements in the range [first, first + count), starting from
    /// first and proceeding to first + count - 1., to another range beginning
    /// at dest. If an exception is thrown during the copy operation, the
    /// function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a uninitialized_copy_n algorithm
    /// invoked without an execution policy object execute in sequential order
    /// in the calling thread.
    ///
    /// \returns  The \a uninitialized_copy_n algorithm returns a
    ///           returns \a FwdIter.
    ///           The \a uninitialized_copy_n algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element copied.
    ///
    template <typename InIter, typename Size, typename FwdIter>
    FwdIter uninitialized_copy_n(InIter first, Size count, FwdIter dest);

    /// Copies the elements in the range [first, first + count), starting from
    /// first and proceeding to first + count - 1., to another range beginning
    /// at dest. If an exception is thrown during the copy operation, the
    /// function has no effects.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
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
    /// The assignments in the parallel \a uninitialized_copy_n algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_copy_n algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a uninitialized_copy_n algorithm returns a
    ///           \a hpx::future<FwdIter2> if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2 otherwise.
    ///           The \a uninitialized_copy_n algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Size,
        typename FwdIter2>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    uninitialized_copy_n(
        ExPolicy&& policy, FwdIter1 first, Size count, FwdIter2 dest);
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
#include <hpx/parallel/unseq/loop.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner_with_cleanup.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/transfer.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // uninitialized_copy
    namespace detail {
        /// \cond NOINTERNAL

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
            typename FwdIter2>
        decltype(auto) parallel_uninitialized_copy_n(
            ExPolicy&& policy, Iter first, std::size_t count, FwdIter2 dest)
        {
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            if constexpr (!has_scheduler_executor)
            {
                if (count == 0)
                {
                    return util::detail::algorithm_result<ExPolicy,
                        util::in_out_result<Iter, FwdIter2>>::
                        get(util::in_out_result<Iter, FwdIter2>{first, dest});
                }
            }

            using zip_iterator = hpx::util::zip_iterator<Iter, FwdIter2>;
            using partition_result_type = std::pair<FwdIter2, FwdIter2>;

            return util::partitioner_with_cleanup<ExPolicy,
                util::in_out_result<Iter, FwdIter2>, partition_result_type>::
                call(
                    HPX_FORWARD(ExPolicy, policy), zip_iterator(first, dest),
                    count,
                    [policy](zip_iterator t, std::size_t part_size) mutable
                        -> partition_result_type {
                        using hpx::get;
                        auto iters = t.get_iterator_tuple();
                        FwdIter2 dest = get<1>(iters);
                        return std::make_pair(dest,
                            util::get_second_element(
                                hpx::parallel::util::uninit_copy_n(
                                    HPX_FORWARD(ExPolicy, policy),
                                    get<0>(iters), part_size, dest)));
                    },
                    // finalize, called once if no error occurred
                    [dest, first, count](auto&&... data) mutable
                        -> util::in_out_result<Iter, FwdIter2> {
                        static_assert(sizeof...(data) < 2);

                        // make sure iterators embedded in function object that is
                        // attached to futures are invalidated
                        util::detail::clear_container(data...);

                        std::advance(first, count);
                        std::advance(dest, count);
                        return util::in_out_result<Iter, FwdIter2>{first, dest};
                    },
                    // cleanup function, called for each partition which
                    // didn't fail, but only if at least one failed
                    [](partition_result_type&& r) -> void {
                        while (r.first != r.second)
                        {
                            std::destroy_at(std::addressof(*r.first));
                            ++r.first;
                        }
                    });
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct uninitialized_copy
          : public algorithm<uninitialized_copy<IterPair>, IterPair>
        {
            constexpr uninitialized_copy() noexcept
              : algorithm<uninitialized_copy, IterPair>("uninitialized_copy")
            {
            }

            template <typename ExPolicy, typename InIter1, typename Sent,
                typename FwdIter2>
            static util::in_out_result<InIter1, FwdIter2> sequential(
                ExPolicy&& policy, InIter1 first, Sent last, FwdIter2 dest)
            {
                std::size_t n = detail::distance(first, last);
                return hpx::parallel::util::uninit_copy_n(
                    HPX_FORWARD(ExPolicy, policy), first, n, dest);
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename FwdIter2>
            static decltype(auto) parallel(
                ExPolicy&& policy, Iter first, Sent last, FwdIter2 dest)
            {
                return parallel_uninitialized_copy_n(
                    HPX_FORWARD(ExPolicy, policy), first,
                    detail::distance(first, last), dest);
            }
        };
        /// \endcond
    }    // namespace detail

    /////////////////////////////////////////////////////////////////////////////
    // uninitialized_copy_sent
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct uninitialized_copy_sent
          : public algorithm<uninitialized_copy_sent<IterPair>, IterPair>
        {
            constexpr uninitialized_copy_sent() noexcept
              : algorithm<uninitialized_copy_sent, IterPair>(
                    "uninitialized_copy_sent")
            {
            }

            template <typename ExPolicy, typename InIter1, typename Sent1,
                typename FwdIter2, typename Sent2>
            static util::in_out_result<InIter1, FwdIter2> sequential(
                ExPolicy&& policy, InIter1 first, Sent1 last, FwdIter2 dest,
                Sent2 last_d)
            {
                std::size_t const dist1 = detail::distance(first, last);
                std::size_t const dist2 = detail::distance(dest, last_d);
                std::size_t dist = dist1 <= dist2 ? dist1 : dist2;

                return hpx::parallel::util::uninit_copy_n(
                    HPX_FORWARD(ExPolicy, policy), first, dist, dest);
            }

            template <typename ExPolicy, typename Iter, typename Sent1,
                typename FwdIter2, typename Sent2>
            static decltype(auto) parallel(ExPolicy&& policy, Iter first,
                Sent1 last, FwdIter2 dest, Sent2 last_d)
            {
                std::size_t const dist1 = detail::distance(first, last);
                std::size_t const dist2 = detail::distance(dest, last_d);
                std::size_t dist = dist1 <= dist2 ? dist1 : dist2;

                return parallel_uninitialized_copy_n(
                    HPX_FORWARD(ExPolicy, policy), first, dist, dest);
            }
        };
        /// \endcond
    }    // namespace detail

    /////////////////////////////////////////////////////////////////////////////
    // uninitialized_copy_n
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct uninitialized_copy_n
          : public algorithm<uninitialized_copy_n<IterPair>, IterPair>
        {
            constexpr uninitialized_copy_n() noexcept
              : algorithm<uninitialized_copy_n, IterPair>(
                    "uninitialized_copy_n")
            {
            }

            // non vectorized overload
            template <typename ExPolicy, typename InIter, typename FwdIter2>
                requires(hpx::is_sequenced_execution_policy_v<ExPolicy>)
            static util::in_out_result<InIter, FwdIter2> sequential(
                ExPolicy&& policy, InIter first, std::size_t count,
                FwdIter2 dest)
            {
                return hpx::parallel::util::uninit_copy_n(
                    HPX_FORWARD(ExPolicy, policy), first, count, dest);
            }

            template <typename ExPolicy, typename Iter, typename FwdIter2>
            static decltype(auto) parallel(
                ExPolicy&& policy, Iter first, std::size_t count, FwdIter2 dest)
            {
                return parallel_uninitialized_copy_n(
                    HPX_FORWARD(ExPolicy, policy), first, count, dest);
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::uninitialized_copy
    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_copy_t final
      : hpx::detail::tag_parallel_algorithm<uninitialized_copy_t>
    {
        template <typename InIter, typename FwdIter>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter> &&
                std::forward_iterator<FwdIter>
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            hpx::uninitialized_copy_t, InIter first, InIter last, FwdIter dest)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_copy<
                    parallel::util::in_out_result<InIter, FwdIter>>()
                    .call(hpx::execution::seq, first, last, dest));
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter1> &&
                std::forward_iterator<FwdIter2>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::uninitialized_copy_t,
            ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_copy<
                    parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call(HPX_FORWARD(ExPolicy, policy), first, last, dest));
        }
    } uninitialized_copy{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::uninitialized_copy_n
    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_copy_n_t final
      : hpx::detail::tag_parallel_algorithm<uninitialized_copy_n_t>
    {
        template <typename InIter, typename Size, typename FwdIter>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter> &&
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            hpx::uninitialized_copy_n_t, InIter first, Size count, FwdIter dest)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                return dest;
            }

            return parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_copy_n<
                    parallel::util::in_out_result<InIter, FwdIter>>()
                    .call(hpx::execution::seq, first,
                        static_cast<std::size_t>(count), dest));
        }

        template <typename ExPolicy, typename FwdIter1, typename Size,
            typename FwdIter2>
        // clang-format off
            requires(hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter1> &&
                std::forward_iterator<FwdIter2> &&
                std::is_integral_v<Size>)
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::uninitialized_copy_n_t,
            ExPolicy&& policy, FwdIter1 first, Size count, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
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
                        FwdIter2>::get(HPX_MOVE(dest));
                }
            }

            return parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_copy_n<
                    parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call(HPX_FORWARD(ExPolicy, policy), first,
                        static_cast<std::size_t>(count), dest));
        }
    } uninitialized_copy_n{};
}    // namespace hpx

#endif    // DOXYGEN
