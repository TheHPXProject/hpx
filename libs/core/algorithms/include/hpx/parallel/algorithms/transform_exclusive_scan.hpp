//  Copyright (c) 2014-2023 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/transform_exclusive_scan.hpp
/// \page hpx::transform_exclusive_scan
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Transforms each element in the range [first, last) with \a unary_op,
    /// then computes an exclusive prefix sum operation using \a binary_op
    /// over the resulting range, with \a init as the initial value, and writes
    /// the results to the range beginning at \a dest.
    /// "exclusive" means that the i-th input element is not included in the
    /// i-th sum. Formally, assigns through each iterator i in
    /// [dest, d_first + (last - first)) the value of the generalized
    /// noncommutative sum of init, unary_op(*j)... for every j in
    /// [first, first + (i - d_first)) over binary_op, where generalized
    /// noncommutative sum GNSUM(op, a1, ..., a N) is defined as follows:
    ///     - if N=1, a1
    ///     - if N > 1, op(GNSUM(op, a1, ..., aK), GNSUM(op, aM, ..., aN))
    ///     for any K where 1 < K+1 = M <= N
    /// In other words, the summation operations may be performed in arbitrary
    /// order, and the behavior is nondeterministic if \a binary_op is not
    /// associative.
    ///
    /// \note   Complexity: O(last - first) applications of each of \a binary_op
    ///                     and \a unary_op.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam BinOp       The type of \a binary_op.
    /// \tparam UnOp        The type of \a unary_op.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param init         The initial value for the generalized sum.
    /// \param binary_op    Binary \a FunctionObject that will be applied to
    ///                     the result of \a unary_op, the results of other
    ///                     \a binary_op, and \a init.
    /// \param unary_op     Unary \a FunctionObject that will be applied to each
    ///                     element of the input range. The return type must
    ///                     be acceptable as input to \a binary_op.
    ///
    /// The reduce operations in the parallel \a transform_exclusive_scan
    /// algorithm invoked without an execution policy object execute in
    /// sequential order in the calling thread.
    ///
    /// \returns  The \a transform_exclusive_scan algorithm returns a
    ///           returns \a OutIter.
    ///           The \a transform_exclusive_scan algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element copied.
    ///
    /// \note   GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aN) is defined as:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aK),
    ///           GENERALIZED_NONCOMMUTATIVE_SUM(op, aM, ..., aN)
    ///           where 1 < K+1 = M <= N.
    ///
    /// Neither \a unary_op nor \a binary_op shall invalidate iterators or
    /// sub-ranges, or modify elements in the ranges [first,last) or
    /// [result,result + (last - first)).
    ///
    /// The behavior of transform_exclusive_scan may be non-deterministic for
    /// a non-associative predicate.
    ///
    template <typename InIter, typename OutIter, typename BinOp, typename UnOp,
        typename T = typename std::iterator_traits<InIter>::value_type>
    OutIter transform_exclusive_scan(InIter first, InIter last, OutIter dest,
        T init, BinOp&& binary_op, UnOp&& unary_op);

    ///////////////////////////////////////////////////////////////////////////
    /// Assigns through each iterator \a i in [result, result + (last - first))
    /// the value of
    /// GENERALIZED_NONCOMMUTATIVE_SUM(binary_op, init, conv(*first), ...,
    /// conv(*(first + (i - result) - 1))). Executed according to the policy.
    ///
    /// \note   Complexity: O(last - first) applications of each of \a binary_op
    ///                     and \a unary_op.
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
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam BinOp       The type of \a binary_op.
    /// \tparam UnOp        The type of \a unary_op.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param init         The initial value for the generalized sum.
    /// \param binary_op    Binary \a FunctionObject that will be applied in to
    ///                     the result of \a unary_op, the results of other
    ///                     \a binary_op, and \a init.
    /// \param unary_op    Unary \a FunctionObject that will be applied to each
    ///                     element of the input range. The return type must
    ///                     be acceptable as input to \a binary_op.
    ///
    /// The reduce operations in the parallel \a transform_exclusive_scan
    /// algorithm invoked with an execution policy object of type \a
    /// sequenced_policy execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a transform_exclusive_scan
    /// algorithm invoked with an execution policy object of type \a
    /// parallel_policy or \a parallel_task_policy are permitted to execute
    /// in an unordered fashion in unspecified threads, and indeterminately
    /// sequenced within each thread.
    ///
    /// \returns  The \a transform_exclusive_scan algorithm returns a
    ///           \a hpx::future<FwdIter2> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2 otherwise.
    ///           The \a transform_exclusive_scan algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element copied.
    ///
    /// \note   GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aN) is defined as:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_NONCOMMUTATIVE_SUM(op, a1, ..., aK),
    ///           GENERALIZED_NONCOMMUTATIVE_SUM(op, aM, ..., aN)
    ///           where 1 < K+1 = M <= N.
    ///
    /// Neither \a unary_op nor \a binary_op shall invalidate iterators or
    /// sub-ranges, or modify elements in the ranges [first,last) or
    /// [result,result + (last - first)).
    ///
    /// The behavior of transform_exclusive_scan may be non-deterministic for
    /// a non-associative predicate.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename BinOp, typename UnOp,
        typename T = typename std::iterator_traits<FwdIter1>::value_type>
    typename parallel::util::detail::algorithm_result<ExPolicy,
        FwdIter2>::type
     transform_exclusive_scan(ExPolicy&& policy, FwdIter1 first,
         FwdIter1 last, FwdIter2 dest, T init, BinOp&& binary_op,
         UnOp&& unary_op);
    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/transform_inclusive_scan.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/scan_partitioner.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // transform_exclusive_scan
    namespace detail {
        /// \cond NOINTERNAL

        // Our own version of the sequential transform_exclusive_scan.
        HPX_CXX_CORE_EXPORT template <typename InIter, typename Sent,
            typename OutIter, typename Conv, typename T, typename Op>
        constexpr util::in_out_result<InIter, OutIter>
        sequential_transform_exclusive_scan(
            InIter first, Sent last, OutIter dest, Conv&& conv, T init, Op&& op)
        {
            T temp = init;
            for (/* */; first != last; (void) ++first, ++dest)
            {
                init = HPX_INVOKE(op, init, HPX_INVOKE(conv, *first));
                *dest = temp;
                temp = init;
            }
            return util::in_out_result<InIter, OutIter>{first, dest};
        }

        HPX_CXX_CORE_EXPORT template <typename InIter, typename OutIter,
            typename Conv, typename T, typename Op>
        constexpr T sequential_transform_exclusive_scan_n(InIter first,
            std::size_t count, OutIter dest, Conv&& conv, T init, Op&& op)
        {
            T temp = init;
            for (/* */; count-- != 0; (void) ++first, ++dest)
            {
                init = HPX_INVOKE(op, init, HPX_INVOKE(conv, *first));
                *dest = temp;
                temp = init;
            }
            return init;
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct transform_exclusive_scan
          : public algorithm<transform_exclusive_scan<IterPair>, IterPair>
        {
            constexpr transform_exclusive_scan() noexcept
              : algorithm<transform_exclusive_scan, IterPair>(
                    "transform_exclusive_scan")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent,
                typename Conv, typename T, typename OutIter, typename Op>
            static constexpr util::in_out_result<InIter, OutIter> sequential(
                ExPolicy, InIter first, Sent last, OutIter dest, Conv&& conv,
                T&& init, Op&& op)
            {
                return sequential_transform_exclusive_scan(first, last, dest,
                    HPX_FORWARD(Conv, conv), HPX_FORWARD(T, init),
                    HPX_FORWARD(Op, op));
            }

            template <typename ExPolicy, typename FwdIter1, typename Sent,
                typename FwdIter2, typename Conv, typename T, typename Op>
            static util::detail::algorithm_result_t<ExPolicy,
                util::in_out_result<FwdIter1, FwdIter2>>
            parallel(ExPolicy&& policy, FwdIter1 first, Sent last,
                FwdIter2 dest, Conv&& conv, T&& init, Op&& op)
            {
                using result_type = util::in_out_result<FwdIter1, FwdIter2>;
                using result =
                    util::detail::algorithm_result<ExPolicy, result_type>;
                using zip_iterator =
                    hpx::util::zip_iterator<FwdIter1, FwdIter2>;
                using difference_type =
                    typename std::iterator_traits<FwdIter1>::difference_type;

                if (first == last)
                    return result::get(std::move(result_type{first, dest}));

                FwdIter1 last_iter = first;
                difference_type count =
                    detail::advance_and_get_distance(last_iter, last);

                FwdIter2 final_dest = dest;
                std::advance(final_dest, count);

                // The overall scan algorithm is performed by executing 2
                // subsequent parallel steps. The first calculates the scan
                // results for each partition and the second produces the
                // overall result

                using hpx::get;

                auto f3 = [op](zip_iterator part_begin, std::size_t part_size,
                              T val) mutable -> void {
                    FwdIter2 dst = get<1>(part_begin.get_iterator_tuple());
                    *dst++ = val;

                    util::const_loop_n<std::decay_t<ExPolicy>>(
                        dst, part_size - 1, [&op, &val](FwdIter2 it) -> void {
                            *it = HPX_INVOKE(op, val, *it);
                        });
                };

                return util::scan_partitioner<ExPolicy, result_type, T>::call(
                    HPX_FORWARD(ExPolicy, policy), zip_iterator(first, dest),
                    count, init,
                    // step 1 performs first part of scan algorithm
                    [op, conv](zip_iterator part_begin,
                        std::size_t part_size) mutable -> T {
                        T part_init = HPX_INVOKE(conv, get<0>(*part_begin++));

                        auto iters = part_begin.get_iterator_tuple();
                        return sequential_transform_exclusive_scan_n(
                            get<0>(iters), part_size - 1, get<1>(iters), conv,
                            part_init, op);
                    },
                    // step 2 propagates the partition results from left
                    // to right
                    op,
                    // step 3 runs final accumulation on each partition
                    HPX_MOVE(f3),
                    // use this return value
                    [last_iter, final_dest](std::vector<T>&&,
                        std::vector<hpx::future<void>>&& data) -> result_type {
                        // make sure iterators embedded in function object that is
                        // attached to futures are invalidated
                        util::detail::clear_container(data);

                        return result_type{last_iter, final_dest};
                    });
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::transform_exclusive_scan
    HPX_CXX_CORE_EXPORT inline constexpr struct transform_exclusive_scan_t final
      : hpx::detail::tag_parallel_algorithm<transform_exclusive_scan_t>
    {
        template <typename InIter, typename OutIter, typename BinOp,
            typename UnOp,
            typename T = typename std::iterator_traits<InIter>::value_type>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter> &&
                hpx::traits::is_iterator_v<OutIter> &&
                hpx::is_invocable_v<UnOp,
                    typename std::iterator_traits<InIter>::value_type> &&
                hpx::is_invocable_v<BinOp,
                    hpx::util::invoke_result_t<UnOp,
                        typename std::iterator_traits<InIter>::value_type>,
                    hpx::util::invoke_result_t<UnOp,
                        typename std::iterator_traits<InIter>::value_type>
                >
            )
        // clang-format on
        friend OutIter tag_fallback_invoke(hpx::transform_exclusive_scan_t,
            InIter first, InIter last, OutIter dest, T init, BinOp binary_op,
            UnOp unary_op)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");
            static_assert(std::output_iterator<OutIter,
                              hpx::traits::iter_value_t<InIter>>,
                "Requires at least output iterator.");

            using result_type = parallel::util::in_out_result<InIter, OutIter>;

            return parallel::util::get_second_element(
                hpx::parallel::detail::transform_exclusive_scan<result_type>()
                    .call(hpx::execution::seq, first, last, dest,
                        HPX_MOVE(unary_op), HPX_MOVE(init),
                        HPX_MOVE(binary_op)));
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename BinOp, typename UnOp,
            typename T = typename std::iterator_traits<FwdIter1>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<UnOp,
                    typename std::iterator_traits<FwdIter1>::value_type> &&
                hpx::is_invocable_v<BinOp,
                    hpx::util::invoke_result_t<UnOp,
                        typename std::iterator_traits<FwdIter1>::value_type>,
                    hpx::util::invoke_result_t<UnOp,
                        typename std::iterator_traits<FwdIter1>::value_type>
                >
            )
        // clang-format on
        friend parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
        tag_fallback_invoke(hpx::transform_exclusive_scan_t, ExPolicy&& policy,
            FwdIter1 first, FwdIter1 last, FwdIter2 dest, T init,
            BinOp binary_op, UnOp unary_op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            using result_type =
                parallel::util::in_out_result<FwdIter1, FwdIter2>;

            return parallel::util::get_second_element(
                hpx::parallel::detail::transform_exclusive_scan<result_type>()
                    .call(HPX_FORWARD(ExPolicy, policy), first, last, dest,
                        HPX_MOVE(unary_op), HPX_MOVE(init),
                        HPX_MOVE(binary_op)));
        }
    } transform_exclusive_scan{};
}    // namespace hpx

#endif    // DOXYGEN
