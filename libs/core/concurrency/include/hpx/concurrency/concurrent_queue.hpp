//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/concurrency/queue.hpp>

#include <memory>

namespace hpx::concurrent {

    HPX_CXX_CORE_EXPORT template <typename T,
        typename Allocator = std::allocator<T>>
    using concurrent_queue = hpx::lockfree::queue<T, Allocator>;
}    // namespace hpx::concurrent
