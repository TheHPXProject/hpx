//  Copyright (c) 2017 Taeguk Kwon
//  Copyright (c) 2021 Giannis Gonidelis
//  Copyright (c) 2017-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/remove.hpp
/// \page hpx::remove, hpx::remove_if
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {

    /////////////////////////////////////////////////////////////////////////////
    /// Removes all elements satisfying specific criteria from the range
    /// [first, last) and returns a past-the-end iterator for the new
    /// end of the range. This version removes all elements that are
    /// equal to \a value.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of
    ///         the operator==().
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam T           The type of the value to remove (deduced).
    ///                     This value type must meet the requirements of
    ///                     \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param value        Specifies the value of elements to remove.
    ///
    /// The assignments in the parallel \a remove algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a remove algorithm returns a \a FwdIter.
    ///           The \a remove algorithm returns the iterator to the new end
    ///           of the range.
    ///
    template <typename FwdIter,
        typename T = typename std::iterator_traits<FwdIter>::value_type>
    FwdIter remove(FwdIter first, FwdIter last, T const& value);

    /////////////////////////////////////////////////////////////////////////////
    /// Removes all elements satisfying specific criteria from the range
    /// [first, last) and returns a past-the-end iterator for the new
    /// end of the range. This version removes all elements that are
    /// equal to \a value. Executed according to the policy.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of
    ///         the operator==().
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam T           The type of the value to remove (deduced).
    ///                     This value type must meet the requirements of
    ///                     \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param value        Specifies the value of elements to remove.
    ///
    /// The assignments in the parallel \a remove algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a remove algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a remove algorithm returns a \a hpx::future<FwdIter>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a remove algorithm returns the iterator to the new end
    ///           of the range.
    ///
    template <typename ExPolicy, typename FwdIter,
        typename T = typename std::iterator_traits<FwdIter>::value_type>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter> remove(
        ExPolicy&& policy, FwdIter first, FwdIter last, T const& value);

    /// Removes all elements satisfying specific criteria from the range
    /// [first, last) and returns a past-the-end iterator for the new
    /// end of the range. This version removes all elements for which predicate
    /// \a pred returns true.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of
    ///         the predicate \a pred.
    ///
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a remove_if requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param pred         Specifies the function (or function object) which
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
    /// The assignments in the parallel \a remove_if algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a remove_if algorithm returns a \a FwdIter.
    ///           The \a remove_if algorithm returns the iterator to the new end
    ///           of the range.
    ///
    template <typename FwdIter, typename Pred>
    FwdIter remove_if(FwdIter first, FwdIter last, Pred&& pred);

    /// Removes all elements satisfying specific criteria from the range
    /// [first, last) and returns a past-the-end iterator for the new
    /// end of the range. This version removes all elements for which predicate
    /// \a pred returns true. Executed according to the policy.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of
    ///         the predicate \a pred.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a remove_if requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param pred         Specifies the function (or function object) which
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
    /// The assignments in the parallel \a remove_if algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a remove_if algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a remove_if algorithm returns a \a hpx::future<FwdIter>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a remove_if algorithm returns the iterator to the new end
    ///           of the range.
    ///
    template <typename ExPolicy, typename FwdIter, typename Pred>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    remove_if(ExPolicy&& policy, FwdIter first, FwdIter last, Pred&& pred);

}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/find.hpp>
#include <hpx/parallel/algorithms/detail/remove.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    /////////////////////////////////////////////////////////////////////////////
    // remove_if
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename FwdIter>
        struct remove_if : public algorithm<remove_if<FwdIter>, FwdIter>
        {
            constexpr remove_if() noexcept
              : algorithm<remove_if, FwdIter>("remove_if")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename Pred, typename Proj>
            static constexpr Iter sequential(ExPolicy&& policy, Iter first,
                Sent last, Pred&& pred, Proj&& proj)
            {
                return sequential_remove_if<ExPolicy>(
                    HPX_FORWARD(ExPolicy, policy), first, last,
                    HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename Pred, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, Iter first,
                Sent last, Pred&& pred, Proj&& proj)
            {
                using inner_policy_type = std::decay_t<ExPolicy>;
                constexpr bool vectorpack_policy =
                    hpx::is_vectorpack_execution_policy_v<inner_policy_type>;

                if constexpr (vectorpack_policy)
                {
                    return sequential_remove_if<ExPolicy>(
                        HPX_FORWARD(ExPolicy, policy), first, last,
                        HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
                }
                else
                {
                    using zip_iterator = hpx::util::zip_iterator<Iter, bool*>;
                    using algorithm_result =
                        util::detail::algorithm_result<ExPolicy, Iter>;
                    using difference_type =
                        typename std::iterator_traits<Iter>::difference_type;
                    constexpr bool has_scheduler_executor =
                        hpx::execution_policy_has_scheduler_executor_v<
                            ExPolicy>;

                    difference_type count = detail::distance(first, last);

                    if constexpr (!has_scheduler_executor)
                    {
                        if (count == 0)
                            return algorithm_result::get(HPX_MOVE(first));
                    }
                    std::shared_ptr<bool[]> flags(new bool[count]);

                    using hpx::get;

                    // Note: replacing the invoke() with HPX_INVOKE()
                    // below makes gcc generate errors
                    auto f1 = [pred = HPX_FORWARD(Pred, pred),
                                  proj = HPX_FORWARD(Proj, proj)](
                                  zip_iterator part_begin,
                                  std::size_t part_size) -> void {
                        // MSVC complains if pred or proj is captured by ref below
                        util::const_loop_n<hpx::execution::sequenced_policy>(
                            part_begin, part_size,
                            [pred, proj](zip_iterator it) mutable {
                                get<1>(*it) = hpx::invoke(
                                    pred, hpx::invoke(proj, get<0>(*it)));
                            });
                    };

                    auto f2 = [flags, first, count](auto&&...) mutable -> Iter {
                        auto part_begin = zip_iterator(first, flags.get());
                        auto dest = first;
                        auto part_size = count;

                        if (dest == get<0>(part_begin.get_iterator_tuple()))
                        {
                            // Self-assignment must be detected.
                            util::const_loop_n<
                                hpx::execution::sequenced_policy>(part_begin,
                                part_size, [&dest](zip_iterator it) {
                                    if (!get<1>(*it))
                                    {
                                        if (dest !=
                                            get<0>(it.get_iterator_tuple()))
                                            *dest++ =
                                                std::ranges::iter_move(get<0>(
                                                    it.get_iterator_tuple()));
                                        else
                                            ++dest;
                                    }
                                });
                        }
                        else
                        {
                            // Self-assignment can't be performed.
                            util::const_loop_n<
                                hpx::execution::sequenced_policy>(part_begin,
                                part_size, [&dest](zip_iterator it) {
                                    if (!get<1>(*it))
                                        *dest++ = std::ranges::iter_move(
                                            get<0>(it.get_iterator_tuple()));
                                });
                        }
                        return dest;
                    };

                    return util::partitioner<ExPolicy, Iter, void>::call(
                        HPX_FORWARD(ExPolicy, policy),
                        zip_iterator(first, flags.get()), count, HPX_MOVE(f1),
                        HPX_MOVE(f2));
                }
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::remove_if
    HPX_CXX_CORE_EXPORT inline constexpr struct remove_if_t final
      : hpx::detail::tag_parallel_algorithm<remove_if_t>
    {
        template <typename FwdIter, typename Pred>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter>::value_type
                >
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            hpx::remove_if_t, FwdIter first, FwdIter last, Pred pred)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::remove_if<FwdIter>().call(
                hpx::execution::sequenced_policy{}, first, last, HPX_MOVE(pred),
                hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter, typename Pred>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                (
                    hpx::parallel::traits::is_indirect_callable_v<ExPolicy,
                        Pred, hpx::parallel::traits::projected<hpx::identity,
                                  FwdIter>
                    > ||
                    hpx::is_invocable_v<Pred,
                        typename std::iterator_traits<FwdIter>::value_type
                    >
                )
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::remove_if_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, Pred pred)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::remove_if<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(pred),
                hpx::identity_v);
        }
    } remove_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::remove
    HPX_CXX_CORE_EXPORT inline constexpr struct remove_t final
      : hpx::detail::tag_parallel_algorithm<remove_t>
    {
    private:
        template <typename FwdIter,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(
            hpx::remove_t, FwdIter first, FwdIter last, T const& value)
        {
            using Type = typename std::iterator_traits<FwdIter>::value_type;

            return hpx::remove_if(hpx::execution::seq, first, last,
                [value](Type const& a) -> bool { return value == a; });
        }

        template <typename ExPolicy, typename FwdIter,
            typename T = typename std::iterator_traits<FwdIter>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::remove_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, T const& value)
        {
            using Type = typename std::iterator_traits<FwdIter>::value_type;

            return hpx::remove_if(HPX_FORWARD(ExPolicy, policy), first, last,
                [value](
                    auto const& a) { return static_cast<Type>(value) == a; });
        }
    } remove{};
}    // namespace hpx

#endif    // DOXYGEN
