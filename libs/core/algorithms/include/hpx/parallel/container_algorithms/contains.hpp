//  Copyright (c) 2024 Zakaria Abdi
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/contains.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/container_algorithms/search.hpp>
#include <hpx/parallel/util/adapt_placement_mode.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>
#include <algorithm>
#include <cstddef>
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

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T,
        typename Enable = void>
    struct contains_subrange_helper;

    template <typename ExPolicy, typename T>
    struct contains_subrange_helper<ExPolicy, T,
        std::enable_if_t<!hpx::is_async_execution_policy_v<ExPolicy>>>
    {
        static bool get_result(T& itr, T& last)
        {
            return itr != last;
        }
    };

    template <typename ExPolicy, typename T>
    struct contains_subrange_helper<ExPolicy, T,
        std::enable_if_t<hpx::is_async_execution_policy_v<ExPolicy>>>
    {
        static hpx::future<bool> get_result(hpx::future<T>& itr, T& last)
        {
            return itr.then(
                [last](hpx::future<T> it) { return it.get() != last; });
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
            auto itr = hpx::ranges::search(hpx::execution::seq, first1, last1,
                first2, last2, HPX_MOVE(pred), HPX_FORWARD(Proj1, proj1),
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
            auto itr = hpx::ranges::search(policy, first1, last1, first2, last2,
                HPX_MOVE(pred), HPX_FORWARD(Proj1, proj1),
                HPX_FORWARD(Proj2, proj2));

            return contains_subrange_helper<ExPolicy, FwdIter1>().get_result(
                itr, last1);
        }
    };
}    // namespace hpx::parallel::detail

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT inline constexpr struct contains_t final
      : hpx::functional::detail::tag_fallback<contains_t>
    {
    private:
        template <typename Iterator, typename Sentinel, typename T,
            typename Proj = hpx::identity>
        // clang-format off
            requires(hpx::traits::is_iterator_v<Iterator> &&
                hpx::traits::is_iterator_v<Iterator> &&
                hpx::is_invocable_v<Proj,
                    typename std::iterator_traits<Iterator>::value_type>
            )
        // clang-format on
        friend bool tag_fallback_invoke(hpx::ranges::contains_t, Iterator first,
            Sentinel last, T const& val, Proj&& proj = Proj())
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
        friend bool tag_fallback_invoke(
            hpx::ranges::contains_t, Rng&& rng, T const& t, Proj proj = Proj())
        {
            return parallel::detail::contains().call(hpx::execution::seq,
                hpx::util::begin(rng), hpx::util::end(rng), t, HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Iterator, typename Sentinel,
            typename T, typename Proj = hpx::identity>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<Iterator>&& hpx::traits::
                is_iterator_v<Iterator> &&
                hpx::is_invocable_v<Proj,
                typename std::iterator_traits<Iterator>::value_type>
            )
        // clang-format on
        friend parallel::util::detail::algorithm_result_t<ExPolicy, bool>
        tag_fallback_invoke(hpx::ranges::contains_t, ExPolicy&& policy,
            Iterator first, Sentinel last, T const& val, Proj&& proj = Proj())
        {
            static_assert(hpx::traits::is_iterator_v<Iterator>,
                "Required at least iterator.");

            static_assert(hpx::traits::is_iterator_v<Sentinel>,
                "Required at least iterator.");

            return hpx::parallel::detail::contains().call(
                HPX_FORWARD(ExPolicy, policy), first, last, val,
                HPX_FORWARD(Proj, proj));
        }

        template <typename ExPolicy, typename Rng, typename T,
            typename Proj = hpx::identity>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::ranges::range<Rng> &&
                hpx::parallel::traits::is_projected_range_v<Proj, Rng>
            )
        // clang-format on
        friend parallel::util::detail::algorithm_result_t<ExPolicy, bool>
        tag_fallback_invoke(hpx::ranges::contains_t, ExPolicy&& policy,
            Rng&& rng, T const& t, Proj proj = Proj())
        {
            return parallel::detail::contains().call(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng),
                hpx::util::end(rng), t, HPX_MOVE(proj));
        }
    } contains{};

    HPX_CXX_CORE_EXPORT inline constexpr struct contains_subrange_t final
      : hpx::functional::detail::tag_fallback<contains_subrange_t>
    {
    private:
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
        friend bool tag_fallback_invoke(hpx::ranges::contains_subrange_t,
            FwdIter1 first1, Sent1 last1, FwdIter2 first2, Sent2 last2,
            Pred pred = Pred(), Proj1&& proj1 = Proj1(),
            Proj2&& proj2 = Proj2())
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
        friend bool tag_fallback_invoke(hpx::ranges::contains_subrange_t,
            Rng1&& rng1, Rng2&& rng2, Pred pred = Pred(), Proj1 proj1 = Proj1(),
            Proj2 proj2 = Proj2())
        {
            return hpx::parallel::detail::contains_subrange().call(
                hpx::execution::seq, hpx::util::begin(rng1),
                hpx::util::end(rng1), hpx::util::begin(rng2),
                hpx::util::end(rng2), HPX_MOVE(pred), HPX_MOVE(proj1),
                HPX_MOVE(proj2));
        }

        template <typename ExPolicy, typename FwdIter1, typename Sent1,
            typename FwdIter2, typename Sent2,
            typename Pred = hpx::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                std::sentinel_for<Sent1,FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                std::sentinel_for<Sent2,FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter1>::value_type,
                    typename std::iterator_traits<FwdIter2>::value_type>
            )
        // clang-format on
        friend parallel::util::detail::algorithm_result_t<ExPolicy, bool>
        tag_fallback_invoke(hpx::ranges::contains_subrange_t, ExPolicy&& policy,
            FwdIter1 first1, Sent1 last1, FwdIter2 first2, Sent2 last2,
            Pred pred = Pred(), Proj1&& proj1 = Proj1(),
            Proj2&& proj2 = Proj2())
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");

            static_assert(std::forward_iterator<FwdIter2>,
                "Required at least forward iterator.");

            return hpx::parallel::detail::contains_subrange().call(
                HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2,
                HPX_MOVE(pred), HPX_FORWARD(Proj1, proj1),
                HPX_FORWARD(Proj2, proj2));
        }

        template <typename ExPolicy, typename Rng1, typename Rng2,
            typename Pred = hpx::ranges::equal_to,
            typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        // clang-format off
            requires (
                std::ranges::range<Rng1> &&
                hpx::parallel::traits::is_projected_range_v<Proj1, Rng1> &&
                std::ranges::range<Rng2> &&
                hpx::parallel::traits::is_projected_range_v<Proj2, Rng2> &&
                hpx::parallel::traits::is_indirect_callable_v<ExPolicy, Pred,
                    hpx::parallel::traits::projected_range<Proj1, Rng1>,
                    hpx::parallel::traits::projected_range<Proj2, Rng2>
                >
            )
        // clang-format on
        friend parallel::util::detail::algorithm_result_t<ExPolicy, bool>
        tag_fallback_invoke(hpx::ranges::contains_subrange_t, ExPolicy&& policy,
            Rng1&& rng1, Rng2&& rng2, Pred pred = Pred(), Proj1 proj1 = Proj1(),
            Proj2 proj2 = Proj2())
        {
            return hpx::parallel::detail::contains_subrange().call(
                HPX_FORWARD(ExPolicy, policy), hpx::util::begin(rng1),
                hpx::util::end(rng1), hpx::util::begin(rng2),
                hpx::util::end(rng2), HPX_MOVE(pred), HPX_MOVE(proj1),
                HPX_MOVE(proj2));
        }
    } contains_subrange{};
}    // namespace hpx::ranges
