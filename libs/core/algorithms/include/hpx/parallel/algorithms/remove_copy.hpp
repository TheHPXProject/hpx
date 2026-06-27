//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2021 Giannis Gonidelis
//  Copyright (c) 2017-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/remove_copy.hpp
/// \page hpx::remove_copy, hpx::remove_copy_if
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {

    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// comparison operator returns false when compare to value.
    /// The order of the elements that are not removed is preserved.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range [first,last) for which the following corresponding
    ///          conditions do not hold: *it == value
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a pred, here comparison operator.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam T           The type that the result of dereferencing FwdIter1 is
    ///                     compared to.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param value        Value to be removed.
    ///
    /// The assignments in the parallel \a remove_copy algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a remove_copy algorithm returns an
    ///           \a OutIter.
    ///           The \a remove_copy algorithm returns the
    ///           iterator to the element past the last element copied.
    ///
    template <typename InIter, typename OutIter,
        typename T = typename std::iterator_traits<InIter>::value_type>
    OutIter remove_copy(
        InIter first, InIter last, OutIter dest, T const& value);

    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// comparison operator returns false when compare to value.
    /// The order of the elements that are not removed is preserved.
    /// Executed according to the policy.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range [first,last) for which the following corresponding
    ///          conditions do not hold: *it == value
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a pred, here comparison operator.
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
    /// \tparam T           The type that the result of dereferencing FwdIter1 is
    ///                     compared to.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param value        Value to be removed.
    ///
    /// The assignments in the parallel \a remove_copy algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a remove_copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a remove_copy algorithm returns a
    ///           \a hpx::future<FwdIter2>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2 otherwise.
    ///           The \a remove_copy algorithm returns the
    ///           iterator to the element past the last element copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename T = typename std::iterator_traits<InIter>::value_type>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    remove_copy(ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest,
        T const& value);

    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// predicate \a pred returns false. The order of the elements that are not
    /// removed is preserved.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range [first,last) for which the following corresponding
    ///          conditions do not hold:
    ///          INVOKE(pred, *it) != false.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a pred.
    ///
    /// \tparam InIter      The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam OutIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     output iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements to be removed. The signature of this predicate
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
    /// The assignments in the parallel \a remove_copy_if algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a remove_copy_if algorithm returns an
    ///           \a OutIter
    ///           The \a remove_copy_if algorithm returns the
    ///           iterator to the element past the last element copied.
    ///
    template <typename InIter, typename OutIter, typename Pred>
    OutIter remove_copy_if(
        InIter first, InIter last, OutIter dest, Pred&& pred);

    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// predicate \a pred returns false. The order of the elements that are not
    /// removed is preserved.
    /// Executed according to the policy.
    ///
    /// Effects: Copies all the elements referred to by the iterator it in the
    ///          range [first,last) for which the following corresponding
    ///          conditions do not hold:
    ///          INVOKE(pred, *it) != false.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a pred.
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
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a remove_copy_if requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     elements to be removed. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The assignments in the parallel \a remove_copy_if algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a remove_copy_if algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a remove_copy_if algorithm returns a
    ///           \a hpx::future<FwdIter2>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2
    ///           otherwise.
    ///           The \a remove_copy_if algorithm returns the
    ///           iterator to the element past the last element copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename Pred>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    remove_copy_if(ExPolicy&& policy, FwdIter1 first, FwdIter1 last,
        FwdIter2 dest, Pred&& pred);

}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/copy.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    /////////////////////////////////////////////////////////////////////////////
    // remove_copy
    namespace detail {

        /// \cond NOINTERNAL

        // sequential remove_copy
        HPX_CXX_CORE_EXPORT template <typename InIter, typename Sent,
            typename OutIter, typename T, typename Proj>
        constexpr util::in_out_result<InIter, OutIter> sequential_remove_copy(
            InIter first, Sent last, OutIter dest, T const& value, Proj&& proj)
        {
            for (/* */; first != last; ++first)
            {
                if (!(HPX_INVOKE(proj, *first) == value))
                {
                    *dest++ = *first;
                }
            }
            return util::in_out_result<InIter, OutIter>{first, dest};
        }

        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct remove_copy : public algorithm<remove_copy<IterPair>, IterPair>
        {
            constexpr remove_copy() noexcept
              : algorithm<remove_copy, IterPair>("remove_copy")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent,
                typename OutIter, typename T, typename Proj>
            static constexpr util::in_out_result<InIter, OutIter> sequential(
                ExPolicy, InIter first, Sent last, OutIter dest, T const& val,
                Proj&& proj)
            {
                return sequential_remove_copy(
                    first, last, dest, val, HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename FwdIter1, typename Sent,
                typename FwdIter2, typename T, typename Proj>
            static util::detail::algorithm_result_t<ExPolicy,
                util::in_out_result<FwdIter1, FwdIter2>>
            parallel(ExPolicy&& policy, FwdIter1 first, Sent last,
                FwdIter2 dest, T const& val, Proj&& proj)
            {
                return copy_if<IterPair>().call(
                    HPX_FORWARD(ExPolicy, policy), first, last, dest,
                    [val](T const& a) -> bool { return !(a == val); },
                    HPX_FORWARD(Proj, proj));
            }
        };
        /// \endcond
    }    // namespace detail

    /////////////////////////////////////////////////////////////////////////////
    // remove_copy_if
    namespace detail {
        /// \cond NOINTERNAL

        // sequential remove_copy_if
        HPX_CXX_CORE_EXPORT template <typename InIter, typename Sent,
            typename OutIter, typename F, typename Proj>
        constexpr util::in_out_result<InIter, OutIter>
        sequential_remove_copy_if(
            InIter first, Sent last, OutIter dest, F p, Proj&& proj)
        {
            for (/* */; first != last; ++first)
            {
                if (!HPX_INVOKE(p, HPX_INVOKE(proj, *first)))
                {
                    *dest++ = *first;
                }
            }
            return util::in_out_result<InIter, OutIter>{first, dest};
        }

        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct remove_copy_if
          : public algorithm<remove_copy_if<IterPair>, IterPair>
        {
            constexpr remove_copy_if() noexcept
              : algorithm<remove_copy_if, IterPair>("remove_copy_if")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent,
                typename OutIter, typename F, typename Proj>
            static constexpr util::in_out_result<InIter, OutIter> sequential(
                ExPolicy, InIter first, Sent last, OutIter dest, F&& f,
                Proj&& proj)
            {
                return sequential_remove_copy_if(first, last, dest,
                    HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename FwdIter1, typename Sent,
                typename FwdIter2, typename F, typename Proj>
            static typename util::detail::algorithm_result<ExPolicy,
                util::in_out_result<FwdIter1, FwdIter2>>::type
            parallel(ExPolicy&& policy, FwdIter1 first, Sent last,
                FwdIter2 dest, F&& f, Proj&& proj)
            {
                return copy_if<IterPair>().call(
                    HPX_FORWARD(ExPolicy, policy), first, last, dest,
                    [f = HPX_FORWARD(F, f)](auto&& a) -> bool {
                        return !HPX_INVOKE(f, HPX_FORWARD(decltype(a), a));
                    },
                    HPX_FORWARD(Proj, proj));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::remove_copy_if
    HPX_CXX_CORE_EXPORT inline constexpr struct remove_copy_if_t final
      : hpx::detail::tag_parallel_algorithm<remove_copy_if_t>
    {
        template <typename InIter, typename OutIter, typename Pred>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter> &&
                hpx::traits::is_iterator_v<OutIter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<InIter>::value_type
                >
            )
        // clang-format on
        friend OutIter tag_fallback_invoke(hpx::remove_copy_if_t, InIter first,
            InIter last, OutIter dest, Pred pred)
        {
            static_assert(
                std::input_iterator<InIter>, "Required input iterator.");

            static_assert(std::output_iterator<OutIter,
                              hpx::traits::iter_value_t<InIter>>,
                "Required output iterator.");

            auto&& res = hpx::parallel::detail::remove_copy_if<
                hpx::parallel::util::in_out_result<InIter, OutIter>>()
                             .call(hpx::execution::sequenced_policy{}, first,
                                 last, dest, HPX_MOVE(pred), hpx::identity_v);

            return hpx::parallel::util::get_second_element(HPX_MOVE(res));
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename Pred>
        // clang-format off
             requires (
                hpx::is_execution_policy_v<ExPolicy>&&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter1>::value_type
                >
            )
        // clang-format on
        // clang-format off
        friend typename parallel::util::detail::algorithm_result<ExPolicy,
            FwdIter2>::type
        tag_fallback_invoke(hpx::remove_copy_if_t, ExPolicy&& policy,
            FwdIter1 first, FwdIter1 last, FwdIter2 dest, Pred pred)
        // clang-format on
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            auto&& res = hpx::parallel::detail::remove_copy_if<
                hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                             .call(HPX_FORWARD(ExPolicy, policy), first, last,
                                 dest, HPX_MOVE(pred), hpx::identity_v);

            return hpx::parallel::util::get_second_element(HPX_MOVE(res));
        }
    } remove_copy_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::remove_copy
    HPX_CXX_CORE_EXPORT inline constexpr struct remove_copy_t final
      : hpx::detail::tag_parallel_algorithm<remove_copy_t>
    {
    private:
        template <typename InIter, typename OutIter,
            typename T = typename std::iterator_traits<InIter>::value_type>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter> &&
                hpx::traits::is_iterator_v<OutIter>
            )
        // clang-format on
        friend OutIter tag_fallback_invoke(hpx::remove_copy_t, InIter first,
            InIter last, OutIter dest, T const& value)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            static_assert(std::output_iterator<OutIter,
                              hpx::traits::iter_value_t<InIter>>,
                "Requires at least output iterator.");

            using type = typename std::iterator_traits<InIter>::value_type;

            return hpx::remove_copy_if(first, last, dest,
                [value](type const& a) -> bool { return value == a; });
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename T = typename std::iterator_traits<FwdIter1>::value_type>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy>&&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        // clang-format off
        friend typename parallel::util::detail::algorithm_result<ExPolicy,
            FwdIter2>::type
        tag_fallback_invoke(hpx::remove_copy_t, ExPolicy&& policy,
            FwdIter1 first, FwdIter1 last, FwdIter2 dest, T const& value)
        // clang-format on
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            using type = typename std::iterator_traits<FwdIter1>::value_type;

            return hpx::remove_copy_if(HPX_FORWARD(ExPolicy, policy), first,
                last, dest,
                [value](type const& a) -> bool { return value == a; });
        }
    } remove_copy{};
}    // namespace hpx

#endif    // DOXYGEN
