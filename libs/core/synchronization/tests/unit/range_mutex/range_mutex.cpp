//  Copyright (c) 2023 Johan511
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This work is based on https://github.com/Johan511/ByteLock

#include <hpx/hpx_main.hpp>
#include <hpx/synchronization/mutex.hpp>
#include <hpx/synchronization/range_mutex.hpp>

#include "range_mutex_util.hpp"

#include <cstddef>

int main()
{
    hpx::synchronization::range_mutex rm;
    {
        hpx::synchronization::range_unique_lock<
            hpx::synchronization::range_mutex>
            lg(rm, 1, 2);
    }
    hpx::ranged_lock::test::util::test_lock_n_times<
        hpx::synchronization::range_mutex>(
        10, 1'00'000, 4, 100,
        [](std::size_t x, std::size_t len) { return (std::min)(x + 100, len); },
        [](auto& v, std::size_t begin, std::size_t end) {
            for (std::size_t i = begin; i != end; i++)
            {
                v[i] += 1;
            }
        });
    return 0;
}
