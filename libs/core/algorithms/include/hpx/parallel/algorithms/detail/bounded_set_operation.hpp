//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/for_each.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <cstddef>
#include <type_traits>
#include <vector>

namespace hpx::parallel::detail {
    /// \cond NOINTERNAL

    HPX_CXX_CORE_EXPORT enum class bounded_set_kind {
        set_union,
        intersection,
        difference,
        symmetric_difference
    };

    // Determine the bounded prefix in input order, then copy independent
    // selected elements in parallel. Keeping iterator positions avoids extra
    // value copies and evaluates comparisons only once, including skipped
    // elements after the destination has filled.
    HPX_CXX_CORE_EXPORT template <bounded_set_kind Kind, typename I1,
        typename I2, typename O>
    struct bounded_set_operation final
      : algorithm<bounded_set_operation<Kind, I1, I2, O>,
            util::in_in_out_result<I1, I2, O>>
    {
        using result_type = util::in_in_out_result<I1, I2, O>;

        bounded_set_operation()
          : algorithm<bounded_set_operation, result_type>("set_operation")
        {
        }

        struct selected_element
        {
            I1 first;
            I2 second;
            bool from_first;
        };

        template <typename ExPolicy, typename Comp, typename Proj1,
            typename Proj2>
        static result_type sequential(ExPolicy policy, I1 first1, I1 last1,
            I2 first2, I2 last2, O dest, O dest_last, Comp comp, Proj1 proj1,
            Proj2 proj2)
        {
            constexpr bool parallel_copy =
                !hpx::is_sequenced_execution_policy_v<ExPolicy> &&
                !hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;
            std::vector<selected_element> selected;
            auto out = dest;
            auto emit = [&](bool from_first) {
                if (out == dest_last)
                    return false;
                if constexpr (parallel_copy)
                {
                    selected.push_back({first1, first2, from_first});
                }
                else
                {
                    if constexpr (Kind == bounded_set_kind::intersection ||
                        Kind == bounded_set_kind::difference)
                        *out = *first1;
                    else if (from_first)
                        *out = *first1;
                    else
                        *out = *first2;
                }
                ++out;
                return true;
            };
            bool stopped = false;
            while (first1 != last1 && first2 != last2)
            {
                if (HPX_INVOKE(comp, HPX_INVOKE(proj1, *first1),
                        HPX_INVOKE(proj2, *first2)))
                {
                    if constexpr (Kind != bounded_set_kind::intersection)
                    {
                        if (!emit(true))
                        {
                            stopped = true;
                            break;
                        }
                    }
                    ++first1;
                }
                else if (HPX_INVOKE(comp, HPX_INVOKE(proj2, *first2),
                             HPX_INVOKE(proj1, *first1)))
                {
                    if constexpr (Kind == bounded_set_kind::set_union ||
                        Kind == bounded_set_kind::symmetric_difference)
                    {
                        if (!emit(false))
                        {
                            stopped = true;
                            break;
                        }
                    }
                    ++first2;
                }
                else
                {
                    if constexpr (Kind == bounded_set_kind::set_union ||
                        Kind == bounded_set_kind::intersection)
                    {
                        if (!emit(true))
                        {
                            stopped = true;
                            break;
                        }
                    }
                    ++first1;
                    ++first2;
                }
            }
            if (!stopped)
            {
                if constexpr (Kind != bounded_set_kind::intersection)
                {
                    while (first1 != last1 && emit(true))
                        ++first1;
                }
                else
                {
                    first1 = last1;
                }
                if constexpr (Kind == bounded_set_kind::set_union ||
                    Kind == bounded_set_kind::symmetric_difference)
                {
                    while (first2 != last2 && emit(false))
                        ++first2;
                }
                else
                {
                    first2 = last2;
                }
            }
            if constexpr (parallel_copy)
            {
                using index_iterator =
                    hpx::util::counting_iterator<std::size_t>;
                auto copy = [&selected, dest](std::size_t index) {
                    auto const& element = selected[index];
                    if constexpr (Kind == bounded_set_kind::intersection ||
                        Kind == bounded_set_kind::difference)
                        *(dest + index) = *element.first;
                    else if (element.from_first)
                        *(dest + index) = *element.first;
                    else
                        *(dest + index) = *element.second;
                };
                detail::for_each_n<index_iterator>().call(
                    hpx::execution::experimental::to_non_task(policy),
                    index_iterator(0), selected.size(), HPX_MOVE(copy),
                    hpx::identity{});
            }
            return {first1, first2, out};
        }

        template <typename ExPolicy, typename... Args>
        static decltype(auto) parallel(ExPolicy&& policy, Args&&... args)
        {
            // Schedule prefix discovery on the requested executor as well;
            // task policies must not perform the scan on the caller's thread.
            return bounded_set_operation().call2(HPX_FORWARD(ExPolicy, policy),
                std::true_type{}, HPX_FORWARD(Args, args)...);
        }
    };
    /// \endcond
}    // namespace hpx::parallel::detail
