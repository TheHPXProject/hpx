//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/cache/entries/lru_entry.hpp>

#include <atomic>
#include <cstdint>

namespace hpx::util::cache::entries::detail {

    // One counter for the whole process: entries that are created in one
    // module and touched in another one compare correctly.
    std::uint64_t next_lru_access_sequence() noexcept
    {
        static std::atomic<std::uint64_t> sequence(0);
        return sequence.fetch_add(1, std::memory_order_relaxed);
    }
}    // namespace hpx::util::cache::entries::detail
