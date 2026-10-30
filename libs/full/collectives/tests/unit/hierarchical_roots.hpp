//  Copyright (c) 2026 Alexander Strack
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/hpx.hpp>

#include <cstdint>
#include <set>
#include <utility>

// Calls f(root) for a few roots out of the participating localities: the first
// and the last non-zero ones and the middle one, besides the default root 0.
template <typename F>
void for_each_distributed_root(F&& f)
{
    std::uint32_t const n = hpx::get_num_localities(hpx::launch::sync);
    for (std::uint32_t const root : std::set<std::uint32_t>{0, 1, n - 1, n / 2})
    {
        f(root);
    }
}

// Calls f(num_sites, arity, root) for a few tree shapes (balanced, uneven and
// with a higher arity) and non-zero roots, simulated within one process.
template <typename F>
void for_each_local_root(F&& f)
{
    for (auto const& [n, arity] : {std::pair<std::uint32_t, int>{4, 2},
             std::pair<std::uint32_t, int>{5, 2},
             std::pair<std::uint32_t, int>{8, 2},
             std::pair<std::uint32_t, int>{11, 4},
             std::pair<std::uint32_t, int>{16, 4}})
    {
        for (std::uint32_t const root : {1u, n - 1, n / 2})
        {
            f(n, arity, root);
        }
    }
}
