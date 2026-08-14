////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2014 Peter Dimov
//  Copyright (c) 2020 Agustin Berge
//  Copyright (c) 2021 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

// see https://rigtorp.se/spinlock/

#pragma once

#include <hpx/config.hpp>

#include <atomic>

namespace hpx::util::detail {

    inline bool yield_k(unsigned const k) noexcept
    {
        if (k < 4)    //-V112
        {
            // do nothing
        }
        else if (k < 16)
        {
            HPX_SMT_PAUSE;
        }
        return false;
    }

    HPX_CORE_EXPORT bool yield_k_backoff(unsigned k) noexcept;

    /// Lockable spinlock class
    HPX_CXX_CORE_EXPORT template <bool Backoff>
    struct spinlock_impl
    {
    private:
        std::atomic<bool> m;

    public:
        constexpr spinlock_impl() noexcept
          : m(false)
        {
        }

        spinlock_impl(spinlock_impl const&) = delete;
        spinlock_impl(spinlock_impl&&) = delete;
        spinlock_impl& operator=(spinlock_impl const&) = delete;
        spinlock_impl& operator=(spinlock_impl&&) = delete;

        ~spinlock_impl() = default;

        HPX_FORCEINLINE bool try_lock() noexcept
        {
            // First do a relaxed load to check if lock is free in order to
            // prevent unnecessary cache misses if someone does
            // while(!try_lock())
            return !m.load(std::memory_order_relaxed) &&
                !m.exchange(true, std::memory_order_acquire);
        }

        HPX_FORCEINLINE void lock() noexcept
        {
            // Wait for lock to be released without generating cache misses
            // Similar implementation to hpx::spinlock
            unsigned k = 0;
            while (!try_lock())
            {
                if constexpr (Backoff)
                {
                    yield_k_backoff(k++);
                }
                else
                {
                    yield_k(k++ & 0xff);
                }
            }
        }

        HPX_FORCEINLINE void unlock() noexcept
        {
            m.store(false, std::memory_order_release);
        }
    };

    /// \brief \c spinlock is a type of lock that causes a thread attempting to
    ///        obtain it to check for its availability while waiting in a loop
    ///        continuously.
    HPX_CXX_CORE_EXPORT using spinlock = spinlock_impl<true>;
    HPX_CXX_CORE_EXPORT using spinlock_no_backoff = spinlock_impl<false>;
}    // namespace hpx::util::detail
