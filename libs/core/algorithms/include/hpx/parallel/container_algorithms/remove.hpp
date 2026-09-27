//  Copyright (c) 2017 Taeguk Kwon
//  Copyright (c) 2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/remove.hpp
/// \page hpx::ranges::remove, hpx::ranges::remove_if
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace ranges {
    /// Removes all elements for which predicate \a pred returns true
    /// from the range [first, last) and returns a subrange [ret, last),
    /// where ret is a past-the-end iterator for the new end of the range.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of
    ///         the predicate \a pred and the projection \a proj.
    ///
    /// \tparam Iter        The type of the source iterators used for the
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a remove_if requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible..
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param sent         Refers to the end of the sequence of elements the
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
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove_if algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a remove_if algorithm returns a \a
    ///           subrange_t<FwdIter, Sent>.
    ///           The \a remove_if algorithm returns an object {ret, last},
    ///           where ret is a past-the-end iterator for a new
    ///           subrange of the values all in valid but unspecified state.
    ///
    template <typename Iter, typename Sent, typename Pred,
        typename Proj = hpx::identity>
    subrange_t<Iter, Sent> remove_if(
        Iter first, Sent sent, Pred&& pred, Proj&& proj = Proj());

    /// Removes all elements that are equal to \a value from the range
    /// \a rng and and returns a subrange [ret, util::end(rng)), where ret
    /// is a past-the-end iterator for the new end of the range.
    ///
    /// \note   Complexity: Performs not more than \a util::end(rng)
    ///         - \a util::begin(rng) assignments, exactly
    ///         \a util::end(rng) - \a util::begin(rng) applications of
    ///         the operator==() and the projection \a proj.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a remove_if requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
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
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove_if algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a remove_if algorithm returns a \a
    ///           subrange_t<std::ranges::iterator_t<Rng>>.
    ///           The \a remove_if algorithm returns an object {ret, last},
    ///           where ret is a past-the-end iterator for a new
    ///           subrange of the values all in valid but unspecified state.
    ///
    template <typename Rng, typename Pred, typename Proj = hpx::identity>
    subrange_t<std::ranges::iterator_t<Rng>> remove_if(
        Rng&& rng, Pred&& pred, Proj&& proj = Proj());

    /// Removes all elements that are equal to \a value from the range
    /// [first, last) and and returns a subrange [ret, last), where ret
    /// is a past-the-end iterator for the new end of the range.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of
    ///         the operator==() and the projection \a proj.
    ///
    /// \tparam Iter        The type of the source iterators used for the
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Sent        The type of the end iterators used (deduced). This
    ///                     sentinel type must be a sentinel for FwdIter.
    /// \tparam T           The type of the value to remove (deduced).
    ///                     This value type must meet the requirements of
    ///                     \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param value        Specifies the value of elements to remove.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a remove algorithm returns a \a
    ///           subrange_t<FwdIter, Sent>.
    ///           The \a remove algorithm returns an object {ret, last},
    ///           where ret is a past-the-end iterator for a new
    ///           subrange of the values all in valid but unspecified state.
    ///
    template <typename Iter, typename Sent, typename Proj = hpx::identity,
        typename T =
            typename hpx::parallel::traits::projected<Iter, Proj>::value_type>
    subrange_t<Iter, Sent> remove(
        Iter first, Sent last, T const& value, Proj&& proj = Proj());

    /// Removes all elements that are equal to \a value from the range
    /// \a rng and and returns a subrange [ret, util::end(rng)), where ret
    /// is a past-the-end iterator for the new end of the range.
    ///
    /// \note   Complexity: Performs not more than \a util::end(rng)
    ///         - \a util::begin(rng) assignments, exactly
    ///         \a util::end(rng) - \a util::begin(rng) applications of
    ///         the operator==() and the projection \a proj.
    ///
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an forward iterator.
    /// \tparam T           The type of the value to remove (deduced).
    ///                     This value type must meet the requirements of
    ///                     \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity
    ///
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param value        Specifies the value of elements to remove.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The assignments in the parallel \a remove algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a remove algorithm returns a \a
    ///           subrange_t<std::ranges::iterator_t<Rng>>.
    ///           The \a remove algorithm returns an object {ret, last},
    ///           where ret is a past-the-end iterator for a new
    ///           subrange of the values all in valid but unspecified state.
    ///
    template <typename Rng, typename Proj = hpx::identity,
        typename T = typename hpx::parallel::traits::projected<
            std::ranges::iterator_t<Rng>, Proj>::value_type>
    subrange_t<std::ranges::iterator_t<Rng>> remove(
        Rng&& rng, T const& value, Proj&& proj = Proj());

    /// \brief Execution-policy overload of \c remove_if.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename Pred,
        typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> && std::permutable<I> &&
        std::indirect_unary_predicate<Pred, std::projected<I, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I>>
    remove_if(ExPolicy&& policy, I first, S last, Pred pred, Proj proj = {});

    /// \brief Execution-policy overload of \c remove_if.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        typename Pred, typename Proj = hpx::identity>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::permutable<std::ranges::iterator_t<R>> &&
        std::indirect_unary_predicate<Pred,
            std::projected<std::ranges::iterator_t<R>, Proj>>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R>>
    remove_if(ExPolicy&& policy, R&& rng, Pred pred, Proj proj = {});

    /// \brief Execution-policy overload of \c remove.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    template <typename ExPolicy, std::random_access_iterator I,
        std::sized_sentinel_for<I> S, typename Proj = hpx::identity,
        typename T = std::remove_cvref_t<
            std::invoke_result_t<Proj&, std::iter_value_t<I>&>>>
        requires hpx::is_execution_policy_v<ExPolicy> && std::permutable<I> &&
        std::indirect_binary_predicate<std::ranges::equal_to,
            std::projected<I, Proj>, T const*>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::subrange<I>>
    remove(ExPolicy&& policy, I first, S last, T const& value, Proj proj = {});

    /// \brief Execution-policy overload of \c remove.
    /// \note Requires random access iterators and sized sentinels, or sized
    /// random access ranges. Callable and element requirements are expressed
    /// in the constraints below.
    /// \returns The resulting subrange, wrapped in a future for task policies.
    /// Iterator and subrange results use the standard borrowed-range rules.
    /// A future does not extend the lifetime of the underlying range storage.
    template <typename ExPolicy, std::ranges::random_access_range R,
        typename Proj = hpx::identity,
        typename T = std::remove_cvref_t<std::invoke_result_t<Proj&,
            std::iter_value_t<std::ranges::iterator_t<R>>&>>>
        requires hpx::is_execution_policy_v<ExPolicy> &&
        std::ranges::sized_range<R> &&
        std::permutable<std::ranges::iterator_t<R>> &&
        std::indirect_binary_predicate<std::ranges::equal_to,
            std::projected<std::ranges::iterator_t<R>, Proj>, T const*>
    parallel::util::detail::algorithm_result_t<ExPolicy,
        std::ranges::borrowed_subrange_t<R>>
    remove(ExPolicy&& policy, R&& rng, T const& value, Proj proj = {});
}}    // namespace hpx::ranges

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/algorithm_value.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/algorithms/remove.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::remove_if
    HPX_CXX_CORE_EXPORT inline constexpr struct remove_if_t final
      : hpx::detail::tag_dispatch<remove_if_t,
            hpx::detail::tag_parallel_algorithm<remove_if_t>>
    {
        template <typename Iter, typename Sent, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter> &&
                std::sentinel_for<Sent, Iter> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<Iter>::value_type
                >
            )
        // clang-format on
        static subrange_t<Iter, Sent> invoke_default(
            Iter first, Sent sent, Pred pred, Proj proj = Proj())
        {
            static_assert(
                std::input_iterator<Iter>, "Required at least input iterator.");

            return hpx::parallel::util::make_subrange<Iter, Sent>(
                hpx::parallel::detail::remove_if<Iter>().call(
                    hpx::execution::seq, first, sent, HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                sent);
        }

        template <typename Rng, typename Pred, typename Proj = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<
                        std::ranges::iterator_t<Rng>
                    >::value_type
                >
            )
        // clang-format on
        static subrange_t<std::ranges::iterator_t<Rng>,
            std::ranges::sentinel_t<Rng>>
        invoke_default(Rng&& rng, Pred pred, Proj proj = Proj())
        {
            static_assert(std::input_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least input iterator.");

            return hpx::parallel::util::make_subrange<
                std::ranges::iterator_t<Rng>, std::ranges::sentinel_t<Rng>>(
                hpx::parallel::detail::remove_if<std::ranges::iterator_t<Rng>>()
                    .call(hpx::execution::seq, hpx::util::begin(rng),
                        hpx::util::end(rng), HPX_MOVE(pred), HPX_MOVE(proj)),
                hpx::util::end(rng));
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Pred,
            typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::permutable<I> &&
            std::indirect_unary_predicate<Pred, std::projected<I, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, Pred pred, Proj proj = {})
        {
            auto end = first + (last - first);
            return parallel::util::detail::convert_to_result(
                parallel::detail::remove_if<I>().call(
                    HPX_FORWARD(ExPolicy, policy), first, end, HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                [end](I position) -> std::ranges::subrange<I> {
                    return {position, end};
                });
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Pred, typename Proj = hpx::identity>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::permutable<std::ranges::iterator_t<R>> &&
            std::indirect_unary_predicate<Pred,
                std::projected<std::ranges::iterator_t<R>, Proj>>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, Pred pred, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                [](std::ranges::subrange<I> result)
                    -> std::ranges::borrowed_subrange_t<R> { return result; });
        }
    } remove_if{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::ranges::remove
    HPX_CXX_CORE_EXPORT inline constexpr struct remove_t final
      : hpx::detail::tag_dispatch<remove_t,
            hpx::detail::tag_parallel_algorithm<remove_t>>
    {
        template <typename Iter, typename Sent, typename Proj = hpx::identity,
            typename T = typename hpx::parallel::traits::projected<Iter,
                Proj>::value_type>
        // clang-format off
            requires(
                hpx::traits::is_iterator_v<Iter> &&
                hpx::parallel::traits::is_projected_v<Proj, Iter> &&
                std::sentinel_for<Sent, Iter>
            )
        // clang-format on
        static subrange_t<Iter, Sent> invoke_default(
            Iter first, Sent last, T const& value, Proj proj = Proj())
        {
            static_assert(
                std::input_iterator<Iter>, "Required at least input iterator.");

            using type = typename std::iterator_traits<Iter>::value_type;

            return hpx::ranges::remove_if(
                first, last,
                [value](type const& a) -> bool { return value == a; },
                HPX_MOVE(proj));
        }

        template <typename Rng, typename Proj = hpx::identity,
            typename T = typename hpx::parallel::traits::projected<
                std::ranges::iterator_t<Rng>, Proj>::value_type>
        // clang-format off
            requires(
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng>
            )
        // clang-format on
        static subrange_t<std::ranges::iterator_t<Rng>,
            std::ranges::sentinel_t<Rng>>
        invoke_default(Rng&& rng, T const& value, Proj proj = Proj())
        {
            static_assert(std::input_iterator<std::ranges::iterator_t<Rng>>,
                "Required at least input iterator.");

            using type = typename std::iterator_traits<
                std::ranges::iterator_t<Rng>>::value_type;

            return hpx::ranges::remove_if(
                HPX_FORWARD(Rng, rng),
                [value](type const& a) -> bool { return value == a; },
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::iter_value_t<I>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::permutable<I> &&
            std::indirect_binary_predicate<std::ranges::equal_to,
                std::projected<I, Proj>, T const*>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, I first, S last, T const& value, Proj proj = {})
        {
            auto end = first + (last - first);
            auto pred = parallel::detail::equal_to_value(value);
            return parallel::util::detail::convert_to_result(
                parallel::detail::remove_if<I>().call(
                    HPX_FORWARD(ExPolicy, policy), first, end, HPX_MOVE(pred),
                    HPX_MOVE(proj)),
                [end](I position) -> std::ranges::subrange<I> {
                    return {position, end};
                });
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<std::invoke_result_t<Proj&,
                std::iter_value_t<std::ranges::iterator_t<R>>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::permutable<std::ranges::iterator_t<R>> &&
            std::indirect_binary_predicate<std::ranges::equal_to,
                std::projected<std::ranges::iterator_t<R>, Proj>, T const*>
        static decltype(auto) invoke_default(
            ExPolicy&& policy, R&& rng, T const& value, Proj proj = {})
        {
            using I = std::ranges::iterator_t<R>;
            auto first = std::ranges::begin(rng);
            return parallel::util::detail::convert_to_result(
                invoke_default(HPX_FORWARD(ExPolicy, policy), first,
                    first + std::ranges::distance(rng), value, HPX_MOVE(proj)),
                [](std::ranges::subrange<I> result)
                    -> std::ranges::borrowed_subrange_t<R> { return result; });
        }

        using base_type = hpx::detail::tag_dispatch<remove_t,
            hpx::detail::tag_parallel_algorithm<remove_t>>;
        using base_type::operator();

        // Typed value parameters permit list-initialized arguments.
        template <typename ExPolicy, std::random_access_iterator I,
            std::sized_sentinel_for<I> S, typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::iter_value_t<I>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::permutable<I> &&
            std::indirect_binary_predicate<std::ranges::equal_to,
                std::projected<I, Proj>, T const*>
        decltype(auto) operator()(ExPolicy&& policy, I first, S last,
            T const& value, Proj proj = {}) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy), first,
                last, value, HPX_MOVE(proj));
        }

        template <typename ExPolicy, std::ranges::random_access_range R,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<std::invoke_result_t<Proj&,
                std::iter_value_t<std::ranges::iterator_t<R>>&>>>
            requires hpx::is_execution_policy_v<ExPolicy> &&
            std::ranges::sized_range<R> &&
            std::permutable<std::ranges::iterator_t<R>> &&
            std::indirect_binary_predicate<std::ranges::equal_to,
                std::projected<std::ranges::iterator_t<R>, Proj>, T const*>
        decltype(auto) operator()(
            ExPolicy&& policy, R&& rng, T const& value, Proj proj = {}) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy),
                HPX_FORWARD(R, rng), value, HPX_MOVE(proj));
        }
    } remove{};
}    // namespace hpx::ranges

#endif    // DOXYGEN
