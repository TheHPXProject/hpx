//  Copyright (c) 2026 Anfsity
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename FwdIter, typename Sent, typename T>
    constexpr FwdIter sequential_iota_helper(FwdIter first, Sent last, T& value)
    {
        for (; first != last; ++first)
        {
            *first = value;
            ++value;
        }
        return first;
    }

    HPX_CXX_CORE_EXPORT struct sequential_iota_t
      : hpx::functional::detail::tag_fallback<sequential_iota_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter, typename Sent,
            typename T>
        friend constexpr FwdIter tag_fallback_invoke(
            sequential_iota_t, ExPolicy&&, FwdIter first, Sent last, T& value)
        {
            return sequential_iota_helper(first, last, value);
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT inline constexpr sequential_iota_t sequential_iota =
        sequential_iota_t{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename FwdIter,
        typename Sent, typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE FwdIter sequential_iota(
        ExPolicy&& policy, FwdIter first, Sent last, T& value)
    {
        return sequential_iota_t{}(
            HPX_FORWARD(ExPolicy, policy), first, last, value);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////

    HPX_CXX_CORE_EXPORT template <typename Iter>
    struct iota : public algorithm<iota<Iter>, Iter>
    {
        constexpr iota() noexcept
          : algorithm<iota<Iter>, Iter>("iota")
        {
        }

        // sequential
        template <typename Expolicy, std::input_or_output_iterator FwdIter,
            std::sentinel_for<FwdIter> Sent, std::weakly_incrementable T>
            requires(std::indirectly_writable<FwdIter, T const&>)
        static FwdIter sequential(
            Expolicy&& policy, FwdIter first, Sent last, T value)
        {
            return sequential_iota(
                HPX_FORWARD(Expolicy, policy), first, last, value);
        }

        // parallel
        template <typename Expolicy, std::forward_iterator FwdIter,
            std::sentinel_for<FwdIter> Sent, std::weakly_incrementable T>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<std::decay_t<Expolicy>> &&
                std::indirectly_writable<FwdIter, T const&>
            )
        // clang-format on
        static auto parallel(
            Expolicy&& policy, FwdIter first, Sent last, T value)
        {
            constexpr bool is_parallel_iota_compatible =
                std::random_access_iterator<FwdIter> &&
                requires(T t, std::iter_difference_t<FwdIter> d) {
                    { t + d } -> std::convertible_to<T>;
                };

            if constexpr (is_parallel_iota_compatible)
            {
                auto dist = hpx::parallel::detail::distance(first, last);

                auto f = [value](FwdIter part_begin, std::size_t part_size,
                             std::size_t global_index) {
                    T cur_value = value + global_index;
                    return util::const_loop_n<std::decay_t<Expolicy>>(
                        part_begin, part_size,
                        [&cur_value](FwdIter it) { *it = cur_value++; });
                };

                return util::partitioner<Expolicy, FwdIter,
                    FwdIter>::call_with_index(HPX_FORWARD(Expolicy, policy),
                    first, dist, 1, std::move(f),
                    [last = first + dist](auto&&) { return last; });
            }
            else
            {
                return sequential_iota(
                    HPX_FORWARD(Expolicy, policy), first, last, value);
            }
        }
    };

}    // namespace hpx::parallel::detail
