//  Copyright (c) 2024 Zakaria Abdi
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_contains_t final
    {
        template <typename Iterator, typename Sentinel, typename T,
            typename Proj>
        constexpr bool operator()(
            Iterator first, Sentinel last, T const& val, Proj&& proj) const
        {
            // no length prepass: it would consume a single-pass input range
            // before the search starts (loop_pred stops at last anyway)
            auto const itr =
                util::loop_pred<std::decay_t<hpx::execution::sequenced_policy>>(
                    first, last, [&val, &proj](auto const& cur) {
                        return HPX_INVOKE(proj, *cur) == val;
                    });

            return itr != last;
        }

        template <typename Iterator, typename T, typename Token, typename Proj>
        constexpr void operator()(Iterator first, T const& val,
            std::size_t count, Token& tok, Proj&& proj) const
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
