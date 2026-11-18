//  Copyright 2013 Peter Dimov
//  Copyright (c) 2016 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <atomic>

namespace hpx::util {

    HPX_CXX_CORE_EXPORT class atomic_count
    {
    public:
        explicit constexpr atomic_count(long value) noexcept
          : value_(value)
        {
        }

        atomic_count(atomic_count const&) = delete;
        atomic_count(atomic_count&&) = delete;
        atomic_count& operator=(atomic_count const&) = delete;
        atomic_count& operator=(atomic_count&&) = delete;

        ~atomic_count() = default;

        atomic_count& operator=(long value) noexcept
        {
            value_.store(value, std::memory_order_relaxed);
            return *this;
        }

        void store(long value,
            std::memory_order mo = std::memory_order_relaxed) noexcept
        {
            value_.store(value, mo);
        }

        // Incrementing reference counts can be done with relaxed semantics,
        // because the object is not at risk of being destroyed, and any memory
        // operations that occur after the increment may as well have occurred
        // before the increment.
        long increment(
            std::memory_order mo = std::memory_order_relaxed) noexcept
        {
            return value_.fetch_add(1, mo) + 1;
        }

        long operator++() noexcept
        {
            return value_.fetch_add(1, std::memory_order_acq_rel) + 1;
        }

        // Any decrement of a reference count must be done with (at least)
        // release semantics so that any straggling writes to memory are visible
        // to the destructing thread before it frees the memory.
        long decrement(
            std::memory_order mo = std::memory_order_release) noexcept
        {
            return value_.fetch_sub(1, mo) - 1;
        }

        long operator--() noexcept
        {
            return value_.fetch_sub(1, std::memory_order_acq_rel) - 1;
        }

        atomic_count& operator+=(long n) noexcept
        {
            value_.fetch_add(n, std::memory_order_acq_rel);
            return *this;
        }

        atomic_count& operator-=(long n) noexcept
        {
            value_.fetch_sub(n, std::memory_order_acq_rel);
            return *this;
        }

        operator long() const noexcept
        {
            return value_.load(std::memory_order_acquire);
        }

    private:
        std::atomic<long> value_;
    };
}    // namespace hpx::util
