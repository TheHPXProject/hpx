//  Copyright (c) 2022 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_equal_t final
      : hpx::functional::detail::tag_fallback<sequential_equal_t<ExPolicy>>
    {
    private:
        template <typename InIter1, typename InIter2, typename F>
        friend constexpr bool tag_fallback_invoke(sequential_equal_t,
            InIter1 first1, InIter1 last1, InIter2 first2, F&& f)
        {
            return std::equal(first1, last1, first2, HPX_FORWARD(F, f));
        }

        template <typename ZipIterator, typename Token, typename F>
        friend constexpr void tag_fallback_invoke(sequential_equal_t,
            ZipIterator it, std::size_t part_count, Token& tok, F&& f)
        {
            util::const_loop_n<ExPolicy>(it, part_count, tok,
                [&f, &tok](auto const& curr) mutable -> void {
                    auto t = *curr;
                    if (!HPX_INVOKE(f, hpx::get<0>(t), hpx::get<1>(t)))
                    {
                        tok.cancel();
                    }
                });
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_equal_t<ExPolicy> sequential_equal =
        sequential_equal_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename InIter2, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE bool sequential_equal(
        InIter1 first1, InIter1 last1, InIter2 first2, F&& f)
    {
        return sequential_equal_t<ExPolicy>{}(
            first1, last1, first2, HPX_FORWARD(F, f));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename ZipIterator,
        typename Token, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE void sequential_equal(
        ZipIterator it, std::size_t part_count, Token& tok, F&& f)
    {
        return sequential_equal_t<ExPolicy>{}(
            it, part_count, tok, HPX_FORWARD(F, f));
    }
#endif

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_equal_binary_t final
      : hpx::functional::detail::tag_fallback<
            sequential_equal_binary_t<ExPolicy>>
    {
    private:
        // Our own version of the C++14 equal (_binary).
        template <typename InIter1, typename Sent1, typename InIter2,
            typename Sent2, typename F, typename Proj1, typename Proj2>
        friend constexpr bool tag_fallback_invoke(sequential_equal_binary_t,
            InIter1 first1, Sent1 last1, InIter2 first2, Sent2 last2, F&& f,
            Proj1&& proj1, Proj2&& proj2)
        {
            for (/* */; first1 != last1 && first2 != last2;
                (void) ++first1, ++first2)
            {
                if (!HPX_INVOKE(f, HPX_INVOKE(proj1, *first1),
                        HPX_INVOKE(proj2, *first2)))
                    return false;
            }
            return first1 == last1 && first2 == last2;
        }

        template <typename ZipIterator, typename Token, typename F,
            typename Proj1, typename Proj2>
        friend constexpr void tag_fallback_invoke(sequential_equal_binary_t,
            ZipIterator it, std::size_t part_count, Token& tok, F&& f,
            Proj1&& proj1, Proj2&& proj2)
        {
            util::const_loop_n<ExPolicy>(it, part_count, tok,
                [&f, &proj1, &proj2, &tok](auto const& curr) mutable -> void {
                    auto t = *curr;
                    if (!hpx::invoke(f, hpx::invoke(proj1, hpx::get<0>(t)),
                            hpx::invoke(proj2, hpx::get<1>(t))))
                    {
                        tok.cancel();
                    }
                });
        }
    };

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_equal_binary_t<ExPolicy>
        sequential_equal_binary = sequential_equal_binary_t<ExPolicy>{};
#else
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter1,
        typename Sent1, typename InIter2, typename Sent2, typename F,
        typename Proj1, typename Proj2>
    HPX_HOST_DEVICE HPX_FORCEINLINE bool sequential_equal_binary(InIter1 first1,
        Sent1 last1, InIter2 first2, Sent2 last2, F&& f, Proj1&& proj1,
        Proj2&& proj2)
    {
        return sequential_equal_binary_t<ExPolicy>{}(first1, last1, first2,
            last2, HPX_FORWARD(F, f), HPX_FORWARD(Proj1, proj1),
            HPX_FORWARD(Proj2, proj2));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename ZipIterator,
        typename Token, typename F, typename Proj1, typename Proj2>
    HPX_HOST_DEVICE HPX_FORCEINLINE void sequential_equal_binary(ZipIterator it,
        std::size_t part_count, Token& tok, F&& f, Proj1&& proj1, Proj2&& proj2)
    {
        return sequential_equal_binary_t<ExPolicy>{}(it, part_count, tok,
            HPX_FORWARD(F, f), HPX_FORWARD(Proj1, proj1),
            HPX_FORWARD(Proj2, proj2));
    }
#endif
}    // namespace hpx::parallel::detail
