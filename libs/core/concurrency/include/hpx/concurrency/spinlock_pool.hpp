//  Copyright (c) 2012-2025 Hartmut Kaiser
//
//  taken from:
//  boost/detail/spinlock_pool.hpp
//
//  Copyright (c) 2008 Peter Dimov
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/concurrency/cache_line_data.hpp>
#include <hpx/modules/hashing.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/thread_support.hpp>

#include <cstddef>

namespace hpx::util {

    HPX_CXX_CORE_EXPORT template <typename Tag,
        std::size_t N = HPX_HAVE_SPINLOCK_POOL_NUM>
    class spinlock_pool
    {
    private:
        static cache_aligned_data<detail::spinlock> pool_[N];

    public:
        static detail::spinlock& spinlock_for(void const* pv) noexcept
        {
            std::size_t i = fibhash<N>(reinterpret_cast<std::size_t>(pv));
            return pool_[i].data_;
        }
    };

    template <typename Tag, std::size_t N>
    cache_aligned_data<detail::spinlock> spinlock_pool<Tag, N>::pool_[N];

}    // namespace hpx::util
