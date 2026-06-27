//  Copyright (c) 2022 Bhumit Attarde
//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2020-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/count.hpp
/// \page hpx::count, hpx::count_if
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Returns the number of elements in the range [first, last) satisfying
    /// a specific criteria. This version counts the elements that are equal to
    /// the given \a value. Executed according to the policy.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first comparisons.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the comparisons.
    /// \tparam FwdIter    The type of the source iterator used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam T           The type of the value to search for (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param value        The value to search for.
    ///
    /// The comparisons in the parallel \a count algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// \note The comparisons in the parallel \a count algorithm invoked with
    ///       an execution policy object of type \a parallel_policy or
    ///       \a parallel_task_policy are permitted to execute in an unordered
    ///       fashion in unspecified threads, and indeterminately sequenced
    ///       within each thread.
    ///
    /// \returns  The \a count algorithm returns a
    ///           \a hpx::future<difference_type> if the execution policy is of
    ///           type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a difference_type otherwise (where \a difference_type
    ///           is defined by \a std::iterator_traits<FwdIterB>::difference_type.
    ///           The \a count algorithm returns the number of elements
    ///           satisfying the given criteria.
    template <typename ExPolicy, typename FwdIter, typename T>
    typename util::detail::algorithm_result<ExPolicy,
        typename std::iterator_traits<FwdIter>::difference_type>::type
    count(ExPolicy&& policy, FwdIter first, FwdIter last, T const& value);

    /// Returns the number of elements in the range [first, last) satisfying
    /// a specific criteria. This version counts the elements that are equal to
    /// the given \a value.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first comparisons.
    /// \tparam InIter      The type of the source iterator used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam T           The type of the value to search for (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param value        The value to search for.
    ///
    ///
    /// \returns  The \a count algorithm returns a \a difference_type (where \a difference_type
    ///           is defined by \a std::iterator_traits<InIter>::difference_type.
    ///           The \a count algorithm returns the number of elements
    ///           satisfying the given criteria.
    ///
    template <typename InIter, typename T>
    typename std::iterator_traits<InIter>::difference_type
    count(InIter first, InIter last, T const& value);

    /// Returns the number of elements in the range [first, last) satisfying
    /// a specific criteria. This version counts elements for which predicate
    /// \a f returns true. Executed according to the policy.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first applications of
    ///         the predicate.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the comparisons.
    /// \tparam FwdIter     The type of the source begin iterator used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a count_if requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     required elements. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// \note The assignments in the parallel \a count_if algorithm invoked with
    ///       an execution policy object of type \a sequenced_policy
    ///       execute in sequential order in the calling thread.
    /// \note The assignments in the parallel \a count_if algorithm invoked with
    ///       an execution policy object of type \a parallel_policy or
    ///       \a parallel_task_policy are permitted to execute in an unordered
    ///       fashion in unspecified threads, and indeterminately sequenced
    ///       within each thread.
    ///
    /// \returns  The \a count_if algorithm returns
    ///           \a hpx::future<difference_type> if the execution policy is of
    ///           type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a difference_type otherwise (where \a difference_type
    ///           is defined by \a std::iterator_traits<FwdIter>::difference_type.
    ///           The \a count algorithm returns the number of elements
    ///           satisfying the given criteria.
    ///
    template <typename ExPolicy, typename FwdIter, typename F>
    typename util::detail::algorithm_result<ExPolicy,
        typename std::iterator_traits<FwdIter>::difference_type>::type
    count_if(ExPolicy&& policy, FwdIter first, FwdIter last, F&& f);

    /// Returns the number of elements in the range [first, last) satisfying
    /// a specific criteria. This version counts elements for which predicate
    /// \a f returns true.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first applications of
    ///         the predicate.
    ///
    /// \tparam InIter      The type of the source begin iterator used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a count_if requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     required elements. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a InIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// \returns  The \a count_if algorithm returns \a difference_type (where
    ///           a difference_type is defined by
    ///           \a std::iterator_traits<InIter>::difference_type.
    ///           The \a count algorithm returns the number of elements
    ///           satisfying the given criteria.
    ///
    template <typename InIter, typename F>
        typename std::iterator_traits<InIter>::difference_type
    count_if(InIter first, InIter last, F&& f);

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
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // count
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Op,
            typename Proj>
        struct count_iteration
        {
            using execution_policy_type = std::decay_t<ExPolicy>;
            using proj_type = std::decay_t<Proj>;
            using op_type = std::decay_t<Op>;

            op_type op_;
            proj_type proj_;

            template <typename Op_, typename Proj_,
                typename U = std::enable_if_t<
                    !std::is_same_v<std::decay_t<Op_>, count_iteration>>>
            HPX_HOST_DEVICE count_iteration(Op_&& op, Proj_&& proj)
              : op_(HPX_FORWARD(Op_, op))
              , proj_(HPX_FORWARD(Proj_, proj))
            {
            }

#if !defined(__NVCC__) && !defined(__CUDACC__)
            count_iteration(count_iteration const&) = default;
            count_iteration(count_iteration&&) = default;
#else
            HPX_HOST_DEVICE count_iteration(count_iteration const& rhs)
              : op_(rhs.op_)
              , proj_(rhs.proj_)
            {
            }

            HPX_HOST_DEVICE count_iteration(count_iteration&& rhs)
              : op_(HPX_MOVE(rhs.op_))
              , proj_(HPX_MOVE(rhs.proj_))
            {
            }
#endif

            count_iteration& operator=(count_iteration const&) = default;
            count_iteration& operator=(count_iteration&&) = default;

            template <typename Iter>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr std::iterator_traits<
                Iter>::difference_type
            operator()(Iter part_begin, std::size_t part_size)
            {
                typename std::iterator_traits<Iter>::difference_type ret = 0;
                util::const_loop_n<execution_policy_type>(part_begin, part_size,
                    hpx::bind_back(*this, std::ref(ret)));
                return ret;
            }

            template <typename Iter>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
                Iter curr, std::iterator_traits<Iter>::difference_type& ret)
            {
                ret += traits::count_bits(
                    HPX_INVOKE(op_, HPX_INVOKE(proj_, *curr)));
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Value>
        struct count : public algorithm<count<Value>, Value>
        {
            using difference_type = Value;

            constexpr count() noexcept
              : algorithm<count, Value>("count")
            {
            }

            template <typename ExPolicy, typename InIterB, typename InIterE,
                typename T, typename Proj>
            static difference_type sequential(ExPolicy&& policy, InIterB first,
                InIterE last, T const& value, Proj&& proj)
            {
                auto f1 =
                    count_iteration<ExPolicy, detail::compare_to<T>, Proj>(
                        detail::compare_to<T>(value), HPX_FORWARD(Proj, proj));

                typename std::iterator_traits<InIterB>::difference_type ret = 0;

                util::const_loop(HPX_FORWARD(ExPolicy, policy), first, last,
                    hpx::bind_back(HPX_MOVE(f1), std::ref(ret)));

                return ret;
            }

            template <typename ExPolicy, typename IterB, typename IterE,
                typename T, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, IterB first,
                IterE last, T const& value, Proj&& proj)
            {
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return util::detail::
                            algorithm_result<ExPolicy, difference_type>::get(
                                static_cast<difference_type>(0));
                    }
                }

                auto f1 =
                    count_iteration<ExPolicy, detail::compare_to<T>, Proj>(
                        detail::compare_to<T>(value), HPX_FORWARD(Proj, proj));

                return util::partitioner<ExPolicy, difference_type>::call(
                    HPX_FORWARD(ExPolicy, policy), first,
                    detail::distance(first, last), HPX_MOVE(f1),
                    hpx::unwrapping([](auto&& results, auto&&...) {
                        return util::accumulate_n(hpx::util::begin(results),
                            hpx::util::size(results), difference_type(0),
                            std::plus<difference_type>());
                    }));
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // count_if
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename Value>
        struct count_if : public algorithm<count_if<Value>, Value>
        {
            using difference_type = Value;

            constexpr count_if() noexcept
              : algorithm<count_if, Value>("count_if")
            {
            }

            template <typename ExPolicy, typename InIterB, typename InIterE,
                typename Pred, typename Proj>
            static constexpr difference_type sequential(ExPolicy&& policy,
                InIterB first, InIterE last, Pred&& op, Proj&& proj)
            {
                auto f1 = count_iteration<ExPolicy, Pred, Proj>(
                    op, HPX_FORWARD(Proj, proj));

                typename std::iterator_traits<InIterB>::difference_type ret = 0;

                util::const_loop(HPX_FORWARD(ExPolicy, policy), first, last,
                    hpx::bind_back(HPX_MOVE(f1), std::ref(ret)));

                return ret;
            }

            template <typename ExPolicy, typename IterB, typename IterE,
                typename Pred, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, IterB first,
                IterE last, Pred&& op, Proj&& proj)
            {
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return util::detail::
                            algorithm_result<ExPolicy, difference_type>::get(
                                static_cast<difference_type>(0));
                    }
                }

                auto f1 = count_iteration<ExPolicy, Pred, Proj>(
                    op, HPX_FORWARD(Proj, proj));

                return util::partitioner<ExPolicy, difference_type>::call(
                    HPX_FORWARD(ExPolicy, policy), first,
                    detail::distance(first, last), HPX_MOVE(f1),
                    hpx::unwrapping([](auto&& results, auto&&...) {
                        return util::accumulate_n(hpx::util::begin(results),
                            hpx::util::size(results), difference_type(0),
                            std::plus<difference_type>());
                    }));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::count
    HPX_CXX_CORE_EXPORT inline constexpr struct count_t final
      : hpx::detail::tag_parallel_algorithm<count_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
            requires(hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>)
        friend decltype(auto) tag_fallback_invoke(count_t, ExPolicy&& policy,
            FwdIter first, FwdIter last, T const& value)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            using difference_type =
                std::iterator_traits<FwdIter>::difference_type;

            return hpx::parallel::detail::count<difference_type>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, value,
                hpx::identity_v);
        }

        template <typename InIter,
            typename T = typename std::iterator_traits<InIter>::value_type>
            requires(hpx::traits::is_iterator_v<InIter>)
        friend decltype(auto) tag_fallback_invoke(
            count_t, InIter first, InIter last, T const& value)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");

            using difference_type =
                std::iterator_traits<InIter>::difference_type;

            return hpx::parallel::detail::count<difference_type>().call(
                hpx::execution::seq, first, last, value, hpx::identity_v);
        }
    } count{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::count_if
    HPX_CXX_CORE_EXPORT inline constexpr struct count_if_t final
      : hpx::detail::tag_parallel_algorithm<count_if_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter, typename F>
        // clang-format off
        requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::is_invocable_v<F,
                    typename std::iterator_traits<FwdIter>::value_type
                >
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(
            count_if_t, ExPolicy&& policy, FwdIter first, FwdIter last, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            using difference_type =
                std::iterator_traits<FwdIter>::difference_type;

            return hpx::parallel::detail::count_if<difference_type>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(f),
                hpx::identity_v);
        }

        template <typename InIter, typename F>
            requires(hpx::traits::is_iterator_v<InIter> &&
                hpx::is_invocable_v<F,
                    typename std::iterator_traits<InIter>::value_type>)
        friend decltype(auto) tag_fallback_invoke(
            count_if_t, InIter first, InIter last, F f)
        {
            static_assert(std::input_iterator<InIter>,
                "Required at least input iterator.");

            using difference_type =
                std::iterator_traits<InIter>::difference_type;

            return hpx::parallel::detail::count_if<difference_type>().call(
                hpx::execution::seq, first, last, HPX_MOVE(f), hpx::identity_v);
        }
    } count_if{};
}    // namespace hpx

#endif    // DOXYGEN
