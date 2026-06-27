//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  This file is based on the following cppreference possible implementation:
//  https://en.cppreference.com/w/cpp/algorithm/ranges/search
#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/advance_to_sentinel.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/adapt_placement_mode.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>
#include <hpx/parallel/util/compare_projected.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {
    /// \cond NOINTERNAL

    ///////////////////////////////////////////////////////////////////////////
    // sequential_search dispatch tag
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_search_t final
      : hpx::functional::detail::tag_fallback<sequential_search_t<ExPolicy>>
    {
    private:
        // Partitioned path: called from search::parallel() f1 for each chunk.
        // Checks each starting position in [base_idx, base_idx+part_size) for a
        // needle match; cancels tok at the first match position found.
        template <typename Iter1, typename Iter2, typename Token, typename Pred,
            typename Proj1, typename Proj2>
        friend inline constexpr void tag_fallback_invoke(
            sequential_search_t<ExPolicy>, Iter1 it, Iter2 s_first,
            std::size_t base_idx, std::size_t part_size, std::size_t diff,
            std::size_t count, Token& tok, Pred&& op, Proj1&& proj1,
            Proj2&& proj2)
        {
            using reference = typename std::iterator_traits<Iter1>::reference;
            Iter1 curr = it;
            util::loop_idx_n<ExPolicy>(base_idx, it, part_size, tok,
                [diff, count, s_first, &tok, &curr, op = HPX_FORWARD(Pred, op),
                    proj1 = HPX_FORWARD(Proj1, proj1),
                    proj2 = HPX_FORWARD(Proj2, proj2)](
                    reference v, std::size_t i) mutable -> void {
                    ++curr;
                    if (HPX_INVOKE(op, HPX_INVOKE(proj1, v),
                            HPX_INVOKE(proj2, *s_first)))
                    {
                        std::size_t local_count = 1;
                        Iter2 needle = s_first;
                        Iter1 mid = curr;
                        // clang-format off
                        for (std::size_t len = 0;
                            local_count != diff && len != count;
                            ++local_count, ++len, ++mid)
                        // clang-format on
                        {
                            if (!HPX_INVOKE(op, HPX_INVOKE(proj1, *mid),
                                    HPX_INVOKE(proj2, *++needle)))
                                break;
                        }
                        if (local_count == diff)
                            tok.cancel(i);
                    }
                });
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // sequential_search_n dispatch tag
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_search_n_t final
      : hpx::functional::detail::tag_fallback<sequential_search_n_t<ExPolicy>>
    {
    private:
        // Partitioned path: called from search_n::parallel() f1 for each chunk.
        // Checks each starting position in [base_idx, base_idx+part_size) for
        // count consecutive elements equal to value_proj.
        template <typename Iter, typename Size, typename V, typename Token,
            typename Pred, typename Proj>
        friend inline constexpr void tag_fallback_invoke(
            sequential_search_n_t<ExPolicy>, Iter it, std::size_t base_idx,
            std::size_t part_size, std::ptrdiff_t max_start, Size count,
            V const& value_proj, Token& tok, Pred&& pred, Proj&& proj)
        {
            using difference_type =
                typename std::iterator_traits<Iter>::difference_type;
            using reference = typename std::iterator_traits<Iter>::reference;
            std::size_t idx = 0;
            util::loop_idx_n<ExPolicy>(base_idx, it, part_size, tok,
                [max_start, count, it, &value_proj, &tok, &idx,
                    pred = HPX_FORWARD(Pred, pred),
                    proj = HPX_FORWARD(Proj, proj)](
                    reference, std::size_t abs_idx) mutable -> void {
                    if (static_cast<difference_type>(abs_idx) >= max_start)
                        return;
                    Iter start = it;
                    std::advance(start, static_cast<difference_type>(idx));
                    ++idx;
                    Iter curr = start;
                    Size matched = 0;
                    while (matched < count &&
                        HPX_INVOKE(pred, HPX_INVOKE(proj, *curr), value_proj))
                    {
                        ++curr;
                        ++matched;
                    }
                    if (matched == count)
                        tok.cancel(abs_idx);
                });
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // search
    HPX_CXX_CORE_EXPORT template <typename FwdIter, typename Sent>
    struct search final : public algorithm<search<FwdIter, Sent>, FwdIter>
    {
        constexpr search() noexcept
          : algorithm<search, FwdIter>("search")
        {
        }

        template <typename ExPolicy, typename FwdIter2, typename Sent2,
            typename Pred, typename Proj1, typename Proj2>
        static constexpr FwdIter sequential(ExPolicy, FwdIter first, Sent last,
            FwdIter2 s_first, Sent2 s_last, Pred&& op, Proj1&& proj1,
            Proj2&& proj2)
        {
            for (;; ++first)
            {
                FwdIter it1 = first;
                for (FwdIter2 it2 = s_first;; (void) ++it1, ++it2)
                {
                    if (it2 == s_last)
                        return first;
                    if (it1 == last)
                        return it1;
                    if (!HPX_INVOKE(op, HPX_INVOKE(proj1, *it1),
                            HPX_INVOKE(proj2, *it2)))
                        break;
                }
            }
        }

        template <typename ExPolicy, typename FwdIter2, typename Sent2,
            typename Pred, typename Proj1, typename Proj2>
        static decltype(auto) parallel(ExPolicy&& orgpolicy, FwdIter first,
            Sent last, FwdIter2 s_first, Sent2 s_last, Pred&& op, Proj1&& proj1,
            Proj2&& proj2)
        {
            using difference_type =
                typename std::iterator_traits<FwdIter>::difference_type;
            using s_difference_type =
                typename std::iterator_traits<FwdIter2>::difference_type;
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    FwdIter>;
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            // Use of hpx::distance instead of std::distance to support
            // sentinels
            s_difference_type diff =
                hpx::parallel::detail::distance(s_first, s_last);

            if constexpr (!has_scheduler_executor)
            {
                if (diff <= 0)
                    return result::get(HPX_MOVE(first));
            }

            difference_type count =
                hpx::parallel::detail::distance(first, last);

            if constexpr (!has_scheduler_executor)
            {
                if (diff > count)
                {
                    std::advance(
                        first, hpx::parallel::detail::distance(first, last));
                    return result::get(HPX_MOVE(first));
                }
            }

            hpx::parallel::util::cancellation_token<difference_type> tok(count);
            auto partitioner_count = count - (diff - 1);

            if constexpr (has_scheduler_executor)
            {
                if (diff <= 0 || diff > count)
                    partitioner_count = 0;

                if (diff <= 0)
                    tok.cancel(0);
            }

            decltype(auto) policy =
                hpx::execution::experimental::adapt_placement_mode(
                    HPX_FORWARD(ExPolicy, orgpolicy),
                    hpx::threads::thread_placement_hint::breadth_first);
            using policy_type = std::decay_t<decltype(policy)>;
            using partitioner =
                hpx::parallel::util::partitioner<decltype(policy), FwdIter,
                    void>;

            auto f1 = [diff, count, tok, s_first, op = HPX_FORWARD(Pred, op),
                          proj1 = HPX_FORWARD(Proj1, proj1),
                          proj2 = HPX_FORWARD(Proj2, proj2)](FwdIter it,
                          std::size_t part_size,
                          std::size_t base_idx) mutable -> void {
                sequential_search_t<policy_type>{}(it, s_first, base_idx,
                    part_size, static_cast<std::size_t>(diff),
                    static_cast<std::size_t>(count), tok, HPX_FORWARD(Pred, op),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));
            };

            auto f2 = [=](auto&&... data) mutable -> FwdIter {
                static_assert(sizeof...(data) < 2);

                // make sure iterators embedded in function object that is
                // attached to futures are invalidated
                util::detail::clear_container(data...);

                difference_type search_res = tok.get_data();
                std::advance(first, search_res);

                return HPX_MOVE(first);
            };

            return partitioner::call_with_index(
                HPX_FORWARD(decltype(policy), policy), first, partitioner_count,
                1, HPX_MOVE(f1), HPX_MOVE(f2));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // search_n
    HPX_CXX_CORE_EXPORT template <typename FwdIter, typename Sent>
    struct search_n final : public algorithm<search_n<FwdIter, Sent>, FwdIter>
    {
        constexpr search_n() noexcept
          : algorithm<search_n, FwdIter>("search_n")
        {
        }

        template <typename ExPolicy, typename Size, typename T, typename Pred,
            typename Proj>
        static FwdIter sequential(ExPolicy, FwdIter first, Sent last,
            Size count, T const& value, Pred&& pred, Proj&& proj)
        {
            using difference_type =
                typename std::iterator_traits<FwdIter>::difference_type;

            if (count <= 0)
                return first;
            if (first == last)
                return first;
            difference_type n = hpx::parallel::detail::distance(first, last);
            if (static_cast<difference_type>(count) > n)
            {
                return hpx::parallel::detail::advance_to_sentinel(first, last);
            }

            auto value_proj = proj(value);

            FwdIter it = first;
            FwdIter end = first;
            std::advance(end, n - static_cast<difference_type>(count) + 1);

            for (; it != end; ++it)
            {
                FwdIter curr = it;
                Size matched = 0;

                while (matched < count && pred(proj(*curr), value_proj))
                {
                    ++curr;
                    ++matched;
                }

                if (matched == count)
                    return it;
            }

            return hpx::parallel::detail::advance_to_sentinel(first, last);
        }
        template <typename ExPolicy, typename Size, typename T, typename Pred,
            typename Proj>
        static util::detail::algorithm_result_t<ExPolicy, FwdIter> parallel(
            ExPolicy&& orgpolicy, FwdIter first, Sent last, Size count,
            T const& value, Pred&& pred, Proj&& proj)
        {
            using result_type =
                util::detail::algorithm_result<ExPolicy, FwdIter>;
            using difference_type =
                typename std::iterator_traits<FwdIter>::difference_type;

            if (count <= 0)
                return result_type::get(HPX_MOVE(first));

            if (first == last)
                return result_type::get(HPX_MOVE(first));

            difference_type n = hpx::parallel::detail::distance(first, last);
            if (static_cast<difference_type>(count) > n)
            {
                return result_type::get(
                    hpx::parallel::detail::advance_to_sentinel(first, last));
            }

            // Number of valid starting positions
            difference_type max_start =
                n - static_cast<difference_type>(count) + 1;

            auto value_proj = proj(value);

            decltype(auto) policy =
                hpx::execution::experimental::adapt_placement_mode(
                    HPX_FORWARD(ExPolicy, orgpolicy),
                    hpx::threads::thread_placement_hint::breadth_first);

            using policy_type = std::decay_t<decltype(policy)>;
            using partitioner = util::partitioner<policy_type, FwdIter, void>;

            hpx::parallel::util::cancellation_token<difference_type> tok(
                max_start);

            auto f1 = [max_start, count, value_proj,
                          pred = HPX_FORWARD(Pred, pred),
                          proj = HPX_FORWARD(Proj, proj),
                          tok](FwdIter it, std::size_t part_size,
                          std::size_t base_idx) mutable -> void {
                sequential_search_n_t<policy_type>{}(it, base_idx, part_size,
                    static_cast<std::ptrdiff_t>(max_start), count, value_proj,
                    tok, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
            };

            auto f2 = [first, last, max_start, tok](
                          auto&&... data) mutable -> FwdIter {
                util::detail::clear_container(data...);

                difference_type idx = tok.get_data();
                if (idx == max_start)
                {
                    return HPX_MOVE(hpx::parallel::detail::advance_to_sentinel(
                        first, last));
                }

                std::advance(first, idx);
                return HPX_MOVE(first);
            };

            return partitioner::call_with_index(
                HPX_FORWARD(decltype(policy), policy), first, max_start, 1,
                HPX_MOVE(f1), HPX_MOVE(f2));
        }
    };
    /// \endcond
}    // namespace hpx::parallel::detail
