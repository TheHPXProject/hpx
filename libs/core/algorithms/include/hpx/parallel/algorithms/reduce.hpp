//  Copyright (c) 2022 Bhumit Attarde
//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/reduce.hpp
/// \page hpx::reduce
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {

    // clang-format off

    /// Returns GENERALIZED_SUM(f, init, *first, ..., *(first + (last - first) - 1)).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a f.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source begin and end iterators used
    ///                     (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a reduce requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The types \a Type1 \a Ret must be
    ///                     such that an object of type \a FwdIter can be
    ///                     dereferenced and then implicitly converted to any
    ///                     of those types.
    ///
    /// The reduce operations in the parallel \a reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a reduce algorithm returns a \a hpx::future<T> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a T otherwise.
    ///           The \a reduce algorithm returns the result of the
    ///           generalized sum over the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a reduce and \a accumulate is
    /// that the behavior of reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename ExPolicy, typename FwdIter, typename F,
        typename T = typename std::iterator_traits<FwdIter>::value_type>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
    reduce(ExPolicy&& policy, FwdIter first, FwdIter last, T init, F&& f);

    /// Returns GENERALIZED_SUM(+, init, *first, ..., *(first + (last - first) - 1)).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         operator+().
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source begin and end iterators used
    ///                     (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    ///
    /// The reduce operations in the parallel \a reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a reduce algorithm returns a \a hpx::future<T> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a T otherwise.
    ///           The \a reduce algorithm returns the result of the
    ///           generalized sum (applying operator+()) over the elements given
    ///           by the input range [first, last).
    ///
    /// \note   GENERALIZED_SUM(+, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(+, b1, ..., bK), GENERALIZED_SUM(+, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a reduce and \a accumulate is
    /// that the behavior of reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename ExPolicy, typename FwdIter,
        typename T = typename std::iterator_traits<FwdIter>::value_type>
    util::detail::algorithm_result_t<ExPolicy, T>
    reduce(ExPolicy&& policy, FwdIter first, FwdIter last, T init);

    /// Returns GENERALIZED_SUM(+, T(), *first, ..., *(first + (last - first) - 1)).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         operator+().
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source begin and end iterators used
    ///                     (deduced).
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
    /// The reduce operations in the parallel \a reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a reduce algorithm returns a \a hpx::future<T> if the
    ///           execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns T otherwise (where T is the value_type of
    ///           \a FwdIter).
    ///           The \a reduce algorithm returns the result of the
    ///           generalized sum (applying operator+()) over the elements given
    ///           by the input range [first, last).
    ///
    /// \note   The type of the initial value (and the result type) \a T is
    ///         determined from the value_type of the used \a FwdIter.
    ///
    /// \note   GENERALIZED_SUM(+, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(+, b1, ..., bK), GENERALIZED_SUM(+, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a reduce and \a accumulate is
    /// that the behavior of reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename ExPolicy, typename FwdIter>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        typename std::iterator_traits<FwdIter>::value_type
    >::type
    reduce(ExPolicy&& policy, FwdIter first, FwdIter last);

    /// Returns GENERALIZED_SUM(f, init, *first, ..., *(first + (last - first) - 1)).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicate \a f.
    ///
    /// \tparam FwdIter     The type of the source begin and end iterators used
    ///                     (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a reduce requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The types \a Type1 \a Ret must be
    ///                     such that an object of type \a InIter can be
    ///                     dereferenced and then implicitly converted to any
    ///                     of those types.
    ///
    /// \returns  The \a reduce algorithm returns \a T.
    ///           The \a reduce algorithm returns the result of the
    ///           generalized sum over the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a reduce and \a accumulate is
    /// that the behavior of reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename FwdIter, typename F,
        typename T = typename std::iterator_traits<FwdIter>::value_type>
    T reduce(FwdIter first, FwdIter last, T init, F&& f);

    /// Returns GENERALIZED_SUM(+, init, *first, ..., *(first + (last - first) - 1)).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         operator+().
    ///
    /// \tparam FwdIter     The type of the source begin and end iterators used
    ///                     (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    ///
    /// \returns  The \a reduce algorithm returns a \a T.
    ///           The \a reduce algorithm returns the result of the
    ///           generalized sum (applying operator+()) over the elements given
    ///           by the input range [first, last).
    ///
    /// \note   GENERALIZED_SUM(+, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(+, b1, ..., bK), GENERALIZED_SUM(+, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a reduce and \a accumulate is
    /// that the behavior of reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename FwdIter,
        typename T = typename std::iterator_traits<FwdIter>::value_type>
    T reduce(FwdIter first, FwdIter last, T init);

    /// Returns GENERALIZED_SUM(+, T(), *first, ..., *(first + (last - first) - 1)).
    /// Executed according to the policy.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         operator+().
    ///
    /// \tparam FwdIter     The type of the source begin and end iterators used
    ///                     (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    ///
    /// \returns  The \a reduce algorithm returns \a T (where T is the
    ///           value_type of \a FwdIter).
    ///           The \a reduce algorithm returns the result of the
    ///           generalized sum (applying operator+()) over the elements given
    ///           by the input range [first, last).
    ///
    /// \note   The type of the initial value (and the result type) \a T is
    ///         determined from the value_type of the used \a FwdIter.
    ///
    /// \note   GENERALIZED_SUM(+, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(+, b1, ..., bK), GENERALIZED_SUM(+, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a reduce and \a accumulate is
    /// that the behavior of reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename FwdIter>
    typename std::iterator_traits<FwdIter>::value_type
    reduce(FwdIter first, FwdIter last);
    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution/executors/rebind_executor_parameters.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/parallel/algorithms/detail/accumulate.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/reduce.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // reduce
    namespace detail {

        /// \cond NOINTERNAL

        // Custom executor parameters for reduce algorithm to prevent
        // single-element partitions. reduce_partition requires at least 2
        // elements per partition because it initializes the accumulator via
        // op(*first, *next(first)), which is the only way to produce a T from
        // value_type elements when T may differ from value_type (e.g. minmax).
        struct reduce_executor_parameters
        {
            template <typename Executor>
            HPX_FORCEINLINE constexpr std::pair<std::size_t, std::size_t>
            adjust_chunk_size_and_max_chunks(Executor&&,
                std::size_t num_elements, std::size_t num_cores,
                std::size_t /*max_chunks*/,
                std::size_t chunk_size) const noexcept
            {
                // Ensure minimum chunk size of 2
                if (chunk_size < 2)
                {
                    chunk_size = (num_elements + num_cores - 1) / num_cores;
                    chunk_size = (std::max) (chunk_size, std::size_t(2));
                }

                // chunk_size_iterator gives the last partition
                // num_elements % chunk_size elements (or chunk_size if
                // evenly divisible). If the remainder is 1, that partition
                // would violate reduce_partition's >= 2 requirement.
                // Bump chunk_size until the remainder is 0 or >= 2.
                while (
                    num_elements > chunk_size && num_elements % chunk_size == 1)
                {
                    chunk_size++;
                }

                std::size_t new_max_chunks =
                    (num_elements + chunk_size - 1) / chunk_size;

                return {chunk_size, new_max_chunks};
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

// Specialize trait to make reduce_executor_parameters a valid executor
// parameters type
namespace hpx::execution::experimental {

    template <>
    struct is_executor_parameters<
        hpx::parallel::detail::reduce_executor_parameters> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

namespace hpx::parallel { namespace detail {

    // Helper function to reduce a partition without requiring an init value.
    // Assumes partition size >= 2 (enforced by reduce_executor_parameters).
    template <typename ExPolicy, typename FwdIterB, typename T, typename Reduce>
    T reduce_partition(
        FwdIterB part_begin, std::size_t part_size, Reduce const& r)
    {
        HPX_ASSERT(part_size >= 2);

        // Combine first two elements using the reduction operator
        T init = HPX_INVOKE(r, *part_begin, *std::next(part_begin));

        if (part_size == 2)
        {
            return init;
        }

        // Reduce remaining elements
        return sequential_reduce<ExPolicy>(
            std::next(part_begin, 2), part_size - 2, HPX_MOVE(init), r);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    struct reduce : public algorithm<reduce<T>, T>
    {
        constexpr reduce() noexcept
          : algorithm<reduce<T>, T>("reduce")
        {
        }

        template <typename ExPolicy, typename InIterB, typename InIterE,
            typename T_, typename Reduce>
        static constexpr T sequential(ExPolicy&& policy, InIterB first,
            InIterE last, T_&& init, Reduce&& r)
        {
            return sequential_reduce<ExPolicy>(HPX_FORWARD(ExPolicy, policy),
                first, last, HPX_FORWARD(T_, init), HPX_FORWARD(Reduce, r));
        }

        template <typename ExPolicy, typename FwdIterB, typename FwdIterE,
            typename T_, typename Reduce>
        static decltype(auto) parallel(ExPolicy&& policy, FwdIterB first,
            FwdIterE last, T_&& init, Reduce&& r)
        {
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            // Handle empty range
            if constexpr (!has_scheduler_executor)
            {
                if (first == last)
                {
                    return util::detail::algorithm_result<ExPolicy, T>::get(
                        HPX_FORWARD(T_, init));
                }
            }

            // Handle single-element case: can't partition into size >= 2
            // This must be checked for all execution policies
            auto const count = hpx::parallel::detail::distance(first, last);
            if (count == 1)
            {
                T result = HPX_INVOKE(r, HPX_FORWARD(T_, init), *first);
                if constexpr (has_scheduler_executor)
                {
                    return result;
                }
                else
                {
                    return util::detail::algorithm_result<ExPolicy, T>::get(
                        HPX_MOVE(result));
                }
            }

            auto f1 = [r](FwdIterB part_begin, std::size_t part_size) -> T {
                return reduce_partition<ExPolicy, FwdIterB, T>(
                    part_begin, part_size, r);
            };

            auto rebound_params =
                hpx::execution::experimental::rebind_executor_parameters(
                    policy.parameters(), reduce_executor_parameters{});
            auto reduce_policy =
                hpx::execution::experimental::create_rebound_policy(
                    policy, HPX_MOVE(rebound_params));
            using reduce_policy_type = std::decay_t<decltype(reduce_policy)>;

            return util::partitioner<reduce_policy_type, T>::call(
                HPX_MOVE(reduce_policy), first,
                count, HPX_MOVE(f1),
                hpx::unwrapping(
                    [init = HPX_FORWARD(T_, init), r = HPX_FORWARD(Reduce, r)](
                        auto&& results) -> T {
                        return sequential_reduce<ExPolicy>(
                            hpx::util::begin(results), hpx::util::size(results),
                            init, r);
                    }));
        }
    };
    /// \endcond
}}    // namespace hpx::parallel::detail

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::reduce
    HPX_CXX_CORE_EXPORT inline constexpr struct reduce_t final
      : hpx::detail::tag_parallel_algorithm<reduce_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter, typename F,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend auto tag_fallback_invoke(hpx::reduce_t, ExPolicy&& policy,
            FwdIter first, FwdIter last, T init, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::reduce<T>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(init),
                HPX_MOVE(f));
        }

        template <typename ExPolicy, typename FwdIter,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend auto tag_fallback_invoke(hpx::reduce_t, ExPolicy&& policy,
            FwdIter first, FwdIter last, T init)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::reduce<T>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(init),
                std::plus<>{});
        }

        template <typename ExPolicy, typename FwdIter>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend auto tag_fallback_invoke(
            hpx::reduce_t, ExPolicy&& policy, FwdIter first, FwdIter last)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            using value_type =
                typename std::iterator_traits<FwdIter>::value_type;

            return hpx::parallel::detail::reduce<value_type>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, value_type{},
                std::plus<>{});
        }

        template <typename InIter, typename F,
            typename T = typename std::iterator_traits<InIter>::value_type>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter>
            )
        // clang-format on
        friend T tag_fallback_invoke(
            hpx::reduce_t, InIter first, InIter last, T init, F f)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::reduce<T>().call(
                hpx::execution::seq, first, last, HPX_MOVE(init), HPX_MOVE(f));
        }

        template <typename InIter,
            typename T = typename std::iterator_traits<InIter>::value_type>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter>
            )
        // clang-format on
        friend T tag_fallback_invoke(
            hpx::reduce_t, InIter first, InIter last, T init)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::reduce<T>().call(hpx::execution::seq,
                first, last, HPX_MOVE(init), std::plus<>{});
        }

        template <typename InIter>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter>
            )
        // clang-format on
        friend typename std::iterator_traits<InIter>::value_type
        tag_fallback_invoke(hpx::reduce_t, InIter first, InIter last)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            using value_type =
                typename std::iterator_traits<InIter>::value_type;

            return hpx::parallel::detail::reduce<value_type>().call(
                hpx::execution::seq, first, last, value_type{}, std::plus<>());
        }
    } reduce{};
}    // namespace hpx

#endif    // DOXYGEN
