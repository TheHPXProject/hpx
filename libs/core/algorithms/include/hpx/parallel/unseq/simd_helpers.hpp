//  Copyright (c) 2022 A Kishore Kumar
//  Copyright (c) 2023-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/unseq/reduce_helpers.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

// Please use static assert and enforce Iter to be Random Access Iterator
namespace hpx::parallel::util {

    // Compiler and Hardware should also support vector operations for IterDiff,
    // else we see slower performance when compared to sequential version
    HPX_CXX_CORE_EXPORT template <typename Iter, typename IterDiff, typename F>
    Iter unseq_first_n(Iter const first, IterDiff const n, F&& f) noexcept
    {
        // OMP loops can not have ++Iter, only integral types are allowed Hence
        // perform arithmetic on Iterators which is O(1) only in case of random
        // access iterators
        static_assert(std::random_access_iterator<Iter>,
            "algorithm is efficient only in case of Random Access Iterator");

#if HPX_EARLYEXIT_PRESENT
        IterDiff i = 0;
        // clang-format off
        HPX_PRAGMA_VECTOR_UNALIGNED HPX_PRAGMA_SIMD_EARLYEXIT
        for (; i < n; ++i)
        {
            if (f(*(first + i)))
            {
                break;
            }
        }
        // clang-format on

        return first + i;
#else
        // std::int32_t has best support for vectorization from compilers and
        // hardware
        IterDiff i = 0;
        constexpr std::int32_t num_blocks =
            HPX_LANE_SIZE / sizeof(std::int32_t);
        alignas(HPX_LANE_SIZE) std::int32_t simd_lane[num_blocks] = {};

        while (i <= n - num_blocks)
        {
            std::int32_t found_flag = 0;

            // clang-format off
            HPX_PRAGMA_VECTOR_UNALIGNED HPX_VECTOR_REDUCTION(| : found_flag)
            for (IterDiff j = i; j < i + num_blocks; ++j)
            {
                std::int32_t const t = f(*(first + j));
                simd_lane[j - i] = t;
                found_flag |= t;
            }
            // clang-format on

            if (found_flag)
            {
                IterDiff j;
                for (j = 0; j < num_blocks; ++j)
                {
                    if (simd_lane[j])
                    {
                        break;
                    }
                }
                return first + i + j;
            }
            i += num_blocks;
        }

        // Keep remainder scalar
        while (i != n)
        {
            if (f(*(first + i)))
            {
                break;
            }
            ++i;
        }
        return first + i;
#endif    // HPX_EARLYEXIT_PRESENT
    }

    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2,
        typename IterDiff, typename F>
    std::pair<Iter1, Iter2> unseq2_first_n(Iter1 const first1,
        Iter2 const first2, IterDiff const n, F&& f) noexcept
    {
#if HPX_EARLYEXIT_PRESENT
        IterDiff i = 0;

        // clang-format offs
        HPX_PRAGMA_VECTOR_UNALIGNED HPX_PRAGMA_SIMD_EARLYEXIT for (; i < n; ++i)
        {
            if (f(*(first1 + i), *(first2 + i)))
                break;
        }
        // clang-format on

        return std::make_pair(first1 + i, first2 + i);
#else

        constexpr std::int32_t num_blocks =
            HPX_LANE_SIZE / sizeof(std::int32_t);
        alignas(HPX_LANE_SIZE) std::int32_t simd_lane[num_blocks] = {};

        IterDiff outer_loop_ind = 0;
        while (outer_loop_ind <= n - num_blocks)
        {
            std::int32_t found_flag = 0;
            IterDiff i;

            // clang-format off
            HPX_PRAGMA_VECTOR_UNALIGNED HPX_VECTOR_REDUCTION(| : found_flag)
            for (i = 0; i < num_blocks; ++i)
            {
                IterDiff const t = f(*(first1 + outer_loop_ind + i),
                    *(first2 + outer_loop_ind + i));
                simd_lane[i] = t;
                found_flag |= t;
            }
            // clang-format on

            if (found_flag)
            {
                IterDiff i2;
                for (i2 = 0; i2 < num_blocks; ++i2)
                {
                    if (simd_lane[i2])
                        break;
                }
                return std::make_pair(
                    first1 + outer_loop_ind + i2, first2 + outer_loop_ind + i2);
            }
            outer_loop_ind += num_blocks;
        }

        // Keep remainder scalar
        for (; outer_loop_ind != n; ++outer_loop_ind)
        {
            if (f(*(first1 + outer_loop_ind), *(first2 + outer_loop_ind)))
                break;
        }

        return std::make_pair(first1 + outer_loop_ind, first2 + outer_loop_ind);
#endif    // HPX_EARLYEXIT_PRESENT
    }
}    // namespace hpx::parallel::util
