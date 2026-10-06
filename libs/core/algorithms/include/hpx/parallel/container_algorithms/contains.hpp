//  Copyright (c) 2024 Zakaria Abdi
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected_range.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/all_any_none.hpp>
#include <hpx/parallel/algorithms/detail/algorithm_value.hpp>
#include <hpx/parallel/algorithms/detail/contains.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/search.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/adapt_placement_mode.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT struct contains : public algorithm<contains, bool>
    {
        constexpr contains() noexcept
          : algorithm("contains")
        {
        }

        template <typename ExPolicy, typename Iterator, typename Sentinel,
            typename T, typename Proj>
        static constexpr bool sequential(
            ExPolicy, Iterator first, Sentinel last, T const& val, Proj&& proj)
        {
            return sequential_contains<std::decay<ExPolicy>>(
                first, last, val, HPX_FORWARD(Proj, proj));
        }

        template <typename ExPolicy, typename Iterator, typename Sentinel,
            typename T, typename Proj>
        static util::detail::algorithm_result_t<ExPolicy, bool> parallel(
            ExPolicy&& orgpolicy, Iterator first, Sentinel last, T const& val,
            Proj&& proj)
        {
            using difference_type =
                typename std::iterator_traits<Iterator>::difference_type;
            difference_type count = detail::distance(first, last);
            if (count <= 0)
                return util::detail::algorithm_result<ExPolicy, bool>::get(
                    false);

            decltype(auto) policy =
                hpx::execution::experimental::adapt_placement_mode(
                    HPX_FORWARD(ExPolicy, orgpolicy),
                    hpx::threads::thread_placement_hint::breadth_first);

            using policy_type = std::decay_t<decltype(policy)>;
            util::cancellation_token<> tok;
            auto f1 = [val, tok, proj](Iterator first, std::size_t count) {
                sequential_contains<policy_type>(first, val, count, tok, proj);
                return tok.was_cancelled();
            };

            auto f2 = [](auto&& results) {
                return std::any_of(hpx::util::begin(results),
                    hpx::util::end(results), hpx::functional::unwrap{});
            };

            return util::partitioner<policy_type, bool>::call(
                HPX_FORWARD(decltype(policy), policy), first, count,
                HPX_MOVE(f1), HPX_MOVE(f2));
        }
    };

    HPX_CXX_CORE_EXPORT struct contains_subrange
      : public algorithm<contains_subrange, bool>
    {
        constexpr contains_subrange() noexcept
          : algorithm("contains_subrange")
        {
        }

        template <typename ExPolicy, typename FwdIter1, typename Sent1,
            typename FwdIter2, typename Sent2, typename Pred, typename Proj1,
            typename Proj2>
        static bool sequential(ExPolicy, FwdIter1 first1, Sent1 last1,
            FwdIter2 first2, Sent2 last2, Pred pred, Proj1&& proj1,
            Proj2&& proj2)
        {
            if (first2 == last2)
                return true;

            auto itr = hpx::parallel::detail::search<FwdIter1, Sent1>().call(
                hpx::execution::seq, first1, last1, first2, last2,
                HPX_MOVE(pred), HPX_FORWARD(Proj1, proj1),
                HPX_FORWARD(Proj2, proj2));

            return itr != last1;
        }

        template <typename ExPolicy, typename FwdIter1, typename Sent1,
            typename FwdIter2, typename Sent2, typename Pred, typename Proj1,
            typename Proj2>
        static constexpr util::detail::algorithm_result_t<ExPolicy, bool>
        parallel(ExPolicy&& policy, FwdIter1 first1, Sent1 last1,
            FwdIter2 first2, Sent2 last2, Pred pred, Proj1&& proj1,
            Proj2&& proj2)
        {
            if (first2 == last2)
                return util::detail::algorithm_result<ExPolicy, bool>::get(
                    true);

            return util::detail::convert_to_result(
                hpx::parallel::detail::search<FwdIter1, Sent1>().call(
                    HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2,
                    HPX_MOVE(pred), HPX_FORWARD(Proj1, proj1),
                    HPX_FORWARD(Proj2, proj2)),
                [last1](FwdIter1 it) { return it != last1; });
        }
    };
}    // namespace hpx::parallel::detail

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT inline constexpr struct contains_t final
      : hpx::detail::tag_dispatch<contains_t, hpx::detail::no_base>
    {
        template <typename Iterator, typename Sentinel, typename T,
            typename Proj = hpx::identity>
        // clang-format off
            requires(hpx::traits::is_iterator_v<Iterator> &&
                hpx::traits::is_iterator_v<Iterator> &&
                hpx::is_invocable_v<Proj,
                    typename std::iterator_traits<Iterator>::value_type>
            )
        // clang-format on
        static bool invoke_default(
            Iterator first, Sentinel last, T const& val, Proj&& proj = Proj())
        {
            static_assert(std::input_iterator<Iterator>,
                "Required at least input iterator.");

            static_assert(std::input_iterator<Sentinel>,
                "Required at least input iterator.");

            return hpx::parallel::detail::contains().call(
                hpx::execution::seq, first, last, val, proj);
        }

        template <typename Rng, typename T, typename Proj = hpx::identity>
        // clang-format off
            requires (
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng>
            )
        // clang-format on
        static bool invoke_default(Rng&& rng, T const& t, Proj proj = Proj())
        {
            return parallel::detail::contains().call(hpx::execution::seq,
                hpx::util::begin(rng), hpx::util::end(rng), t, HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Iterator, typename Sentinel,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::iter_value_t<Iterator>&>>>
        // clang-format off
            requires (
                parallel::detail::algorithm_value_argument<ExPolicy, T> &&
                hpx::is_execution_policy_v<ExPolicy> &&
                std::random_access_iterator<Iterator> &&
                std::sized_sentinel_for<Sentinel, Iterator> &&
                std::indirect_binary_predicate<std::ranges::equal_to,
                    std::projected<Iterator, Proj>, std::remove_reference_t<T> const*>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy, bool>
        invoke_default(ExPolicy&& policy, Iterator first, Sentinel last,
            T&& val, Proj proj = Proj())
        {
            return hpx::parallel::detail::any_of().call(
                HPX_FORWARD(ExPolicy, policy), first, last,
                parallel::detail::equal_to_value<ExPolicy>(HPX_FORWARD(T, val)),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Rng,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::ranges::range_value_t<Rng>&>>>
        // clang-format off
            requires (
                parallel::detail::algorithm_value_argument<ExPolicy, T> &&
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::random_access_range<Rng> &&
                std::ranges::sized_range<Rng> &&
                std::indirect_binary_predicate<std::ranges::equal_to,
                    std::projected<std::ranges::iterator_t<Rng>, Proj>,
                    std::remove_reference_t<T> const*>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy, bool>
        invoke_default(ExPolicy&& policy, Rng&& rng, T&& t, Proj proj = Proj())
        {
            return parallel::detail::any_of().call(
                HPX_FORWARD(ExPolicy, policy), std::ranges::begin(rng),
                (std::ranges::begin(rng) + std::ranges::distance(rng)),
                parallel::detail::equal_to_value<ExPolicy>(HPX_FORWARD(T, t)),
                HPX_MOVE(proj));
        }

        using base_type =
            hpx::detail::tag_dispatch<contains_t, hpx::detail::no_base>;
        using base_type::operator();

        /// \brief Supports list-initialized values in policy iterator calls.
        template <typename ExPolicy, typename Iter, typename Sent,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::iter_value_t<Iter>&>>>
            requires(parallel::detail::algorithm_value_argument<ExPolicy, T> &&
                hpx::is_execution_policy_v<ExPolicy> &&
                std::random_access_iterator<Iter> &&
                std::sized_sentinel_for<Sent, Iter> &&
                requires(ExPolicy&& policy, Iter first, Sent last, T&& value,
                    Proj proj) {
                    invoke_default(HPX_FORWARD(ExPolicy, policy), first, last,
                        HPX_FORWARD(T, value), HPX_MOVE(proj));
                })
        decltype(auto) operator()(ExPolicy&& policy, Iter first, Sent last,
            T&& value, Proj proj = Proj()) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy), first,
                last, HPX_FORWARD(T, value), HPX_MOVE(proj));
        }

        /// \brief Supports list-initialized values in policy range calls.
        template <typename ExPolicy, typename Rng,
            typename Proj = hpx::identity,
            typename T = std::remove_cvref_t<
                std::invoke_result_t<Proj&, std::ranges::range_value_t<Rng>&>>>
            requires(parallel::detail::algorithm_value_argument<ExPolicy, T> &&
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::random_access_range<Rng> &&
                std::ranges::sized_range<Rng> &&
                requires(ExPolicy&& policy, Rng&& rng, T&& value, Proj proj) {
                    invoke_default(HPX_FORWARD(ExPolicy, policy),
                        HPX_FORWARD(Rng, rng), HPX_FORWARD(T, value),
                        HPX_MOVE(proj));
                })
        decltype(auto) operator()(
            ExPolicy&& policy, Rng&& rng, T&& value, Proj proj = Proj()) const
        {
            return base_type::operator()(HPX_FORWARD(ExPolicy, policy),
                HPX_FORWARD(Rng, rng), HPX_FORWARD(T, value), HPX_MOVE(proj));
        }
    } contains{};

    HPX_CXX_CORE_EXPORT inline constexpr struct contains_subrange_t final
      : hpx::detail::tag_dispatch<contains_subrange_t, hpx::detail::no_base>
    {
        template <typename FwdIter1, typename Sent1, typename FwdIter2,
            typename Sent2, typename Pred = ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent1,FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                std::sentinel_for<Sent2,FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter1>::value_type,
                    typename std::iterator_traits<FwdIter2>::value_type> &&
                hpx::is_invocable_v<Proj1,
                    typename std::iterator_traits<FwdIter1>::value_type> &&
                hpx::is_invocable_v<Proj2,
                    typename std::iterator_traits<FwdIter2>::value_type>
            )
        // clang-format on
        static bool invoke_default(FwdIter1 first1, Sent1 last1,
            FwdIter2 first2, Sent2 last2, Pred pred = Pred(),
            Proj1&& proj1 = Proj1(), Proj2&& proj2 = Proj2())
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::contains_subrange().call(
                hpx::execution::seq, first1, last1, first2, last2,
                HPX_MOVE(pred), HPX_FORWARD(Proj1, proj1),
                HPX_FORWARD(Proj2, proj2));
        }

        template <typename Rng1, typename Rng2,
            typename Pred = hpx::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires(
                std::ranges::range<Rng1> &&
                hpx::parallel::traits::is_projected_range_v<Proj1,Rng1> &&
                std::ranges::range<Rng2> &&
                hpx::parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected_range<Proj1, Rng1>,
                    hpx::parallel::traits::projected_range<Proj2,Rng2>
                >
            )
        // clang-format on
        static bool invoke_default(Rng1&& rng1, Rng2&& rng2, Pred pred = Pred(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            return hpx::parallel::detail::contains_subrange().call(
                hpx::execution::seq, hpx::util::begin(rng1),
                hpx::util::end(rng1), hpx::util::begin(rng2),
                hpx::util::end(rng2), HPX_MOVE(pred), HPX_MOVE(proj1),
                HPX_MOVE(proj2));
        }

        template <typename ExPolicy, typename FwdIter1, typename Sent1,
            typename FwdIter2, typename Sent2,
            typename Pred = std::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::random_access_iterator<FwdIter1> &&
                std::sized_sentinel_for<Sent1, FwdIter1> &&
                std::random_access_iterator<FwdIter2> &&
                std::sized_sentinel_for<Sent2, FwdIter2> &&
                std::indirectly_comparable<FwdIter1,
                    FwdIter2, Pred, Proj1, Proj2>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy, bool>
        invoke_default(ExPolicy&& policy, FwdIter1 first1, Sent1 last1,
            FwdIter2 first2, Sent2 last2, Pred pred = Pred(),
            Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            return hpx::parallel::detail::contains_subrange().call(
                HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2,
                HPX_MOVE(pred), HPX_FORWARD(Proj1, proj1),
                HPX_FORWARD(Proj2, proj2));
        }

        template <typename ExPolicy, typename Rng1, typename Rng2,
            typename Pred = std::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::random_access_range<Rng1> &&
                std::ranges::sized_range<Rng1> &&
                std::ranges::random_access_range<Rng2> &&
                std::ranges::sized_range<Rng2> &&
                std::indirectly_comparable<std::ranges::iterator_t<Rng1>,
                    std::ranges::iterator_t<Rng2>, Pred, Proj1, Proj2>
            )
        // clang-format on
        static parallel::util::detail::algorithm_result_t<ExPolicy, bool>
        invoke_default(ExPolicy&& policy, Rng1&& rng1, Rng2&& rng2,
            Pred pred = Pred(), Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
        {
            return hpx::parallel::detail::contains_subrange().call(
                HPX_FORWARD(ExPolicy, policy), std::ranges::begin(rng1),
                (std::ranges::begin(rng1) + std::ranges::distance(rng1)),
                std::ranges::begin(rng2),
                (std::ranges::begin(rng2) + std::ranges::distance(rng2)),
                HPX_MOVE(pred), HPX_MOVE(proj1), HPX_MOVE(proj2));
        }
    } contains_subrange{};
}    // namespace hpx::ranges
