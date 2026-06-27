//  Copyright (c) 2014-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/uninitialized_move.hpp
/// \page hpx::uninitialized_move, hpx::uninitialized_move_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    /// Moves the elements in the range, defined by [first, last), to an
    /// uninitialized memory area beginning at \a dest. If an exception is
    /// thrown during the initialization, some objects in [first, last) are
    /// left in a valid but unspecified state.
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
    /// \returns  The \a uninitialized_move algorithm returns \a FwdIter.
    ///           The \a uninitialized_move algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element moved.
    ///
    template <typename InIter, typename FwdIter>
    FwdIter uninitialized_move(InIter first, InIter last, FwdIter dest);

    /// Moves the elements in the range, defined by [first, last), to an
    /// uninitialized memory area beginning at \a dest. If an exception is
    /// thrown during the initialization, some objects in [first, last) are
    /// left in a valid but unspecified state. Executed according to the
    /// policy.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
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
    /// The assignments in the parallel \a uninitialized_move algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_move algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a uninitialized_move algorithm returns a
    ///           \a hpx::future<FwdIter2>, if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2 otherwise.
    ///           The \a uninitialized_move algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element moved.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    uninitialized_move(
        ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest);

    /// Moves the elements in the range [first, first + count), starting from
    /// first and proceeding to first + count - 1., to another range beginning
    /// at dest. If an exception is
    /// thrown during the initialization, some objects in [first, first + count)
    /// are left in a valid but unspecified state.
    ///
    /// \note   Complexity: Performs exactly \a count movements, if
    ///         count > 0, no move operations otherwise.
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
    /// \returns  The \a uninitialized_move_n algorithm returns a
    ///           returns \a std::pair<InIter,FwdIter>.
    ///           The \a uninitialized_move_n algorithm returns A pair whose
    ///           first element is an iterator to the element past the last
    ///           element moved in the source range, and whose second element
    ///           is an iterator to the element past the last element moved
    ///           in the destination range.
    ///
    template <typename InIter, typename Size, typename FwdIter>
    std::pair<InIter, FwdIter> uninitialized_move_n(
        InIter first, Size count, FwdIter dest);

    /// Moves the elements in the range [first, first + count), starting from
    /// first and proceeding to first + count - 1., to another range beginning
    /// at dest. If an exception is
    /// thrown during the initialization, some objects in [first, first + count)
    /// are left in a valid but unspecified state. Executed according to the
    /// policy.
    ///
    /// \note   Complexity: Performs exactly \a count movements, if
    ///         count > 0, no move operations otherwise.
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
    /// The assignments in the parallel \a uninitialized_move_n algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_move_n algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a uninitialized_move_n algorithm returns a
    ///           \a hpx::future<std::pair<FwdIter1,FwdIter2>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a std::pair<FwdIter1,FwdIter2> otherwise.
    ///           The \a uninitialized_move_n algorithm returns A pair whose
    ///           first element is an iterator to the element past the last
    ///           element moved in the source range, and whose second element
    ///           is an iterator to the element past the last element moved
    ///           in the destination range.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Size,
        typename FwdIter2>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        std::pair<FwdIter1, FwdIter2>>::type
    uninitialized_move_n(
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
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // uninitialized_move
    namespace detail {

        /// \cond NOINTERNAL
        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename InIter1, typename InIter2>
        util::in_out_result<InIter1, InIter2> sequential_uninitialized_move_n(
            InIter1 first, std::size_t count, InIter2 dest)
        {
            return util::in_out_result<InIter1, InIter2>{
                std::next(first, count),
                util::loop_with_cleanup_n(
                    first, count, dest,
                    [](InIter1 it, InIter2 dest) -> void {
                        hpx::construct_at(
                            std::addressof(*dest), std::ranges::iter_move(it));
                    },
                    [](InIter2 dest) -> void {
                        std::destroy_at(std::addressof(*dest));
                    })};
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
            typename FwdIter2>
        decltype(auto) parallel_uninitialized_move_n(
            ExPolicy&& policy, Iter first, std::size_t count, FwdIter2 dest)
        {
            bool const has_scheduler_executor =
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
                        FwdIter2 dst = get<1>(iters);
                        return std::make_pair(dst,
                            util::get_second_element(
                                hpx::parallel::util::uninit_move_n(
                                    HPX_FORWARD(ExPolicy, policy),
                                    get<0>(iters), part_size, dst)));
                    },
                    // finalize, called once if no error occurred
                    [first, dest, count](auto&&... data) mutable
                        -> util::in_out_result<Iter, FwdIter2> {
                        static_assert(sizeof...(data) < 2);

                        // make sure iterators embedded in function object that
                        // is attached to futures are invalidated
                        util::detail::clear_container(data...);

                        std::advance(first, count);
                        std::advance(dest, count);
                        return util::in_out_result<Iter, FwdIter2>{first, dest};
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
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct uninitialized_move
          : public algorithm<uninitialized_move<IterPair>, IterPair>
        {
            constexpr uninitialized_move() noexcept
              : algorithm<uninitialized_move, IterPair>("uninitialized_move")
            {
            }

            template <typename ExPolicy, typename InIter1, typename Sent,
                typename FwdIter2>
            static util::in_out_result<InIter1, FwdIter2> sequential(
                ExPolicy&& policy, InIter1 first, Sent last, FwdIter2 dest)
            {
                return hpx::parallel::util::uninit_move_n(
                    HPX_FORWARD(ExPolicy, policy), first,
                    hpx::parallel::detail::distance(first, last), dest);
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename FwdIter2>
            static decltype(auto) parallel(
                ExPolicy&& policy, Iter first, Sent last, FwdIter2 dest)
            {
                return parallel_uninitialized_move_n(
                    HPX_FORWARD(ExPolicy, policy), first,
                    hpx::parallel::detail::distance(first, last), dest);
            }
        };
        /// \endcond
    }    // namespace detail

    /////////////////////////////////////////////////////////////////////////////
    // uninitialized_move_sent
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct uninitialized_move_sent
          : public algorithm<uninitialized_move_sent<IterPair>, IterPair>
        {
            constexpr uninitialized_move_sent() noexcept
              : algorithm<uninitialized_move_sent, IterPair>(
                    "uninitialized_move_sent")
            {
            }

            template <typename ExPolicy, typename InIter1, typename Sent1,
                typename FwdIter2, typename Sent2>
            static util::in_out_result<InIter1, FwdIter2> sequential(
                ExPolicy&& policy, InIter1 first, Sent1 last, FwdIter2 dest,
                Sent2 last_d)
            {
                std::size_t const dist1 =
                    hpx::parallel::detail::distance(first, last);
                std::size_t const dist2 =
                    hpx::parallel::detail::distance(dest, last_d);
                std::size_t dist = dist1 <= dist2 ? dist1 : dist2;

                return hpx::parallel::util::uninit_move_n(
                    HPX_FORWARD(ExPolicy, policy), first, dist, dest);
            }

            template <typename ExPolicy, typename Iter, typename Sent1,
                typename FwdIter2, typename Sent2>
            static decltype(auto) parallel(ExPolicy&& policy, Iter first,
                Sent1 last, FwdIter2 dest, Sent2 last_d)
            {
                std::size_t const dist1 =
                    hpx::parallel::detail::distance(first, last);
                std::size_t const dist2 =
                    hpx::parallel::detail::distance(dest, last_d);
                std::size_t dist = dist1 <= dist2 ? dist1 : dist2;

                return parallel_uninitialized_move_n(
                    HPX_FORWARD(ExPolicy, policy), first, dist, dest);
            }
        };
        /// \endcond
    }    // namespace detail

    /////////////////////////////////////////////////////////////////////////////
    // uninitialized_move_n
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct uninitialized_move_n
          : public algorithm<uninitialized_move_n<IterPair>, IterPair>
        {
            constexpr uninitialized_move_n() noexcept
              : algorithm<uninitialized_move_n, IterPair>(
                    "uninitialized_move_n")
            {
            }

            template <typename ExPolicy, typename InIter1, typename InIter2>
            static IterPair sequential(ExPolicy&& policy, InIter1 first,
                std::size_t count, InIter2 dest)
            {
                return hpx::parallel::util::uninit_move_n(
                    HPX_FORWARD(ExPolicy, policy), first, count, dest);
            }

            template <typename ExPolicy, typename Iter, typename FwdIter2>
            static decltype(auto) parallel(
                ExPolicy&& policy, Iter first, std::size_t count, FwdIter2 dest)
            {
                return parallel_uninitialized_move_n(
                    HPX_FORWARD(ExPolicy, policy), first, count, dest);
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::uninitialized_move
    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_move_t final
      : hpx::detail::tag_parallel_algorithm<uninitialized_move_t>
    {
        template <typename InIter, typename FwdIter>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter> &&
                std::forward_iterator<FwdIter>
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            hpx::uninitialized_move_t, InIter first, InIter last, FwdIter dest)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_move<
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
        friend decltype(auto) tag_fallback_invoke(hpx::uninitialized_move_t,
            ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_move<
                    parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call(HPX_FORWARD(ExPolicy, policy), first, last, dest));
        }
    } uninitialized_move{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::uninitialized_move_n
    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_move_n_t final
      : hpx::detail::tag_parallel_algorithm<uninitialized_move_n_t>
    {
        template <typename InIter, typename Size, typename FwdIter>
        // clang-format off
            requires(hpx::traits::is_iterator_v<InIter> &&
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>)
        // clang-format on
        friend std::pair<InIter, FwdIter> tag_fallback_invoke(
            hpx::uninitialized_move_n_t, InIter first, Size count, FwdIter dest)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                return std::pair<InIter, FwdIter>(first, dest);
            }

            return parallel::util::get_pair(
                hpx::parallel::detail::uninitialized_move_n<
                    parallel::util::in_out_result<InIter, FwdIter>>()
                    .call(hpx::execution::seq, first,
                        static_cast<std::size_t>(count), dest));
        }

        template <typename ExPolicy, typename FwdIter1, typename Size,
            typename FwdIter2>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter1> &&
                std::forward_iterator<FwdIter2> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::uninitialized_move_n_t,
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
                        std::pair<FwdIter1, FwdIter2>>::get(std::pair<FwdIter1,
                        FwdIter2>(first, dest));
                }
            }

            return parallel::util::get_pair(
                hpx::parallel::detail::uninitialized_move_n<
                    parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call(HPX_FORWARD(ExPolicy, policy), first,
                        static_cast<std::size_t>(count), dest));
        }
    } uninitialized_move_n{};
}    // namespace hpx

#endif    // DOXYGEN
