//  Copyright (c) 2020 Hartmut Kaiser
//  Copyright (c) 2021 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>

#include <algorithm>
#include <cstddef>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent, typename T>
    constexpr Iter sequential_fill_helper(Iter first, Sent last, T const& value)
    {
        for (; first != last; ++first)
        {
            *first = value;
        }
        return first;
    }

    HPX_CXX_CORE_EXPORT struct sequential_fill_t
      : hpx::detail::tag_dispatch<sequential_fill_t, hpx::detail::no_base>
    {
        template <typename ExPolicy, typename Iter, typename Sent, typename T>
        static constexpr Iter invoke_default(
            ExPolicy&&, Iter first, Sent last, T const& value)
        {
            return sequential_fill_helper(first, last, value);
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT inline constexpr sequential_fill_t sequential_fill =
        sequential_fill_t{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE Iter sequential_fill(
        ExPolicy&& policy, Iter first, Sent last, T const& value)
    {
        return sequential_fill_t{}(
            HPX_FORWARD(ExPolicy, policy), first, last, value);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter, typename T>
    constexpr Iter sequential_fill_n_helper(
        Iter first, std::size_t count, T const& value)
    {
        return std::fill_n(first, count, value);
    }

    HPX_CXX_CORE_EXPORT struct sequential_fill_n_t
      : hpx::detail::tag_dispatch<sequential_fill_n_t, hpx::detail::no_base>
    {
        template <typename ExPolicy, typename Iter, typename T>
        static constexpr Iter invoke_default(
            ExPolicy&&, Iter first, std::size_t count, T const& value)
        {
            return sequential_fill_n_helper(first, count, value);
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT inline constexpr sequential_fill_n_t sequential_fill_n =
        sequential_fill_n_t{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE Iter sequential_fill_n(
        ExPolicy&& policy, Iter first, std::size_t count, T const& value)
    {
        return sequential_fill_n_t{}(
            HPX_FORWARD(ExPolicy, policy), first, count, value);
    }
#endif

}    // namespace hpx::parallel::detail
