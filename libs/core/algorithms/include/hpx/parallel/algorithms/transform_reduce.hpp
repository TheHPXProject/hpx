//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2017-2023 Hartmut Kaiser
//  Copyright (c) 2022 Bhumit Attarde
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/transform_reduce.hpp
/// \page hpx::transform_reduce
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Returns GENERALIZED_SUM(red_op, init, conv_op(*first), ...,
    /// conv_op(*(first + (last - first) - 1))). Executed according to the
    /// policy.
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicates \a red_op and \a conv_op.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for each of the values returned
    ///                     from the invocation of \a conv_op. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1, \a Type2, and \a Ret must be
    ///                     such that an object of a type as returned from
    ///                     \a conv_op can be implicitly converted to any
    ///                     of those types.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is a
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     R fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///                     The type \a R must be such that an object of this
    ///                     type can be implicitly converted to \a T.
    ///
    /// The reduce operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The reduce operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a hpx::future<T> if the
    ///           execution policy is of type \a parallel_task_policy and
    ///           returns \a T otherwise.
    ///           The \a transform_reduce algorithm returns the result of the
    ///           generalized sum over the values returned from \a conv_op when
    ///           applied to the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a transform_reduce and \a accumulate is
    /// that the behavior of transform_reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename ExPolicy, typename FwdIter, typename T, typename Reduce,
        typename Convert>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
    transform_reduce(ExPolicy&& policy, FwdIter first, FwdIter last, T init,
        Reduce&& red_op, Convert&& conv_op);

    /// Returns GENERALIZED_SUM(red_op, init, conv_op(*first), ...,
    /// conv_op(*(first + (last - first) - 1))).
    ///
    /// \note   Complexity: O(\a last - \a first) applications of the
    ///         predicates \a red_op and \a conv_op.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam T           The type of the value to be used as initial (and
    ///                     intermediate) values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the reduction operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param init         The initial value for the generalized sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for each of the values returned
    ///                     from the invocation of \a conv_op. This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The types \a Type1, \a Type2, and \a Ret must be
    ///                     such that an object of a type as returned from
    ///                     \a conv_op can be implicitly converted to any
    ///                     of those types.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last). This is a
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     R fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a InIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///                     The type \a R must be such that an object of this
    ///                     type can be implicitly converted to \a T.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a T.
    ///           The \a transform_reduce algorithm returns the result of the
    ///           generalized sum over the values returned from \a conv_op when
    ///           applied to the elements given by the input range
    ///           [first, last).
    ///
    /// \note   GENERALIZED_SUM(op, a1, ..., aN) is defined as follows:
    ///         * a1 when N is 1
    ///         * op(GENERALIZED_SUM(op, b1, ..., bK), GENERALIZED_SUM(op, bM, ..., bN)),
    ///           where:
    ///           * b1, ..., bN may be any permutation of a1, ..., aN and
    ///           * 1 < K+1 = M <= N.
    ///
    /// The difference between \a transform_reduce and \a accumulate is
    /// that the behavior of \a transform_reduce may be non-deterministic for
    /// non-associative or non-commutative binary predicate.
    ///
    template <typename InIter, typename T, typename Reduce, typename Convert>
    T transform_reduce(InIter first, InIter last, T init, Reduce&& red_op,
        Convert&& conv_op);

    ///////////////////////////////////////////////////////////////////////////
    /// Returns the result of accumulating init with the inner products of the
    /// pairs formed by the elements of two ranges starting at first1 and
    /// first2. Executed according to the policy.
    ///
    /// \note   Complexity: O(\a last - \a first) applications each of
    ///                     \a reduce and \a transform.
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the first source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of a forward iterator.
    /// \tparam FwdIter2    The type of the second source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of a forward iterator.
    /// \tparam T           The type of the value to be used as return)
    ///                     values (deduced).
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first1       Refers to the beginning of the first sequence of
    ///                     elements the result will be calculated with.
    /// \param last1        Refers to the end of the first sequence of elements
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the result will be calculated with.
    /// \param init         The initial value for the sum.
    ///
    /// The operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a hpx::future<T> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a T otherwise.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2, typename T>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
    transform_reduce(ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1,
        FwdIter2 first2, T init);

    /////////////////////////////////////////////////////////////////////////
    /// Returns the result of accumulating init with the inner products of the
    /// pairs formed by the elements of two ranges starting at first1 and
    /// first2.
    ///
    /// \note   Complexity: O(\a last - \a first) applications each of
    ///                     \a reduce and \a transform.
    ///
    /// \tparam InIter1     The type of the first source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an input iterator.
    /// \tparam InIter2     The type of the second source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an input iterator.
    /// \tparam T           The type of the value to be used as return)
    ///                     values (deduced).
    /// \param first1       Refers to the beginning of the first sequence of
    ///                     elements the result will be calculated with.
    /// \param last1        Refers to the end of the first sequence of elements
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the result will be calculated with.
    /// \param init         The initial value for the sum.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a T.
    ///
    template <typename InIter1, typename InIter2, typename T>
    T transform_reduce(InIter1 first1, InIter1 last1, InIter2 first2, T init);

    ///////////////////////////////////////////////////////////////////////////
    /// Returns the result of accumulating init with the inner products of the
    /// pairs formed by the elements of two ranges starting at first1 and
    /// first2. Executed according to the policy.
    ///
    /// \note   Complexity: O(\a last - \a first) applications each of
    ///                     \a reduce and \a transform.
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the first source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of a forward iterator.
    /// \tparam FwdIter2    The type of the second source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of a forward iterator.
    /// \tparam T           The type of the value to be used as return)
    ///                     values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the multiplication operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first1       Refers to the beginning of the first sequence of
    ///                     elements the result will be calculated with.
    /// \param last1        Refers to the end of the first sequence of elements
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the result will be calculated with.
    /// \param init         The initial value for the sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for the initial value and each
    ///                     of the return values of \a conv_op.
    ///                     This is a binary predicate. The
    ///                     signature of this predicate should be equivalent to
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The type \a Ret must be
    ///                     such that it can be implicitly converted to a type
    ///                     of \a T.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the input values
    ///                     of the sequence. This is a binary predicate. The
    ///                     signature of this predicate should be equivalent to
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The type \a Ret must be such that it can be
    ///                     implicitly converted to an object for the second
    ///                     argument type of \a red_op.
    ///
    /// The operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The operations in the parallel \a transform_reduce algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a hpx::future<T>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a T otherwise.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename T, typename Reduce, typename Convert>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, T>
    transform_reduce(ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1,
        FwdIter2 first2, T init, Reduce&& red_op, Convert&& conv_op);

    ///////////////////////////////////////////////////////////////////////////
    /// Returns the result of accumulating init with the inner products of the
    /// pairs formed by the elements of two ranges starting at first1 and
    /// first2.
    ///
    /// \note   Complexity: O(\a last - \a first) applications each of
    ///                     \a reduce and \a transform.
    /// \tparam InIter1     The type of the first source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an input iterator.
    /// \tparam InIter2     The type of the second source iterators used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an input iterator.
    /// \tparam T           The type of the value to be used as return)
    ///                     values (deduced).
    /// \tparam Reduce      The type of the binary function object used for
    ///                     the multiplication operation.
    /// \tparam Convert     The type of the unary function object used to
    ///                     transform the elements of the input sequence before
    ///                     invoking the reduce function.
    ///
    /// \param first1       Refers to the beginning of the first sequence of
    ///                     elements the result will be calculated with.
    /// \param last1        Refers to the end of the first sequence of elements
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the result will be calculated with.
    /// \param init         The initial value for the sum.
    /// \param red_op       Specifies the function (or function object) which
    ///                     will be invoked for the initial value and each
    ///                     of the return values of \a conv_op.
    ///                     This is a binary predicate. The
    ///                     signature of this predicate should be equivalent to
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The type \a Ret must be
    ///                     such that it can be implicitly converted to a type
    ///                     of \a T.
    /// \param conv_op      Specifies the function (or function object) which
    ///                     will be invoked for each of the input values
    ///                     of the sequence. This is a binary predicate. The
    ///                     signature of this predicate should be equivalent to
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it.
    ///                     The type \a Ret must be such that it can be
    ///                     implicitly converted to an object for the second
    ///                     argument type of \a red_op.
    ///
    /// \returns  The \a transform_reduce algorithm returns a \a T.
    ///
    template <typename InIter1, typename InIter2,
        typename T, typename Reduce, typename Convert>
    T transform_reduce(ExPolicy&& policy, InIter1 first1, InIter1 last1,
        InIter2 first2, T init, Reduce&& red_op, Convert&& conv_op);

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/reduce.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // transform_reduce
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        struct transform_reduce : public algorithm<transform_reduce<T>, T>
        {
            constexpr transform_reduce() noexcept
              : algorithm<transform_reduce, T>("transform_reduce")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename T_, typename Reduce, typename Convert>
            static constexpr T sequential(ExPolicy&& policy, Iter first,
                Sent last, T_&& init, Reduce&& r, Convert&& conv)
            {
                return detail::sequential_reduce<ExPolicy>(
                    HPX_FORWARD(ExPolicy, policy), first, last,
                    HPX_FORWARD(T_, init), HPX_FORWARD(Reduce, r),
                    HPX_FORWARD(Convert, conv));
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename T_, typename Reduce, typename Convert>
            static decltype(auto) parallel(ExPolicy&& policy, Iter first,
                Sent last, T_&& init, Reduce&& r, Convert&& conv)
            {
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        T init_ = init;
                        return util::detail::algorithm_result<ExPolicy, T>::get(
                            HPX_MOVE(init_));
                    }
                }

                auto f1 = [r, conv](
                              Iter part_begin, std::size_t part_size) mutable {
                    auto val = HPX_INVOKE(conv, *part_begin);
                    return detail::sequential_reduce<ExPolicy>(
                        ++part_begin, --part_size, HPX_MOVE(val), r, conv);
                };

                return util::partitioner<ExPolicy, T>::call(
                    HPX_FORWARD(ExPolicy, policy), first,
                    detail::distance(first, last), HPX_MOVE(f1),
                    hpx::unwrapping([init = HPX_FORWARD(T_, init),
                                        r = HPX_FORWARD(Reduce, r)](
                                        auto&& results) mutable -> T {
                        return detail::sequential_reduce<ExPolicy>(
                            hpx::util::begin(results), hpx::util::size(results),
                            init, r);
                    }));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // transform_reduce_binary
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        struct transform_reduce_binary
          : public algorithm<transform_reduce_binary<T>, T>
        {
            constexpr transform_reduce_binary() noexcept
              : algorithm<transform_reduce_binary, T>("transform_reduce_binary")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename Iter2, typename T_, typename Op1, typename Op2>
            static constexpr T sequential(ExPolicy&& /* policy */, Iter first1,
                Sent last1, Iter2 first2, T_ init, Op1&& op1, Op2&& op2)
            {
                return detail::sequential_reduce<ExPolicy>(first1, last1,
                    first2, HPX_FORWARD(T_, init), HPX_FORWARD(Op1, op1),
                    HPX_FORWARD(Op2, op2));
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename Iter2, typename T_, typename Op1, typename Op2>
            static decltype(auto) parallel(ExPolicy&& policy, Iter first1,
                Sent last1, Iter2 first2, T_&& init, Op1&& op1, Op2&& op2)
            {
                using result = util::detail::algorithm_result<ExPolicy, T>;
                using zip_iterator = hpx::util::zip_iterator<Iter, Iter2>;
                using difference_type =
                    typename std::iterator_traits<Iter>::difference_type;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first1 == last1)
                    {
                        T init_ = init;
                        return result::get(HPX_MOVE(init_));
                    }
                }

                difference_type count = detail::distance(first1, last1);

                auto f1 = [op1, op2 = HPX_FORWARD(Op2, op2)](
                              zip_iterator part_begin,
                              std::size_t part_size) mutable -> T {
                    auto iters = part_begin.get_iterator_tuple();
                    Iter it1 = hpx::get<0>(iters);
                    Iter2 it2 = hpx::get<1>(iters);

                    Iter last = it1;
                    std::advance(last, part_size);

                    auto&& r = HPX_INVOKE(op2, *it1, *it2);
                    ++it1;
                    ++it2;

                    return detail::sequential_reduce<ExPolicy>(it1, last, it2,
                        HPX_MOVE(r), HPX_FORWARD(Op1, op1),
                        HPX_FORWARD(Op2, op2));
                };

                return util::partitioner<ExPolicy, T>::call(
                    HPX_FORWARD(ExPolicy, policy), zip_iterator(first1, first2),
                    count, HPX_MOVE(f1),
                    [init = HPX_FORWARD(T_, init), op1 = HPX_FORWARD(Op1, op1)](
                        auto&& results) mutable -> T {
                        T ret = HPX_MOVE(init);
                        for (auto&& result : results)
                        {
                            ret = HPX_INVOKE(
                                op1, HPX_MOVE(ret), hpx::unwrap(result));
                        }
                        return ret;
                    });
            }
        };
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::transform_reduce
    HPX_CXX_CORE_EXPORT inline constexpr struct transform_reduce_t final
      : hpx::detail::tag_dispatch<transform_reduce_t,
            hpx::detail::tag_parallel_algorithm<transform_reduce_t>>
    {
        template <typename ExPolicy, typename FwdIter, typename T,
            typename Reduce, typename Convert>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::is_invocable_v<Convert,
                   typename std::iterator_traits<FwdIter>::value_type> &&
                hpx::is_invocable_v<Reduce,
                   hpx::util::invoke_result_t<Convert,
                       typename std::iterator_traits<FwdIter>::value_type>,
                   hpx::util::invoke_result_t<Convert,
                       typename std::iterator_traits<FwdIter>::value_type>
                >
            )
        // clang-format on
        static decltype(auto) invoke_default(ExPolicy&& policy, FwdIter first,
            FwdIter last, T init, Reduce red_op, Convert conv_op)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::transform_reduce<T>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(init),
                HPX_MOVE(red_op), HPX_MOVE(conv_op));
        }

        template <typename InIter, typename T, typename Reduce,
            typename Convert>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter> &&
                hpx::is_invocable_v<Convert,
                   typename std::iterator_traits<InIter>::value_type> &&
                hpx::is_invocable_v<Reduce,
                   hpx::util::invoke_result_t<Convert,
                       typename std::iterator_traits<InIter>::value_type>,
                   hpx::util::invoke_result_t<Convert,
                       typename std::iterator_traits<InIter>::value_type>
                >
            )
        // clang-format on
        static T invoke_default(
            InIter first, InIter last, T init, Reduce red_op, Convert conv_op)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::transform_reduce<T>().call(
                hpx::execution::seq, first, last, HPX_MOVE(init),
                HPX_MOVE(red_op), HPX_MOVE(conv_op));
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename T>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        static decltype(auto) invoke_default(ExPolicy&& policy, FwdIter1 first1,
            FwdIter1 last1, FwdIter2 first2, T init)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                HPX_FORWARD(ExPolicy, policy), first1, last1, first2,
                HPX_MOVE(init), hpx::parallel::detail::plus(),
                hpx::parallel::detail::multiplies());
        }

        template <typename InIter1, typename InIter2, typename T>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter1> &&
                hpx::traits::is_iterator_v<InIter2>
            )
        // clang-format on
        static T invoke_default(
            InIter1 first1, InIter1 last1, InIter2 first2, T init)
        {
            static_assert(std::input_iterator<InIter1>,
                "Requires at least input iterator.");
            static_assert(std::input_iterator<InIter2>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                hpx::execution::seq, first1, last1, first2, HPX_MOVE(init),
                hpx::parallel::detail::plus(),
                hpx::parallel::detail::multiplies());
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename T, typename Reduce, typename Convert>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Convert,
                    typename std::iterator_traits<FwdIter1>::value_type,
                    typename std::iterator_traits<FwdIter2>::value_type> &&
                hpx::is_invocable_v<Reduce,
                    hpx::util::invoke_result_t<Convert,
                        typename std::iterator_traits<FwdIter1>::value_type,
                        typename std::iterator_traits<FwdIter2>::value_type>,
                    hpx::util::invoke_result_t<Convert,
                        typename std::iterator_traits<FwdIter1>::value_type,
                        typename std::iterator_traits<FwdIter2>::value_type>
                >
            )
        // clang-format on
        static decltype(auto) invoke_default(ExPolicy&& policy, FwdIter1 first1,
            FwdIter1 last1, FwdIter2 first2, T init, Reduce red_op,
            Convert conv_op)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2>,
                "Requires at least forward iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                HPX_FORWARD(ExPolicy, policy), first1, last1, first2,
                HPX_MOVE(init), HPX_MOVE(red_op), HPX_MOVE(conv_op));
        }

        template <typename InIter1, typename InIter2, typename T,
            typename Reduce, typename Convert>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter1> &&
                hpx::traits::is_iterator_v<InIter2> &&
                hpx::is_invocable_v<Convert,
                    typename std::iterator_traits<InIter1>::value_type,
                    typename std::iterator_traits<InIter2>::value_type> &&
                hpx::is_invocable_v<Reduce,
                    hpx::util::invoke_result_t<Convert,
                        typename std::iterator_traits<InIter1>::value_type,
                        typename std::iterator_traits<InIter2>::value_type>,
                    hpx::util::invoke_result_t<Convert,
                        typename std::iterator_traits<InIter1>::value_type,
                        typename std::iterator_traits<InIter2>::value_type>
                >
            )
        // clang-format on
        static T invoke_default(InIter1 first1, InIter1 last1, InIter2 first2,
            T init, Reduce red_op, Convert conv_op)
        {
            static_assert(std::input_iterator<InIter1>,
                "Requires at least input iterator.");
            static_assert(std::input_iterator<InIter2>,
                "Requires at least input iterator.");

            return hpx::parallel::detail::transform_reduce_binary<T>().call(
                hpx::execution::seq, first1, last1, first2, HPX_MOVE(init),
                HPX_MOVE(red_op), HPX_MOVE(conv_op));
        }
    } transform_reduce{};
}    // namespace hpx

#endif    // DOXYGEN
