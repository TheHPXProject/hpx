//  Copyright (c) 2024 Zakaria Abdi
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_contains_t final
      : hpx::functional::detail::tag_fallback<sequential_contains_t<ExPolicy>>
    {
    private:
        template <typename Iterator, typename Sentinel, typename T,
            typename Proj>
        friend constexpr bool tag_fallback_invoke(sequential_contains_t,
            Iterator first, Sentinel last, T const& val, Proj&& proj)
        {
            using difference_type =
                typename std::iterator_traits<Iterator>::difference_type;
            difference_type distance =
                hpx::parallel::detail::distance(first, last);
            if (distance <= 0)
                return false;

            auto const itr =
                util::loop_pred<std::decay_t<hpx::execution::sequenced_policy>>(
                    first, last, [&val, &proj](auto const& cur) {
                        return HPX_INVOKE(proj, *cur) == val;
                    });

            return itr != last;
        }

        template <typename Iterator, typename T, typename Token, typename Proj>
        friend constexpr void tag_fallback_invoke(sequential_contains_t,
            Iterator first, T const& val, std::size_t count, Token& tok,
            Proj&& proj)
        {
            util::const_loop_n<ExPolicy>(
                first, count, tok, [&val, &tok, &proj](auto const& cur) {
                    if (HPX_INVOKE(proj, *cur) == val)
                    {
                        tok.cancel();
                        return;
                    }
                });
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    inline constexpr sequential_contains_t<ExPolicy> sequential_contains =
        sequential_contains_t<ExPolicy>{};
}    //namespace hpx::parallel::detail
