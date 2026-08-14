//  Copyright (c) 2024 Shreyas Atre
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/reduce_deterministic.hpp
/// \page hpx::reduce_deterministic
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
    /// The reduce operations in the parallel \a copy_if algorithm invoked
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
    reduce_deterministic(ExPolicy&& policy, FwdIter first, FwdIter last, T init, F&& f);

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
    /// The reduce operations in the parallel \a copy_if algorithm invoked
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
    reduce_deterministic(ExPolicy&& policy, FwdIter first, FwdIter last, T init);

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
    reduce_deterministic(ExPolicy&& policy, FwdIter first, FwdIter last);

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
    T reduce_deterministic(FwdIter first, FwdIter last, T init, F&& f);

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
    T reduce_deterministic(FwdIter first, FwdIter last, T init);

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
    reduce_deterministic(FwdIter first, FwdIter last);
    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/parallel/algorithms/detail/accumulate.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/reduce.hpp>
#include <hpx/parallel/algorithms/detail/reduce_deterministic.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // reduce
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename T>
        struct reduce_deterministic
          : public algorithm<reduce_deterministic<T>, T>
        {
            constexpr reduce_deterministic() noexcept
              : algorithm<reduce_deterministic, T>("reduce_deterministic")
            {
            }

            template <typename ExPolicy, typename InIterB, typename InIterE,
                typename T_, typename Reduce>
            static constexpr T sequential(ExPolicy&& policy, InIterB first,
                InIterE last, T_&& init, Reduce&& r)
            {
                // TODO: abstract initializing memory
                hpx::parallel::detail::rfa::RFA_bins<T_> bins;
                bins.initialize_bins();
                std::memcpy(hpx::parallel::detail::rfa::hpx_rfa_bin_host_buffer,
                    &bins, sizeof(bins));
                return hpx::parallel::detail::sequential_reduce_deterministic<
                    ExPolicy>(HPX_FORWARD(ExPolicy, policy), first, last,
                    HPX_FORWARD(T_, init), HPX_FORWARD(Reduce, r));
            }

            template <typename ExPolicy, typename FwdIterB, typename FwdIterE,
                typename T_, typename Reduce>
            static util::detail::algorithm_result_t<ExPolicy, T> parallel(
                ExPolicy&& policy, FwdIterB first, FwdIterE last, T_&& init,
                [[maybe_unused]] Reduce&& r)
            {
                if (first == last)
                {
                    return util::detail::algorithm_result<ExPolicy, T>::get(
                        HPX_FORWARD(T_, init));
                }

                // TODO: abstract initializing memory
                hpx::parallel::detail::rfa::RFA_bins<T_> bins;
                bins.initialize_bins();
                std::memcpy(hpx::parallel::detail::rfa::hpx_rfa_bin_host_buffer,
                    &bins, sizeof(bins));

                auto f1 = [policy](FwdIterB part_begin, std::size_t part_size)
                    -> hpx::parallel::detail::rfa::
                        reproducible_floating_accumulator<T_> {
                            T_ val = *part_begin;
                            // Assumed that hpx_rfa_bin_host_buffer is initialized
                            return hpx::parallel::detail::
                                sequential_reduce_deterministic_rfa<ExPolicy>(
                                    HPX_FORWARD(ExPolicy, policy), ++part_begin,
                                    --part_size, HPX_MOVE(val),
                                    std::true_type{});
                        };

                return util::partitioner<ExPolicy, T_,
                    hpx::parallel::detail::rfa::
                        reproducible_floating_accumulator<T_>>::
                    call(HPX_FORWARD(ExPolicy, policy), first,
                        detail::distance(first, last), HPX_MOVE(f1),
                        hpx::unwrapping([policy, init](auto&& results) -> T_ {
                            // Assumed that hpx_rfa_bin_host_buffer is initialized
                            hpx::parallel::detail::rfa::
                                reproducible_floating_accumulator<T_>
                                    rfa;
                            rfa.zero();
                            rfa += init;
                            return hpx::parallel::detail::
                                sequential_reduce_deterministic_rfa<ExPolicy>(
                                    HPX_FORWARD(ExPolicy, policy),
                                    hpx::util::begin(results),
                                    hpx::util::size(results), HPX_MOVE(rfa),
                                    std::false_type{})
                                    .conv();
                        }));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::reduce
    HPX_CXX_CORE_EXPORT inline constexpr struct reduce_deterministic_t final
      : hpx::detail::tag_dispatch<reduce_deterministic_t,
            hpx::detail::tag_parallel_algorithm<reduce_deterministic_t>>
    {
        template <typename ExPolicy, typename FwdIter, typename F,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
        invoke_default(
            ExPolicy&& policy, FwdIter first, FwdIter last, T init, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::reduce_deterministic<T>().call(
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
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
        invoke_default(ExPolicy&& policy, FwdIter first, FwdIter last, T init)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::reduce_deterministic<T>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(init),
                std::plus<>{});
        }

        template <typename ExPolicy, typename FwdIter>
        // clang-format off
            requires(
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        static hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
            typename std::iterator_traits<FwdIter>::value_type>
        invoke_default(ExPolicy&& policy, FwdIter first, FwdIter last)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            using value_type =
                typename std::iterator_traits<FwdIter>::value_type;

            return hpx::parallel::detail::reduce_deterministic<value_type>()
                .call(HPX_FORWARD(ExPolicy, policy), first, last, value_type{},
                    std::plus<>{});
        }

        template <typename InIter, typename F,
            typename T = typename std::iterator_traits<InIter>::value_type>
            requires(hpx::traits::is_iterator_v<InIter>)
        static T invoke_default(InIter first, InIter last, T init, F f)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::reduce_deterministic<T>().call(
                hpx::execution::seq, first, last, HPX_MOVE(init), HPX_MOVE(f));
        }

        template <typename InIter,
            typename T = typename std::iterator_traits<InIter>::value_type>
            requires(hpx::traits::is_iterator_v<InIter>)
        static T invoke_default(InIter first, InIter last, T init)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::reduce_deterministic<T>().call(
                hpx::execution::seq, first, last, HPX_MOVE(init),
                std::plus<>{});
        }

        template <typename InIter>
            requires(hpx::traits::is_iterator_v<InIter>)
        static typename std::iterator_traits<InIter>::value_type invoke_default(
            InIter first, InIter last)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            using value_type =
                typename std::iterator_traits<InIter>::value_type;

            return hpx::parallel::detail::reduce_deterministic<value_type>()
                .call(hpx::execution::seq, first, last, value_type{},
                    std::plus<>());
        }
    } reduce_deterministic{};
}    // namespace hpx::experimental

#endif    // DOXYGEN
